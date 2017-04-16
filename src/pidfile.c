/*
 * pidfile.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "journal.h"
#include "main.h"
#include "pidfile.h"

static int wrote_pidfile;

void pidfile_create(const struct options *opt)
{
	struct stat statbuf;
	int ret;
	FILE *fh;

	if (opt->pid_file) {
		journal("No pidfile was written.\n");
		return;
	}

	/* Check if the pidfile already exists */
	ret = stat(opt->pid_file, &statbuf);
	if (ret) {
		if (errno != ENOENT) {
			journal("Unable to stat pid file \"%s\": %s.\n",
				opt->pid_file, strerror(errno));
			cleanup(EXIT_IO, true);
		}
	} else {
		journal("The pid file already exists. Quitting.\n");
		cleanup(EXIT_FAILURE, true);
	}

	/* Write the pidfile */
	fh = fopen(opt->pid_file, "w+");

	if (!fh) {
		journal("Unable to open pid file: %s.\n", strerror(errno));

		if (opt->require_pidfile)
			cleanup(EXIT_IO, true);
		else
			return;
	}

	ret = fprintf(fh, "%d\n", getpid());
	if (ret < 0) {
		ERR_TRACE();
		perror("Unable to write process id to pid file");

		if (opt->require_pidfile) {
			fclose(fh);
			cleanup(EXIT_IO, true);
		}
	}

	wrote_pidfile = true;

	ret = fclose(fh);
	if (ret) {
		ERR_TRACE();
		journal("Unable to close pid file handle: %s.\n", strerror(errno));

		if (opt->require_pidfile)
			cleanup(EXIT_IO, true);
	}
}

