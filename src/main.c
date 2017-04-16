/*
 * main.c
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
#include <stdlib.h>

#include "config.h"
#include "daemon.h"
#include "main.h"
#include "journal.h"
#include "pidfile.h"
#include "signal_handler.h"

static int main_loop(void)
{
	void (*accept_connection)(void);

	pidfile_create(opt);

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
		drop_privileges();
	}

	switch (opt.tproto) {
	case PROTOCOL_TCP:
		accept_connection = &tcp_accept_connection;
		break;
	case PROTOCOL_UDP:
		accept_connection = &udp_accept_connection;
		break;
	default:
		ERR_TRACE();
		journal("Internal error: invalid enum value for \"tproto\": %d.\n", opt.tproto);
		cleanup(EXIT_INTERNAL, true);
		return -1;
	}

	while (true) {
		accept_connection();
	}

	return EXIT_SUCCESS;
}

int main(const int argc, const char *const argv[])
{
#if DEBUG
	printf("(Running in debug mode)\n");
#endif /* DEBUG */

	set_up_handlers();
	load_config(argc, argv);
	open_journal(opt.journal_file);

	/* Check security settings */
	if (opt.strict)
		security_options_check(&opt);

	return opt.daemonize ? daemonize() : main_loop();
}

void cleanup(int ret, int quiet)
{
	if (!quiet) {
		journal("Quitting with exit code %d.\n", retcode);
	}

	if (wrote_pidfile) {
		if (unlink(opt.pid_file)) {
			journal("Unable to remove pid file (%s): %s.\n",
				opt.pid_file, strerror(errno));
		}
	}

	destroy_quote_buffers();
	close_socket();
	close_quotes_file();
	close_journal();

	exit(ret);
}
