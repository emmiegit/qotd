/*
 * sighandler.c
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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "daemon.h"
#include "journal.h"
#include "signal_handler.h"

#define JOURNAL(x)		(journal_is_open() ? journal(x) : fprintf(stderr, (x)))

/* Static declarations */
static void handle_signal(const int signum);

/* Function implementations */
void set_up_handlers()
{
	signal(SIGSEGV, handle_signal);
	signal(SIGTERM, handle_signal);
	signal(SIGINT,  handle_signal);
	signal(SIGHUP,  handle_signal);
}

static void handle_signal(const int signum)
{
	switch (signum) {
	case SIGSEGV:
		journal("Error: segmentation fault. Dumping core (if enabled).\n");
		cleanup(EXIT_INTERNAL, true);
		break;
	case SIGTERM:
		journal("Termination signal received. Exiting...\n");
		cleanup(EXIT_SUCCESS, true);
		break;
	case SIGINT:
		journal("Interrupt signal received. Exiting...\n");
		cleanup(EXIT_SIGNAL, true);
		break;
	case SIGCHLD:
		journal("My child died. Doing nothing.\n");
		break;
	case SIGHUP:
		journal("Hangup recieved. Doing nothing.\n");
	}
}

