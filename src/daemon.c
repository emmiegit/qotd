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

#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "

/* Static function declarations */
static int daemonize(void);
static int main_loop(void);
static void write_pidfile(void);
static void load_config(const int argc, const char *const argv[]);
static void check_config(void);

/* Static member declarations */
static struct options opt;
static bool wrote_pidfile;

/* Function implementations */

int daemonize(void)
{
	pid_t pid;

	/* Fork to background */
	pid = fork();
	if (pid < 0) {
		journal("Unable to fork: %s.\n", strerror(errno));
		cleanup(EXIT_FAILURE, true);
	} else if (pid != 0) {
		/* If we're the parent, then quit */
		journal("Successfully created background daemon, pid %d.\n", pid);
		cleanup(EXIT_SUCCESS, true);
	}

	/* We're the child, so daemonize */
	pid = setsid();

	if (pid < 0) {
		journal("Unable to create new session: %s.\n", strerror(errno));
		cleanup(EXIT_FAILURE, true);
	}

	if (opt.chdir_root) {
		int ret = chdir("/");

		if (ret) {
			journal("Unable to chdir to root dir: %s.\n", strerror(errno));
		}
	}

	return main_loop();
}


static void load_config(const int argc, const char *const argv[])
{
	int ret;

	parse_args(&opt, argc, argv);
	check_config();

	ret = open_quotes_file(&opt);
	if (ret) {
		journal("Unable to open quotes file: %s.\n", strerror(errno));
		cleanup(EXIT_IO, true);
	}
}


