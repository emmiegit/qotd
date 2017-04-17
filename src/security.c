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

#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <libgen.h>
#include <unistd.h>

#include "core.h"
#include "daemon.h"
#include "journal.h"
#include "security.h"

static gid_t get_daemon_group(void)
{
	const struct group *grp;

	grp = getgrnam(DAEMON_GROUP_NAME);
	if (!grp) {
		journal("Unable to find daemon group id: %s.\n", strerror(errno));
		return (gid_t)-1;
	}
	return grp->gr_gid;
}

void drop_privileges(void)
{
	gid_t group;
	int ret;

	if (geteuid() != ROOT_USER_ID) {
		journal("Not running as root, no privileges to drop.\n");
		return;
	}
	group = get_daemon_group();
	if (unlikely((int)group < 0)) {
		journal("Unable to drop drop privileges.\n");
		return;
	}

	journal("Everything is ready, dropping privileges.\n");

	/* POSIX specifies that the group should be dropped first */
	ret = setgid(group);
	if (unlikely(ret)) {
		journal("Unable to set effective group id to %d: %s.\n", group, strerror(errno));
	}

	ret = setuid(group);
	if (unlikely(ret)) {
		journal("Unable to set effective user id to %d: %s.\n", group, strerror(errno));
	}

	if (unlikely(!setuid(ROOT_USER_ID))) {
		journal("Managed to regain root privileges. Bailing out.\n");
		cleanup(EXIT_SECURITY, 1);
	}
}

void security_options_check(const struct options *const opt)
{
	char *pidpath, *piddir;
	struct stat statbuf;
	int ret;

	if (!opt->pid_file) {
		return;
	}

	journal("Checking options...\n");
	pidpath = strdup(opt->pid_file);
	piddir = dirname(pidpath);

	ret = stat(piddir, &statbuf);
	if (ret) {
		journal("Unable to stat \"%s\" (the directory that will contain the pidfile): %s.\n",
			piddir, strerror(errno));
		cleanup(EXIT_IO, 1);
	}

	if (!S_ISDIR(statbuf.st_mode)) {
		journal("\"%s\" is meant to hold the pidfile, but it's not a directory.\n",
			piddir);
		cleanup(EXIT_IO, 1);
	}

	if ((statbuf.st_mode & S_IWOTH) && !(statbuf.st_mode & S_ISVTX)) {
		journal("\"%s\" (the directory that will contain the pidfile) potentially allows others\n"
			"to delete our pidfile. The daemon will not start.\n"
			"(To disable this behavior, use the --lax flag when running).\n",
			piddir);
		cleanup(EXIT_SECURITY, 1);
	}

	free(pidpath);
}

void security_file_check(const char *const path, const char *const file_type)
{
	struct stat statbuf;
	int ret;

	journal("Checking %s file \"%s\"...\n", file_type, path);
	ret = stat(path, &statbuf);
	if (ret < 0) {
		journal("Unable to open %s file \"%s\": %s.\n",
			file_type, path, strerror(errno));
		cleanup(EXIT_IO, 1);
	}

	/* Check ownership of the file */
	if (statbuf.st_uid != geteuid() && statbuf.st_uid != ROOT_USER_ID) {
		journal("Your %s file is not owned by the calling user or root. The daemon will not start.\n"
			"(To disable this behavior, use the --lax flag when running).\n",
			file_type);
		cleanup(EXIT_SECURITY, 1);
	}

	/* Check file write permissions */
	if ((statbuf.st_mode & S_IWGRP) || (statbuf.st_mode & S_IWOTH)) {
		journal("Your %s file is writable by those who are not the owner or root.\n"
			"The daemon will not start.\n"
			"(To disable this behavior, use the --lax flag when running).\n",
			file_type);
		cleanup(EXIT_SECURITY, 1);
	}
}
