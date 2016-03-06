/*
 * qotdd.c
 *
 * qotd - A simple QOTD daemon.
 * Copyright (c) 2015-2016 Ammon Smith
 *
 * qotd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * qotd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with qotd.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "arguments.h"
#include "info.h"
#include "qotdd.h"
#include "quotes.h"
#include "sighandler.h"

#define CONNECTION_BACKLOG 50

#define UNUSED(x)    ((void)(x))
#define STREQ(x, y)  (strcmp((x), (y)) == 0)
#define ever         (;;)

static int daemonize();
static int main_loop();
static void *ipv4_main_loop(void *arg);
static void *ipv6_main_loop(void *arg);
static void setup_ipv4_socket(struct sockaddr_in *serv_addr);
static void setup_ipv6_socket(struct sockaddr_in6 *serv_addr);
static bool accept_ipv4_connection();
static bool accept_ipv6_connection();
static void save_args(const int argc, const char *argv[]);
static void check_config();
static void write_pidfile();

static arguments *args;
static options *opt;
static int ipv4_sockfd, ipv6_sockfd;
static pthread_t *ipv4_thread, *ipv6_thread;
static bool wrote_pidfile;

int main(int argc, const char *argv[])
{
    /* Set up signal handlers */
    set_up_handlers();

    /* Make a static copy of the arguments */
    save_args(argc, argv);

    /* Load configuration */
    load_config();

    /* Check configuration */
    check_config();

    /* Set default values for static variables */
    ipv4_sockfd = -1;
    ipv6_sockfd = -1;
    ipv4_thread = NULL;
    ipv6_thread = NULL;

    return (opt->daemonize) ? daemonize() : main_loop();
}

static int daemonize()
{
    /* Fork to background */
    pid_t pid;
    if ((pid = fork()) < 0) {
        cleanup(1);
        return 1;
    } else if (pid != 0) {
        /* If we're the parent, then quit */
        printf("Successfully created background daemon, pid %d.\n", pid);
        cleanup(0);
        return 0;
    }

    /* We're the child, so daemonize */
    pid = setsid();

    if (pid < 0) {
        perror("Unable to create new session");
        cleanup(1);
        return 1;
    }

    if (opt->chdir_root) {
        int ret = chdir("/");

        if (ret < 0) {
            perror("Unable to chdir to root dir");
        }
    }

    return main_loop();
}

static int main_loop()
{
    write_pidfile();

    int ret;
    switch (opt->protocol) {
        case PROTOCOL_IPV4:
            ipv4_main_loop(NULL);
            break;
        case PROTOCOL_IPV6:
            ipv6_main_loop(NULL);
            break;
        case PROTOCOL_BOTH:
            /* Create threads */
            ret = pthread_create(ipv4_thread, NULL, ipv4_main_loop, NULL);
            if (ret) {
                perror("Unable to create IPv4 listener thread");
                ipv4_thread = NULL;
                cleanup(1);
            }

            ret = pthread_create(ipv6_thread, NULL, ipv6_main_loop, NULL);
            if (ret) {
                perror("Unable to create IPv6 listener thread");
                ipv6_thread = NULL;
                cleanup(1);
            }

            /* Block until the threads join */
            ret = pthread_join(*ipv4_thread, NULL);
            if (ret) {
                perror("Unable to join IPv4 listener thread");
                ipv4_thread = NULL;
                cleanup(1);
            }

            ret = pthread_join(*ipv6_thread, NULL);
            if (ret) {
                perror("Unable to join IPv6 listener thread");
                ipv6_thread = NULL;
                cleanup(1);
            }
            break;
        default:
            fprintf(stderr, "Internal error: invalid protocol value: %d.\n", opt->protocol);
            cleanup(1);
    }

    return EXIT_SUCCESS;
}

static void *ipv4_main_loop(void *arg)
{
    UNUSED(arg);
    struct sockaddr_in serv_addr;
    setup_ipv4_socket(&serv_addr);

    printf("Starting main loop (IPv4)...\n");
    for ever {
        accept_ipv4_connection();
    }

    return NULL;
}

static void *ipv6_main_loop(void *arg)
{
    UNUSED(arg);
    struct sockaddr_in6 serv_addr;
    setup_ipv6_socket(&serv_addr);

    printf("Starting main loop (IPv6)...\n");
    for ever {
        accept_ipv6_connection();
    }

    return NULL;
}

static void setup_ipv4_socket(struct sockaddr_in *serv_addr)
{
    printf("Setting up IPv4 socket connection...\n");
    ipv4_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ipv4_sockfd < 0) {
        perror("Unable to connect to IPv4 socket");
        cleanup(1);
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = INADDR_ANY;
    serv_addr->sin_port = htons(opt->port);

    int ret = bind(ipv4_sockfd, (const struct sockaddr *)serv_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        perror("Unable to bind to IPv4 socket");
        cleanup(1);
    }
}

static void setup_ipv6_socket(struct sockaddr_in6 *serv_addr)
{
    printf("Setting up IPv6 socket connection...\n");
    ipv6_sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (ipv6_sockfd < 0) {
        perror("Unable to connect to IPv6 socket");
        cleanup(1);
    }

    serv_addr->sin6_family = AF_INET6;
    serv_addr->sin6_addr = in6addr_any;
    serv_addr->sin6_port = htons(opt->port);

    int ret = bind(ipv6_sockfd, (const struct sockaddr *)serv_addr, sizeof(struct sockaddr_in6));
    if (ret < 0) {
        perror("Unable to bind to IPv6 socket");
        cleanup(1);
    }
}

static bool accept_ipv4_connection()
{
    printf("Listening for IPv4 connection...\n");
    listen(ipv4_sockfd, CONNECTION_BACKLOG);

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int consockfd = accept(ipv4_sockfd, (struct sockaddr *)(&cli_addr), &clilen);
    if (consockfd < 0) {
        perror("Unable to accept connection");
        return false;
    }

    int ret = send_quote(consockfd, opt);
    if (ret < 0) {
        perror("Unable to write to socket");
        return false;
    }

    ret = close(consockfd);
    if (ret < 0) {
        perror("Unable to close connection");
        return false;
    }

    return true;
}

static bool accept_ipv6_connection()
{
    printf("Listening for IPv6 connection...\n");
    listen(ipv6_sockfd, CONNECTION_BACKLOG);

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int consockfd = accept(ipv6_sockfd, (struct sockaddr *)(&cli_addr), &clilen);
    if (consockfd < 0) {
        perror("Unable to accept connection");
        return false;
    }

    int ret = send_quote(consockfd, opt);
    if (ret < 0) {
        perror("Unable to write to socket");
        return false;
    }

    ret = close(consockfd);
    if (ret < 0) {
        perror("Unablee to close connection");
        return false;
    }

    return true;
}

void cleanup(const int ret)
{
    printf("Quitting with exit code %d.\n", ret);
    quietcleanup(ret);
}

void quietcleanup(int ret)
{
    int ret2;
    if (ipv4_thread) {
        ret2 = pthread_cancel(*ipv4_thread);
        if (ret2) {
            perror("Unable to cancel IPv4 pthread");
            ret++;
        }
    }

    if (ipv6_thread) {
        ret2 = pthread_cancel(*ipv6_thread);
        if (ret2) {
            perror("Unable to cancel IPv6 pthread");
            ret++;
        }
    }

    if (ipv4_sockfd >= 0) {
        ret2 = close(ipv4_sockfd);
        if (ret2 < 0) {
            fprintf(stderr, "Unable to close IPv4 socket filed descriptor %d: %s\n",
                    ipv4_sockfd, strerror(errno));
            ret++;
        }
    }

    if (ipv6_sockfd >= 0) {
        ret2 = close(ipv6_sockfd);
        if (ret2 < 0) {
            fprintf(stderr, "Unable to close IPv6 socket file descriptor %d: %s\n",
                    ipv6_sockfd, strerror(errno));
            ret++;
        }
    }

    if (args) {
        free(args);
        args = NULL;
    }

    if (opt) {
        if (wrote_pidfile) {
            ret2 = unlink(opt->pidfile);
            if (ret2 < 0) {
                fprintf(stderr, "Unable to remove pid file (%s): %s\n",
                        opt->pidfile, strerror(errno));
                ret++;
            }
        }

        if (opt->quotesmalloc) {
            free(opt->quotesfile);
        }

        if (opt->pidmalloc) {
            free(opt->pidfile);
        }

        free(opt);
        opt = NULL;
    }

    exit(ret);
}

void load_config()
{
    printf("Loading configuration settings...\n");

    if (opt) {
        free(opt);
    }

    opt = parse_args(args->argc, args->argv);
}

static void save_args(const int argc, const char *argv[])
{
    arguments args_ = {
        .argc = argc,
        .argv = argv,
    };

    args = malloc(sizeof(arguments));
    memcpy(args, &args_, sizeof(arguments));
}

static void write_pidfile()
{
    if (STREQ(opt->pidfile, "none")) {
        printf("No pidfile was written at the request of the user.\n");
        return;
    }

    FILE *fh = fopen(opt->pidfile, "w+");
    int ret;

    if (fh == NULL) {
        perror("Unable to open pid file");

        if (opt->require_pidfile) {
            cleanup(1);
        } else {
            return;
        }
    }

    ret = fprintf(fh, "%d", getpid());
    if (ret < 0) {
        perror("Unable to write pid to pid file");

        if (opt->require_pidfile) {
            fclose(fh);
            cleanup(1);
        }
    }

    wrote_pidfile = true;

    ret = fclose(fh);
    if (ret < 0) {
        perror("Unable to close pid file handle");

        if (opt->require_pidfile) {
            cleanup(1);
        }
    }
}

static void check_config()
{
    if (opt->port < 1024 && geteuid() != 0) {
        fprintf(stderr, "Only root can bind to ports below 1024.\n");
        cleanup(1);
    }

    if (opt->pidfile[0] != '/') {
        fprintf(stderr, "Specified pid file is not an absolute path.\n");
        cleanup(1);
    }

    struct stat *statbuf = malloc(sizeof(struct stat));
    if (statbuf == NULL) {
        perror("Unable to allocate memory for stat of quotes file");
        cleanup(1);
    }

    int ret = stat(opt->quotesfile, statbuf);
    free(statbuf);

    if (ret < 0) {
        perror("Unable to stat quotes file");
        cleanup(1);
    }
}

