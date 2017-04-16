/*
 * journal.c
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "journal.h"
#include "main.h"

/* Static variables */
static FILE *journal_fh = NULL;

/* Static function declarations */
static void open_journal_fd(int fd);

void open_journal(const char *const path)
{
	if (!path) {
		open_journal_fd(STDOUT_FILENO);
		return;
	}

#if DEBUG
	printf("Setting journal to be \"%s\".\n", path);
#endif /* DEBUG */

	journal_fh = fopen(path, "w");
	if (!journal_fh) {
		fprintf(stderr, "Unable to open journal handle for \"%s\": %s.\n",
			path, strerror(errno));
		cleanup(EXIT_IO, true);
	}
}

static void open_journal_fd(const int fd)
{
#if DEBUG
	printf("Setting journal to use file descriptor %d.\n", fd);
#endif /* DEBUG */

	journal_fh = fdopen(fd, "w");
	if (!journal_fh) {
		fprintf(stderr, "Unable to open journal handle for file descriptor %d: %s.\n",
			fd, strerror(errno));
		cleanup(EXIT_IO, true);
	}
}

int close_journal(void)
{
	if (journal_fh) {
		int ret = fclose(journal_fh);
		if (ret < 0) {
			fprintf(stderr, "Unable to close journal handle: %s.\n",
					strerror(errno));
		}

		return ret;
	}

	return 0;
}

bool journal_is_open(void)
{
	return journal_fh != NULL;
}

int journal(const char *const format, ...)
{
	va_list args;
	int ret;

	if (!journal_fh) {
		return 0;
	}

	va_start(args, format);
	ret = vfprintf(journal_fh, format, args);
	va_end(args);
	return ret;
}

