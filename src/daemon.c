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
#include "journal.h"
#include "quotes.h"
#include "signal_handler.h"
#include "standard.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CONNECTION_BACKLOG 50

#define UNUSED(x)    ((void)(x))
#define STREQUALS(x, y)  (strcmp((x), (y)) == 0)

/* Static function declarations */
static int daemonize();
static int main_loop();
static void setup_ipv4_socket();
static void setup_ipv6_socket();
static bool tcp_accept_connection();
static bool udp_accept_connection();
static void check_errno();
static void save_args(const int argc, const char *argv[]);
static void check_config();
static void write_pidfile();

/* Static member declarations */
static struct arguments args;
static struct options opt;
static int sockfd;
static bool wrote_pidfile;

/* Function implementations */
int main(int argc, const char *argv[])
{
    /* Set up signal handlers */
    set_up_handlers();

    /* Set default values for static variables */
    sockfd = -1;
    wrote_pidfile = false;

    /* Make a static copy of the arguments */
    save_args(argc, argv);

    /* Load configuration */
    load_config();

    return opt.daemonize ? daemonize() : main_loop();
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
        journal("Successfully created background daemon, pid %d.\n", pid);
        cleanup(0, true);
        return 0;
    }

    /* We're the child, so daemonize */
    pid = setsid();

    if (pid < 0) {
        journal("Unable to create new session: %s.\n", strerror(errno));
        cleanup(1, true);
        return 1;
    }

    if (opt.chdir_root) {
        int ret = chdir("/");

        if (ret < 0) {
            journal("Unable to chdir to root dir: %s.\n", strerror(errno));
        }
    }

    return main_loop();
}

static int main_loop()
{
    bool (*accept_connection)();
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
            journal("Internal error: invalid enum value for \"iproto\": %d.\n", opt.iproto);
            cleanup(1, true);
    }

    switch (opt.tproto) {
        case PROTOCOL_TCP:
            accept_connection = &tcp_accept_connection;
            break;
        case PROTOCOL_UDP:
            accept_connection = &udp_accept_connection;
            break;
        default:
            journal("Internal error: invalid enum value for \"tproto\": %d.\n", opt.tproto);
            cleanup(1, true);
            return EXIT_FAILURE;
    }

    for (;;) {
        accept_connection();
    }

    return EXIT_SUCCESS;
}

static void setup_ipv4_socket()
{
    struct sockaddr_in serv_addr;
    int ret, one = 1;

    if (opt.tproto == PROTOCOL_TCP) {
        journal("Setting up IPv4 socket connection over TCP...\n");
        sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    } else {
        journal("Setting up IPv4 socket connection over UDP...\n");
        sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (sockfd < 0) {
        journal("Unable to create IPv4 socket: %s.\n", strerror(errno));
        cleanup(1, true);
    }

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)(&one), sizeof(one));
    if (ret < 0) {
        journal("Unable to set the socket to allow address reuse: %s.\n", strerror(errno));
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(opt.port);

    ret = bind(sockfd, (const struct sockaddr *)(&serv_addr), sizeof(struct sockaddr_in));
    if (ret < 0) {
        journal("Unable to bind to IPv4 socket: %s.\n", strerror(errno));
        cleanup(1, true);
    }
}

static void setup_ipv6_socket()
{
    struct sockaddr_in6 serv_addr;
    bool no_ipv4;
    int ret, one = 1;

    no_ipv4 = (opt.iproto != PROTOCOL_BOTH);

    if (opt.tproto == PROTOCOL_TCP) {
        journal("Setting up IPv%s6 socket connection over TCP...\n",
                ((opt.iproto == PROTOCOL_BOTH) ? "4/" : ""));
        sockfd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    } else {
        printf("Setting up IPv%s6 socket connection over UDP...\n", (no_ipv4 ? "" : "4/"));
        sockfd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (sockfd < 0) {
        journal("Unable to create IPv6 socket: %s.\n", strerror(errno));
        cleanup(1, true);
    }

    if (opt.iproto == PROTOCOL_IPv6) {
        ret = setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)(&one), sizeof(one));
        if (ret < 0) {
            journal("Unable to set IPv4 compatibility option: %s.\n", strerror(errno));
            cleanup(1, true);
        }
    }

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)(&one), sizeof(one));
    if (ret < 0) {
        journal("Unable to set the socket to allow address reuse: %s.\n", strerror(errno));
    }

    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(opt.port);

    ret = bind(sockfd, (const struct sockaddr *)(&serv_addr), sizeof(struct sockaddr_in6));
    if (ret < 0) {
        journal("Unable to bind to socket: %s.\n", strerror(errno));
        cleanup(1, true);
    }
}

static bool tcp_accept_connection()
{
    struct sockaddr_in cli_addr;
    socklen_t cli_len;
    int consockfd, ret;

    journal("Listening for connection...\n");
    listen(sockfd, CONNECTION_BACKLOG);

    cli_len = sizeof(cli_addr);
    consockfd = accept(sockfd, (struct sockaddr *)(&cli_addr), &cli_len);
    if (consockfd < 0) {
        int errno_ = errno;
        journal("Unable to accept connection: %s.\n", strerror(errno));
        errno = errno_;
        check_errno();
        return false;
    }

    ret = tcp_send_quote(consockfd, &opt);
    if (ret < 0) {
        /* Error message is printed by tcp_send_quote */
        return false;
    }

    ret = close(consockfd);
    if (ret < 0) {
        journal("Unable to close connection: %s.\n", strerror(errno));
        return false;
    }

    return true;
}

static bool udp_accept_connection()
{
    struct sockaddr_in cli_addr;
    socklen_t cli_len;
    int ret;

    journal("Listening for connection...\n");
    ret = recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *)(&cli_addr), &cli_len);
    if (ret < 0) {
        int errno_ = errno;
        journal("Unable to write to socket: %s.\n", strerror(errno));
        errno = errno_;
        check_errno();
        return false;
    }

    cli_len = sizeof(cli_addr);
    ret = udp_send_quote(sockfd, (struct sockaddr *)(&cli_addr), cli_len, &opt);
    if (ret < 0) {
        /* Error message is printed by udp_send_quote */
        return false;
    }

    return true;
}

void cleanup(int ret, bool quiet)
{
    int ret2;

    if (!quiet) {
        ret2 = journal("Quitting with exit code %d.\n", ret);

        if (ret2 < 0) {
            ret++;
        }
    }

    if (sockfd >= 0) {
        ret2 = close(sockfd);
        if (ret2 < 0) {
            ret2 = journal("Unable to close socket file descriptor %d: %s.\n",
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
            ret2 = journal("Unable to remove pid file (%s): %s.\n",
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
    journal("Loading configuration settings...\n");
    parse_args(&opt, args.argc, args.argv);
    check_config();
    /* TODO rebind connection if needed */
}

static void check_errno()
{
    switch (errno) {
        case EBADF:
        case EFAULT:
        case EINVAL:
        case EMFILE:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
        case EPROTO:
        case EPERM:
        case ENOSR:
        case ESOCKTNOSUPPORT:
        case EPROTONOSUPPORT:
            cleanup(1, true);
    }
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

    if (STREQUALS(opt.pidfile, "none")) {
        journal("No pidfile was written at the request of the user.\n");
        return;
    }

    /* Check if the pidfile already exists */
    ret = stat(opt.pidfile, &statbuf);

    if (ret == 0) {
        journal("The pid file already exists. Quitting.\n");
        cleanup(1, true);
    }

    /* Write the pidfile */
    fh = fopen(opt.pidfile, "w+");

    if (fh == NULL) {
        journal("Unable to open pid file: %s.\n", strerror(errno));

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
        journal("Unable to close pid file handle: %s.\n", strerror(errno));

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
        journal("Only root can bind to ports below 1024.\n");
        cleanup(1, true);
    }

    if (opt.pidfile[0] != '/') {
        journal("Specified pid file is not an absolute path.\n");
        cleanup(1, true);
    }

    ret = stat(opt.quotesfile, &statbuf);

    if (ret < 0) {
        journal("Unable to stat quotes file: %s.\n");
        cleanup(1, true);
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

