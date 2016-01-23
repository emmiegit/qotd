/*
 * sighandler.c
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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "qotdd.h"
#include "sighandler.h"

#define UNUSED(x) ((void)(x))

static void handle_segv(int signum);
static void handle_term(int signum);
static void handle_int(int signum);
static void handle_hup(int signum);

void set_up_handlers()
{
    signal(SIGSEGV, handle_segv);
    signal(SIGTERM, handle_term);
    signal(SIGINT,  handle_int);
    signal(SIGHUP,  handle_hup);
}

static void handle_segv(int signum)
{
    fprintf(stderr, "Error: segmentation fault. Dumping core.\n");
    cleanup(signum);
}

static void handle_term(int signum)
{
    fprintf(stderr, "Termination signal received. Exiting...\n");
    cleanup(signum);
}

static void handle_int(int signum)
{
    fprintf(stderr, "Interrupt signal received. Exiting...\n");
    cleanup(signum);
}


static void handle_hup(int signum)
{
    printf("Hangup recieved. Restarting daemon...\n");
    printf("TODO: actually restart daemon.\n");
    UNUSED(signum);
}

