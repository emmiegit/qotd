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
#include "qotdd.h"
#include "quotes.h"
#include "sighandler.h"

#define ever (;;)

static void save_args(const int argc, const char* argv[]);
static void write_pidfile();

static arguments* args;
static options* opt;
static int sockfd;

int main(int argc, const char* argv[])
{
    /* Set up signal handlers */
    set_up_handlers();

    /* Make a global copy of arguments */
    save_args(argc, argv);

    /* Load configuration */
    load_config();

    /* Write to pid file */
    write_pidfile();

    send_quote(1, opt);
    cleanup(0);

    /* Check configuration */
    if (opt->port < 1024 && geteuid() != 0) {
        fprintf(stderr, "Only root can bind to ports below 1024.\n");
        cleanup(1);
    }

    /* Set up socket */
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int consockfd, ret;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Unable to connect to socket");
        cleanup(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(opt->port);

    /* Open and bind to a socket */
    ret = bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        perror("Unable to bind to socket");
        cleanup(1);
    }

    /* Continually accept connections */
    for ever {
        listen(sockfd, 10);

        clilen = sizeof(cli_addr);
        consockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
        if (consockfd < 0) {
            perror("Unable to accept connection");
            continue;
        }

        ret = send_quote(consockfd, opt);
        if (ret < 0) {
            perror("Unable to write to socket");
            continue;
        }

        ret = close(consockfd);
        if (ret < 0) {
            perror("Unable to close connection");
            continue;
        }
    }

    return EXIT_SUCCESS;
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

void load_config()
{
    if (opt) {
        free(opt);
    }

    opt = parse_args(args->argc, args->argv);
}

void cleanup(int ret)
{
    int ret2 = close(sockfd);
    if (ret2 < 0) {
        perror("Unable to close socket file descriptor");
        ret++;
    }

    if (args) {
        free(args);
    }

    ret2 = unlink(opt->pidfile);
    if (ret2 < 0) {
        fprintf(stderr, "Unable to remove pid file (%s): %s\n",
                opt->pidfile, strerror(errno));
        ret++;
    }

    if (opt) {
        free(opt->quotesfile);
        free(opt->pidfile);
        free(opt);
    }

    exit(ret);
}

static void write_pidfile()
{
    FILE* fh = fopen(opt->pidfile, "w+");
    int ret;

    if (fh == NULL) {
        perror("Unable to open pid file");
        return;
    }

    ret = fprintf(fh, "%d", getpid());
    if (ret < 0) {
        perror("Unable to write pid to pid file");
    }

    ret = fclose(fh);
    if (ret < 0) {
        perror("Unable to close pid file handle");
    }
}

