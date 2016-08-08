/*
 * options.h
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

#ifndef __OPTIONS_H
# define __OPTIONS_H

# include <stdbool.h>

# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

enum quote_divider {
	DIV_EVERYLINE,
	DIV_PERCENT,
	DIV_WHOLEFILE
};

enum transport_protocol {
	PROTOCOL_TCP,
	PROTOCOL_UDP,
	PROTOCOL_TNONE
};

enum internet_protocol {
	PROTOCOL_IPv4,
	PROTOCOL_IPv6,
	PROTOCOL_BOTH,
	PROTOCOL_INONE
};

# define DEFAULT_CONFIG_FILE		"/etc/qotd.conf"
# define DEFAULT_DAEMONIZE		true
# define DEFAULT_TRANSPORT_PROTOCOL	PROTOCOL_TCP
# define DEFAULT_INTERNET_PROTOCOL	PROTOCOL_BOTH
# define DEFAULT_PORT			17
# define DEFAULT_DROP_PRIVILEGES	true
# define DEFAULT_REQUIRE_PIDFILE	true
# define DEFAULT_QUOTES_FILE		"/usr/share/qotd/quotes.txt"
# define DEFAULT_LINE_DIVIDER		DIV_PERCENT
# define DEFAULT_PAD_QUOTES		true
# define DEFAULT_IS_DAILY		true
# define DEFAULT_ALLOW_BIG		false
# define DEFAULT_CHDIR_ROOT		true

struct options {
	const char *quotes_file;		/* string containing path to quotes file */
	const char *pid_file;			/* string containing path to pid file */
	unsigned int port;			/* what port to listen on */
	enum quote_divider linediv;	 	/* how to read the quotes file */
	enum transport_protocol tproto; 	/* which transport protocol to use */
	enum internet_protocol iproto;  	/* which internet protocol to use */

	bool daemonize;				/* whether to fork to the background or not */
	bool require_pidfile;			/* whether to quit if the pidfile cannot be made */
	bool strict;				/* enables extra checks to ensure security */
	bool drop_privileges;			/* whether to setuid() after opening the connection */
	bool is_daily;				/* whether quotes are random every day or every visit */
	bool pad_quotes;			/* whether to pad the quote with newlines */
	bool allow_big;				/* ignore 512-byte limit */
	bool chdir_root;			/* whether to chdir to / when running */
};

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __OPTIONS_H */

