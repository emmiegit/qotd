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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "arguments.h"
#include "info.h"
#include "qotdd.h"
#include "quotes.h"
#include "sighandler.h"

#define ever (;;)

static int daemonize();
static void save_args(const int argc, const char* argv[]);
static void check_config();
static void write_pidfile();
static void setup_socket(struct sockaddr_in* serv_addr);
static bool accept_connection();

static arguments* args;
static options* opt;
static int sockfd;
static bool wrote_pidfile;

int main(int argc, const char* argv[])
{
    /* Set up signal handlers */
    set_up_handlers();

    /* Make a static copy of the arguments */
    save_args(argc, argv);

    /* Load configuration */
    load_config();

    /* Check configuration */
    check_config();

    /* Set up socket */
    struct sockaddr_in serv_addr;
    setup_socket(&serv_addr);

    /* Fork to the background and quit */
    return daemonize();
}

static int daemonize()
{
    pid_t pid;
    if ((pid = fork()) < 0) {
        cleanup(1);
        return 1;
    } else if (pid != 0) {
        /* If you're the parent, then quit */
        printf("Successfully created background daemon.\n");
        cleanup(0);
        return 0;
    }

    /* Set up daemon environment */
    pid = setsid();

    if (pid < 0) {
        perror("Unable to create new session");
        cleanup(1);
        return 1;
    }

    chdir("/");
    write_pidfile();

    /* Listen to the specified port */
    for ever {
        accept_connection();
    }

    return EXIT_SUCCESS;
}

void cleanup(int ret)
{
    printf("Quitting with exit code %d.\n", ret);

    int ret2 = close(sockfd);
    if (ret2 < 0) {
        perror("Unable to close socket file descriptor");
        ret++;
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
    if (opt) {
        free(opt);
    }

    opt = parse_args(args->argc, args->argv);
}

static void save_args(const int argc, const char* argv[])
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
    if (opt->pidfile[0] != '/') {
        fprintf(stderr, "Specified pid file is not an absolute path.\n");
        cleanup(1);
    }

    FILE* fh = fopen(opt->pidfile, "w+");
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

    if (access(opt->quotesfile, F_OK) == -1) {
        fprintf(stderr, "Specified quotes file \"%s\" does not exist or is inaccessible.\n", opt->quotesfile);
        cleanup(1);
    }
}

static void setup_socket(struct sockaddr_in* serv_addr)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Unable to connect to socket");
        cleanup(1);
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = INADDR_ANY;
    serv_addr->sin_port = htons(opt->port);

    int ret = bind(sockfd, (const struct sockaddr*)serv_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        perror("Unable to bind to socket");
        cleanup(1);
    }
}

static bool accept_connection()
{
    listen(sockfd, 10);

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int consockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
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

