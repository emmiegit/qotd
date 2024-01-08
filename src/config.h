/*
 * config.h
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

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
# define DEFAULT_DAEMONIZE		1
# define DEFAULT_TRANSPORT_PROTOCOL	PROTOCOL_TCP
# define DEFAULT_INTERNET_PROTOCOL	PROTOCOL_BOTH
# define DEFAULT_PORT			17
# define DEFAULT_DROP_PRIVILEGES	1
# define DEFAULT_REQUIRE_PIDFILE	1
# define DEFAULT_JOURNAL_FILE		NULL /* means "use stdout" */
# define DEFAULT_QUOTES_FILE		"/usr/share/qotd/quotes.txt"
# define DEFAULT_LINE_DIVIDER		DIV_PERCENT
# define DEFAULT_PAD_QUOTES		1
# define DEFAULT_IS_DAILY		1
# define DEFAULT_ALLOW_BIG		0
# define DEFAULT_CHDIR_ROOT		1

struct options {
	const char *quotes_file;		/* string containing path to quotes file */
	const char *pid_file;			/* string containing path to pid file */
	const char *journal_file;		/* string containing path to journal file */
	unsigned int port;			/* what port to listen on */
	enum quote_divider linediv;	 	/* how to read the quotes file */
	enum transport_protocol tproto; 	/* which transport protocol to use */
	enum internet_protocol iproto;  	/* which internet protocol to use */

	unsigned daemonize		: 1;	/* whether to fork to the background or not */
	unsigned require_pidfile	: 1;	/* whether to quit if the pidfile cannot be made */
	unsigned strict			: 1;	/* enables extra checks to ensure security */
	unsigned drop_privileges	: 1;	/* whether to setuid() after opening the connection */
	unsigned is_daily		: 1;	/* whether quotes are random every day or every visit */
	unsigned pad_quotes		: 1;	/* whether to pad the quote with newlines */
	unsigned allow_big		: 1;	/* ignore 512-byte limit */
	unsigned chdir_root		: 1;	/* whether to chdir to / when running */
};

void parse_config(struct options *opt, const char *conf_file);
void check_config(const struct options *opt);

#endif /* _CONFIG_H_ */
