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
#include <stdlib.h>
#include <string.h>

#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "daemon.h"
#include "journal.h"
#include "security.h"
#include "standard.h"

void drop_privileges(void)
{
	int ret;

	if (geteuid() != ROOT_USER_ID) {
		journal("Not running as root, no privileges to drop.\n");
		return;
	}

	journal("Everything is ready, dropping privileges.\n");

	/* POSIX specifies that the group should be dropped first */
	ret = setgid(DAEMON_GROUP_ID);
	if (unlikely(ret)) {
		journal("Unable to set effective group id to %d: %s.\n", DAEMON_GROUP_ID, strerror(errno));
	}

	ret = setuid(DAEMON_USER_ID);
	if (unlikely(ret)) {
		journal("Unable to set effective user id to %d: %s.\n", DAEMON_USER_ID, strerror(errno));
	}

	if (unlikely(!setuid(ROOT_USER_ID))) {
		journal("Managed to regain root privileges. Bailing out.\n");
		cleanup(EXIT_SECURITY, true);
	}
}

void security_options_check(const struct options *opt)
{
	char *pidpath, *piddir;
	struct stat statbuf;
	int ret;

	if (!opt->pidfile) {
		return;
	}

	pidpath = strdup(opt->pidfile);
	piddir = dirname(pidpath);
	free(pidpath);

	ret = stat(piddir, &statbuf);
	if (ret) {
		fprintf(stderr, "Unable to stat \"%s\" (the directory that will contain the pidfile): %s.\n",
			piddir, strerror(errno));
		cleanup(EXIT_IO, true);
	}

	if (!S_ISDIR(statbuf.st_mode)) {
		fprintf(stderr, "\"%s\" is meant to hold the pidfile, but it's not a directory.\n",
			piddir);
		cleanup(EXIT_IO, true);
	}

	if ((statbuf.st_mode & S_IWOTH) && !(statbuf.st_mode & S_ISVTX)) {
		fprintf(stderr,
			"\"%s\" (the directory that will contain the pidfile) potentially allows others\n"
			"to delete our pidfile. The daemon will not start.\n"
			"(To disable this behavior, use the --lax flag when running).\n",
			piddir);
		cleanup(EXIT_SECURITY, true);
	}
}

void security_file_check(const char *path, const char *file_type)
{
	struct stat statbuf;
	int ret;

	ret = stat(path, &statbuf);
	if (ret < 0) {
		fprintf(stderr, "Unable to open %s file \"%s\": %s.\n",
			file_type, path, strerror(errno));
		cleanup(EXIT_IO, true);
	}

	/* Check ownership of the file */
	if (statbuf.st_uid != geteuid() && statbuf.st_uid != ROOT_USER_ID) {
		fprintf(stderr,
			"Your %s file is not owned by the calling user or root. The daemon will not start.\n"
			"(To disable this behavior, use the --lax flag when running).\n",
			file_type);
		cleanup(EXIT_SECURITY, true);
	}

	/* Check file write permissions */
	if ((statbuf.st_mode & S_IWGRP) || (statbuf.st_mode & S_IWOTH)) {
		fprintf(stderr,
			"Your %s file is writable by those who are not the owner or root.\n"
			"The daemon will not start.\n"
			"(To disable this behavior, use the --lax flag when running).\n",
			file_type);
		cleanup(EXIT_SECURITY, true);
	}
}

