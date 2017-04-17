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

#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "config.h"
#include "daemon.h"
#include "journal.h"
#include "network.h"
#include "pidfile.h"
#include "quotes.h"
#include "security.h"
#include "signal_hndl.h"

static struct options opt;

static void load_config(const int argc,
			const char *const argv[])
{
	parse_args(&opt, argc, argv);
	check_config(&opt);

	if (open_quotes_file(&opt)) {
		journal("Unable to open quotes file: %s.\n", strerror(errno));
		cleanup(EXIT_IO, 1);
	}
}

static int main_loop(void)
{
	void (*accept_connection)(void);

	pidfile_create(&opt);

	switch (opt.iproto) {
	case PROTOCOL_BOTH:
	case PROTOCOL_IPv6:
		set_up_ipv6_socket(&opt);
		break;
	case PROTOCOL_IPv4:
		set_up_ipv4_socket(&opt);
		break;
	default:
		journal("Internal error: invalid enum value for \"iproto\": %d.\n",
			opt.iproto);
		cleanup(EXIT_INTERNAL, 1);
	}

	if (opt.drop_privileges)
		drop_privileges();

	switch (opt.tproto) {
	case PROTOCOL_TCP:
		accept_connection = tcp_accept_connection;
		break;
	case PROTOCOL_UDP:
		accept_connection = udp_accept_connection;
		break;
	default:
		ERR_TRACE();
		journal("Internal error: invalid enum value for \"tproto\": %d.\n",
			opt.tproto);
		cleanup(EXIT_INTERNAL, 1);
		return -1;
	}

	for (;;)
		accept_connection();
}

static int daemonize(void)
{
	pid_t pid;

	/* Fork to background */
	if ((pid = fork()) < 0) {
		journal("Unable to fork: %s.\n", strerror(errno));
		cleanup(EXIT_FAILURE, 1);
	} else if (pid) {
		/* If we're the parent, then quit */
		journal("Successfully created background daemon, pid %d.\n", pid);
		cleanup(EXIT_SUCCESS, 1);
	}

	if (setsid() < 0) {
		journal("Unable to create new session: %s.\n", strerror(errno));
		cleanup(EXIT_FAILURE, 1);
	}
	if (opt.chdir_root) {
		if (chdir("/")) {
			journal("Unable to chdir to root dir: %s.\n",
				strerror(errno));
		}
	}
	return main_loop();
}

int main(const int argc, const char *const argv[])
{
#if DEBUG
	printf("(Running in debug mode)\n");
#endif /* DEBUG */

	signal_hndl_init();
	load_config(argc, argv);
	open_journal(opt.journal_file);

	/* Check security settings */
	if (opt.strict)
		security_options_check(&opt);

	return opt.daemonize ? daemonize() : main_loop();
}

void cleanup(int ret, int quiet)
{
	if (!quiet)
		journal("Quitting with exit code %d.\n", ret);
	pidfile_remove(&opt);

	destroy_quote_buffers();
	close_socket();
	close_quotes_file();
	close_journal();

	exit(ret);
}
