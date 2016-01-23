/*
 * qotdd.c
 *
 * qotd - A simple QOTD server.
 * Copyright (c) 2015 Ammon Smith
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
#include "sighandler.h"

#define ever (;;)
#define BUFFER_SIZE 512

static int send_quote();

static options* opt;
static int sockfd;

int main(int argc, const char* argv[])
{
    /* Set up signal handlers */
    set_up_handlers();

    /* Parse command line arguments */
    opt = parse_args(argc, argv);

    /* Check configuration */
    if (opt->port < 1024 && geteuid() != 0) {
        fprintf(stderr, "Only root can bind to ports below 1024.\n");
        cleanup(1);
    }

    /* Open and bind to a socket */
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

        ret = send_quote();
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

static int send_quote(int fd)
{
    char buffer[] = "Quote of the day: something\n";
    int bufsize = 28;

    return write(fd, buffer, bufsize);
}

void cleanup(const int ret)
{
    if (close(sockfd) < 0) {
        perror("Unable to close socket file descriptor");
    }

    if (opt) {
        free(opt);
    }

    exit(ret);
}

