/*
 * signal_hndl.c
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

#include <signal.h>

#include <stdlib.h>
#include <stdio.h>

#include "daemon.h"
#include "journal.h"
#include "quotes.h"
#include "signal_hndl.h"

#define JOURNAL(x)				\
	do {					\
		if (journal_is_open())		\
			journal(x);		\
		else				\
			fputs((x), stderr);	\
	} while (0)

static void handle_signal(const int signum)
{
	switch (signum) {
	case SIGSEGV:
		JOURNAL("Error: segmentation fault. Dumping core (if enabled).\n");
		cleanup(EXIT_INTERNAL, 1);
		break;
	case SIGTERM:
		JOURNAL("Termination signal received. Exiting...\n");
		cleanup(EXIT_SUCCESS, 1);
		break;
	case SIGINT:
		JOURNAL("Interrupt signal received. Exiting...\n");
		cleanup(EXIT_SIGNAL, 1);
		break;
	case SIGHUP:
		JOURNAL("Hangup recieved. Loading new quotes...\n");
		if (reopen_quotes_file())
			JOURNAL("Error reopening quotes file!\n");
		break;
	case SIGCHLD:
		JOURNAL("My child died. Doing nothing.\n");
	}
}

void signal_hndl_init(void)
{
	signal(SIGSEGV, handle_signal);
	signal(SIGTERM, handle_signal);
	signal(SIGINT,  handle_signal);
	signal(SIGHUP,  handle_signal);
	signal(SIGCHLD, handle_signal);
}
