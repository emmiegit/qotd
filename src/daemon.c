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
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "arguments.h"
#include "daemon.h"
#include "info.h"
#include "journal.h"
#include "network.h"
#include "quotes.h"
#include "security.h"
#include "signal_handler.h"
#include "standard.h"

/* Static function declarations */
static int daemonize();
static int main_loop();
static void save_args(const int argc, const char *argv[]);
static void check_config();
static void write_pidfile();

/* Static member declarations */
static struct options opt;
static bool wrote_pidfile;

static struct {
	int argc;
	const char **argv;
} arguments;

/* Function implementations */
int main(int argc, const char *argv[])
{
	/* Set up signal handlers */
	set_up_handlers();

#if DEBUG
	printf("(Running in debug mode)\n");
#endif /* DEBUG */

	/* Set default values for static variables */
	wrote_pidfile = false;

	/* Make a copy of the arguments */
	save_args(argc, argv);

	/* Load configuration and open journal */
	load_config(true);

	return opt.daemonize ? daemonize() : main_loop();
}

static int daemonize()
{
	pid_t pid;

	/* Fork to background */
	if ((pid = fork()) < 0) {
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

		if (ret < 0) {
			journal("Unable to chdir to root dir: %s.\n", strerror(errno));
		}
	}

	return main_loop();
}

static int main_loop()
{
	bool (*accept_connection)(const struct options *opt);

	write_pidfile();

	switch (opt.iproto) {
		case PROTOCOL_BOTH:
		case PROTOCOL_IPv6:
			set_up_ipv6_socket(&opt);
			break;
		case PROTOCOL_IPv4:
			set_up_ipv4_socket(&opt);
			break;
		default:
			journal("Internal error: invalid enum value for \"iproto\": %d.\n", opt.iproto);
			cleanup(EXIT_INTERNAL, true);
	}

	if (opt.drop_privileges) {
		drop_privileges(&opt);
	}

	switch (opt.tproto) {
		case PROTOCOL_TCP:
			accept_connection = &tcp_accept_connection;
			break;
		case PROTOCOL_UDP:
			accept_connection = &udp_accept_connection;
			break;
		default:
			journal("Internal error: invalid enum value for \"tproto\": %d.\n", opt.tproto);
			cleanup(EXIT_INTERNAL, true);
			return -1;
	}

	for (;;) {
		accept_connection(&opt);
	}

	return EXIT_SUCCESS;
}

void cleanup(int retcode, bool quiet)
{
	int ret;

	if (!quiet) {
		journal("Quitting with exit code %d.\n", retcode);
	}

	if (wrote_pidfile) {
		ret = unlink(opt.pidfile);
		if (ret) {
			journal("Unable to remove pid file (%s): %s.\n",
				opt.pidfile, strerror(errno));
		}
	}

	close_socket();
	close_journal();
	exit(retcode);
}

void load_config(bool first_time)
{
	struct options old_opt = opt;
	int ret;

	if (!first_time) {
		journal("Reloading configuration settings...\n");
	}

	parse_args(&opt, arguments.argc, arguments.argv);
	check_config();

	if (first_time) {
		ret = open_quotes_file(opt.quotesfile);
		if (ret) {
			journal("Unable to open quotes file: %s.\n", strerror(errno));
			cleanup(EXIT_IO, true);
		}
		return;
	}

	/* not finished yet */
	if (opt.quotesfile != old_opt.quotesfile &&
		strcmp(opt.quotesfile, old_opt.quotesfile)) {

		journal("Quotes file changed, opening new file handle...\n");

			/* Close old quotes file */
			ret = close_quotes_file();
			if (ret) {
				journal("Unable to close quotes file: %s.\n", strerror(errno));
				cleanup(EXIT_IO, true);
			}

			/* Reopen quotes file */
			ret = open_quotes_file(opt.quotesfile);
			if (ret) {
				journal("Unable to reopen quotes file: %s.\n", strerror(errno));
				cleanup(EXIT_IO, true);
			}
	}

	if (!opt.pidfile || !old_opt.pidfile ||
		(opt.pidfile != old_opt.pidfile && strcmp(opt.pidfile, old_opt.pidfile))) {
		journal("Pid file changed, switching them out...\n");

		ret = unlink(old_opt.pidfile);
		if (ret) {
			ERR_TRACE();
			journal("Unable to remove old pidfile: %s.\n", strerror(errno));
			cleanup(EXIT_IO, true);
		}

		write_pidfile();
	}

	if (opt.port != old_opt.port ||
		opt.tproto != old_opt.tproto ||
		opt.iproto != old_opt.iproto) {

		journal("Connection settings changed, remaking socket...\n");

		if (opt.drop_privileges && getuid() == ROOT_USER_ID) {
			/* Get root access back */
			regain_privileges();
		}

		/* Close old socket */
		close_socket();

		/* Create new socket */
		/* TODO */

		if (opt.drop_privileges) {
			/* Drop privileges again */
			drop_privileges(&opt);
		}
	}
}

static void save_args(int argc, const char *argv[])
{
	arguments.argc = argc;
	arguments.argv = argv;
}

static void write_pidfile()
{
	struct stat statbuf;
	int ret;
	FILE *fh;

	if (opt.pidfile == NULL) {
		journal("No pidfile was written at the request of the user.\n");
		return;
	}

	/* Check if the pidfile already exists */
	ret = stat(opt.pidfile, &statbuf);
	if (ret) {
		if (errno != ENOENT) {
			journal("Unable to stat pid file \"%s\": %s.\n", opt.pidfile, strerror(errno));
			cleanup(EXIT_IO, true);
		}
	} else {
		journal("The pid file already exists. Quitting.\n");
		cleanup(EXIT_FAILURE, true);
	}

	/* Write the pidfile */
	fh = fopen(opt.pidfile, "w+");

	if (fh == NULL) {
		journal("Unable to open pid file: %s.\n", strerror(errno));

		if (opt.require_pidfile) {
			cleanup(EXIT_IO, true);
		} else {
			return;
		}
	}

	ret = fprintf(fh, "%d\n", getpid());
	if (ret < 0) {
		perror("Unable to write pid to pid file");

		if (opt.require_pidfile) {
			fclose(fh);
			cleanup(EXIT_IO, true);
		}
	}

	wrote_pidfile = true;

	ret = fclose(fh);
	if (ret < 0) {
		journal("Unable to close pid file handle: %s.\n", strerror(errno));

		if (opt.require_pidfile) {
			cleanup(EXIT_IO, true);
		}
	}
}

static void check_config()
{
	struct stat statbuf;
	int ret;

	if (opt.port < 1024 && geteuid() != ROOT_USER_ID) {
		journal("Only root can bind to ports below 1024.\n");
		cleanup(EXIT_ARGUMENTS, true);
	}

	if (opt.pidfile[0] != '/') {
		journal("Specified pid file is not an absolute path.\n");
		cleanup(EXIT_ARGUMENTS, true);
	}

	ret = stat(opt.quotesfile, &statbuf);

	if (ret < 0) {
		journal("Unable to stat quotes file \"%s\": %s.\n", opt.quotesfile, strerror(errno));
		cleanup(EXIT_IO, true);
	}
}

