/*
 * daemon.c
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "arguments.h"
#include "daemon.h"
#include "info.h"
#include "quotes.h"
#include "signal_handler.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CONNECTION_BACKLOG 50

#define UNUSED(x)    ((void)(x))
#define STREQ(x, y)  (strcmp((x), (y)) == 0)

static int daemonize();
static int main_loop();
static void setup_ipv4_socket();
static void setup_ipv6_socket();
static bool accept_connection();
static void save_args(const int argc, const char *argv[]);
static void check_config();
static void write_pidfile();

static struct arguments args;
static struct options opt;
static int sockfd;
static bool wrote_pidfile;

int main(int argc, const char *argv[])
{
    /* Set up signal handlers */
    set_up_handlers();

    /* Set default values for static variables */
    sockfd = -1;

    /* Make a static copy of the arguments */
    save_args(argc, argv);

    /* Load configuration */
    load_config();

    /* Check configuration */
    check_config();

    return (opt.daemonize) ? daemonize() : main_loop();
}

static int daemonize()
{
    /* Fork to background */
    pid_t pid;
    if ((pid = fork()) < 0) {
        cleanup(1, true);
        return 1;
    } else if (pid != 0) {
        /* If we're the parent, then quit */
        printf("Successfully created background daemon, pid %d.\n", pid);
        cleanup(0, true);
        return 0;
    }

    /* We're the child, so daemonize */
    pid = setsid();

    if (pid < 0) {
        perror("Unable to create new session");
        cleanup(1, true);
        return 1;
    }

    if (opt.chdir_root) {
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

    switch (opt.iproto) {
        case PROTOCOL_BOTH:
        case PROTOCOL_IPv6:
            setup_ipv6_socket();
            break;
        case PROTOCOL_IPv4:
            setup_ipv4_socket();
            break;
        default:
            fprintf(stderr, "Invalid enum value for \"iproto\": %d.\n", opt.iproto);
    }

    while (accept_connection());

    cleanup(1, true);
    return EXIT_FAILURE;
}

static void setup_ipv4_socket()
{
    struct sockaddr_in serv_addr;
    int ret;

    if (opt.tproto == PROTOCOL_TCP) {
        printf("Setting up IPv4 socket connection over TCP...\n");
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        printf("Setting up IPv4 socket connection over UDP...\n");
        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (sockfd < 0) {
        perror("Unable to connect to IPv4 socket");
        cleanup(1, true);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(opt.port);

    ret = bind(sockfd, (const struct sockaddr *)(&serv_addr), sizeof(struct sockaddr_in));
    if (ret < 0) {
        perror("Unable to bind to IPv4 socket");
        cleanup(1, true);
    }
}

static void setup_ipv6_socket()
{
    struct sockaddr_in6 serv_addr;
    bool no_ipv4;
    int ret;

    no_ipv4 = (opt.iproto != PROTOCOL_BOTH);

    if (opt.tproto == PROTOCOL_TCP) {
        printf("Setting up IPv%s6 socket connection over TCP...\n", (no_ipv4 ? "" : "4/"));
        sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    } else {
        printf("Setting up IPv%s6 socket connection over UDP...\n", (no_ipv4 ? "" : "4/"));
        sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (sockfd < 0) {
        perror("Unable to connect to IPv6 socket");
        cleanup(1, true);
    }

    setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)(&no_ipv4), sizeof(no_ipv4));

    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(opt.port);

    ret = bind(sockfd, (const struct sockaddr *)(&serv_addr), sizeof(struct sockaddr_in6));
    if (ret < 0) {
        if (no_ipv4) {
            perror("Unable to bind to IPv6 socket");
        } else {
            perror("Unable to bind to IPv4/6 socket");
        }

        cleanup(1, true);
    }
}

static bool accept_connection()
{
    struct sockaddr_in cli_addr;
    int consockfd, ret;
    socklen_t clilen;

    printf("Listening for connection...\n");
    listen(sockfd, CONNECTION_BACKLOG);

    clilen = sizeof(cli_addr);
    consockfd = accept(sockfd, (struct sockaddr *)(&cli_addr), &clilen);
    if (consockfd < 0) {
        perror("Unable to accept connection");
        return false;
    }

    ret = send_quote(consockfd, &opt);
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

void cleanup(int ret, bool quiet)
{
    int ret2;

    if (!quiet) {
        ret2 = printf("Quitting with exit code %d.\n", ret);

        if (ret2 < 0) {
            ret++;
        }
    }

    if (sockfd >= 0) {
        ret2 = close(sockfd);
        if (ret2 < 0) {
            ret2 = fprintf(stderr, "Unable to close socket file descriptor %d: %s.\n",
                    sockfd, strerror(errno));
            ret++;

            if (ret2 < 0) {
                ret++;
            }
        }
    }

    if (wrote_pidfile) {
        ret2 = unlink(opt.pidfile);
        if (ret2 < 0) {
            ret2 = fprintf(stderr, "Unable to remove pid file (%s): %s.\n",
                    opt.pidfile, strerror(errno));
            ret++;

            if (ret2 < 0) {
                ret++;
            }
        }
    }

    if (opt.quotesmalloc) {
        free((char *)opt.quotesfile);
    }

    if (opt.pidmalloc) {
        free((char *)opt.pidfile);
    }

    exit(ret);
}

void load_config()
{
    printf("Loading configuration settings...\n");
    parse_args(&opt, args.argc, args.argv);
}

static void save_args(int argc, const char *argv[])
{
    args.argc = argc;
    args.argv = argv;
}

static void write_pidfile()
{
    struct stat statbuf;
    int ret;
    FILE *fh;

    if (STREQ(opt.pidfile, "none")) {
        printf("No pidfile was written at the request of the user.\n");
        return;
    }

    /* Check if the pidfile already exists */
    ret = stat(opt.pidfile, &statbuf);

    if (ret == 0) {
        printf("The pid file already exists. Quitting.\n");
        cleanup(1, true);
    }

    /* Write the pidfile */
    fh = fopen(opt.pidfile, "w+");

    if (fh == NULL) {
        perror("Unable to open pid file");

        if (opt.require_pidfile) {
            cleanup(1, true);
        } else {
            return;
        }
    }

    ret = fprintf(fh, "%d\n", getpid());
    if (ret < 0) {
        perror("Unable to write pid to pid file");

        if (opt.require_pidfile) {
            fclose(fh);
            cleanup(1, true);
        }
    }

    wrote_pidfile = true;

    ret = fclose(fh);
    if (ret < 0) {
        perror("Unable to close pid file handle");

        if (opt.require_pidfile) {
            cleanup(1, true);
        }
    }
}

static void check_config()
{
    struct stat statbuf;
    int ret;

    if (opt.port < 1024 && geteuid() != 0) {
        fprintf(stderr, "Only root can bind to ports below 1024.\n");
        cleanup(1, true);
    }

    if (opt.pidfile[0] != '/') {
        fprintf(stderr, "Specified pid file is not an absolute path.\n");
        cleanup(1, true);
    }

    ret = stat(opt.quotesfile, &statbuf);

    if (ret < 0) {
        perror("Unable to stat quotes file");
        cleanup(1, true);
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
