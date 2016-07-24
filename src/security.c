/*
 * security.c
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "daemon.h"
#include "journal.h"
#include "security.h"
#include "standard.h"

void drop_privileges()
{
	int ret;

	if (geteuid() != ROOT_USER_ID) {
		journal("Not root, no privileges to drop.\n");
		return;
	}

	journal("Connected to socket, dropping privileges.\n");

	/* POSIX specifies that the group should be dropped first */
	ret = setgid(DAEMON_GROUP_ID);
	if (unlikely(ret < 0)) {
		journal("Unable to set effective group id to %d: %s.\n", DAEMON_GROUP_ID, strerror(errno));
	}

	ret = setuid(DAEMON_USER_ID);
	if (unlikely(ret < 0)) {
		journal("Unable to set effective user id to %d: %s.\n", DAEMON_USER_ID, strerror(errno));
	}
}

void regain_privileges()
{
	int ret;

	journal("Regaining privileges to reconnect to socket.\n");

	/* Regain privileges */
	ret = setuid(getuid());

	if (ret < 0) {
		journal("Unable to set effective user ID to %d: %s.\n",
				getuid(), strerror(errno));
		return;
	}
}

void file_check(const char *path, const char *file_type)
{
	struct stat statbuf;
	int ret;

	ret = stat(path, &statbuf);
	if (ret < 0) {
		fprintf(stderr, "Unable to open %s file \"%s\": %s.\n",
				file_type, path, strerror(errno));
		cleanup(1, true);
	}

	/* Check ownership of the file */
	if (statbuf.st_uid != geteuid() && statbuf.st_uid != ROOT_USER_ID) {
		fprintf(stderr,
				"Your %s file is not owned by the calling user or root. The daemon will not start.\n"
				"(To disable this behavior, use the --lax flag when running).\n",
				file_type);
		cleanup(1, true);
	}

	/* Check file write permissions */
	if ((statbuf.st_mode & S_IWGRP) || (statbuf.st_mode & S_IWOTH)) {
		fprintf(stderr,
				"Your %s file is writable by those who are not the owner or root.\n"
				"The daemon will not start.\n"
				"(To disable this behavior, use the --lax flag when running).\n",
				file_type);
		cleanup(1, true);
	}
}

