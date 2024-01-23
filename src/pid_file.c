/*
 * pid_file.c
 *
 * qotd - A simple QOTD daemon.
 * Copyright (c) 2015-2016 Emmie Smith
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "daemon.h"
#include "journal.h"
#include "pid_file.h"

static unsigned char wrote_pidfile;

void pidfile_create(const struct options *opt)
{
	FILE *fh;

	if (!opt->pid_file) {
		journal("No pidfile was written.\n");
		return;
	}

	/* Check if the pidfile already exists */
	if (access(opt->pid_file, F_OK)) {
		if (errno != ENOENT) {
			journal("Unable to access pid file \"%s\": %s.\n",
				opt->pid_file, strerror(errno));
			cleanup(EXIT_IO, 1);
		}
	} else {
		journal("The pid file already exists. Quitting.\n");
		cleanup(EXIT_FAILURE, 1);
	}

	/* Write the pidfile */
	fh = fopen(opt->pid_file, "w+");
	if (!fh) {
		journal("Unable to open pid file: %s.\n", strerror(errno));

		if (opt->require_pidfile) {
			cleanup(EXIT_IO, 1);
		} else {
			return;
		}
	}
	if (fprintf(fh, "%d\n", getpid()) < 0) {
		JTRACE();
		perror("Unable to write process id to pid file");

		if (opt->require_pidfile) {
			cleanup(EXIT_IO, 1);
		}
	}
	wrote_pidfile = 1;
	fclose(fh);
}

void pidfile_remove(const struct options *opt)
{
	if (!opt->pid_file) {
		return;
	}

	if (access(opt->pid_file, F_OK)) {
		journal("Pid file \"%s\" is inaccessible: %s.\n",
			opt->pid_file, strerror(errno));
	}
	if (unlink(opt->pid_file)) {
		journal("Unable to unlink \"%s\": %s.\n",
			opt->pid_file, strerror(errno));
	}
}
