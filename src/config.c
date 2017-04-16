/*
 * configuration.c
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

#include <limits.h>
#include <strings.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "core.h"
#include "main.h"
#include "journal.h"
#include "security.h"

#define PORT_MAX		65535    /* Not in limits.h */
#define BUFFER_SIZE		PATH_MAX

#define NOT_BOOL(x)		((x) != (!!(x)))

struct buffer {
	char data[BUFFER_SIZE];
	size_t length;
};

struct string {
	const char *ptr;
	size_t length;
};

/* Utilities */

static int caseless_eq(const struct string *x,
		       const char *y,
		       size_t y_len)
{
	size_t i;
	char c1, c2;

	if (x->length != y_len)
		return 0;
	for (i = 0; i < y_len; i++) {
		c1 = tolower(x->ptr[i]);
		c2 = tolower(y[i]);
		if (c1 != c2)
			return 0;
	}
	return 1;
}

static void print_str(FILE *out, const struct string *s)
{
	size_t i;

	for (i = 0; i < s->length; i++)
		putc(s->ptr[i], out);
	putc('\n', out);
}

static char *dup_str(const struct string *s)
{
	char *buf;

	buf = malloc(s->length + 1);
	if (unlikely(!buf))
		return NULL;

	memcpy(buf, s->ptr, s->length);
	buf[s->length] = '\0';
	return buf;
}

/* I/O */

static int read_line(FILE *fh,
		     const char *filename,
		     unsigned int *lineno,
		     struct buffer *line)
{
	int ch;
	size_t i;

	for (i = 0; i < BUFFER_SIZE; i++) {
		ch = getc(fh);

		switch (ch) {
		case '\n':
			if (i) {
				line->data[i] = '\0';
				line->length = i;
				return 0;
			} else {
				/* Ignore empty lines */
				i--;
				(*lineno)++;
			}
			break;
		case EOF:
			if (i) {
				/*
				 * Return reset of line when EOF
				 * hits before newline
				 */
				line->data[i] = '\0';
				line->length = i;
				return 0;
			} else {
				/* EOF */
				return -1;
			}
			break;
		default:
			line->data[i] = (char)ch;
		}
	}

	/* Out of buffer space */
	fprintf(stderr, "%s:%u: Line is too long, must be under %d bytes.\n",
		filename,
		*lineno,
		BUFFER_SIZE);
	return -1;
}

static int read_kv(const char *filename,
		   unsigned int lineno,
		   const struct buffer *line,
		   struct string *key,
		   struct string *val)
{
	size_t i;

	/* Ignore leading whitespace, comments, and blank lines */
	for (i = 0; isspace(line->data[i]); i++)
		;

	if (!line->data[i] || line->data[i] == '#') {
		key->ptr = NULL;
		return 0;
	}

	/* Read to end of key */
	key->ptr = &line->data[i];
	for (; !isspace(line->data[i]); i++) {
		if (!line->data[i])
			goto invalid;
	}
	key->length = &line->data[i] - key->ptr;

	/* Eat up whitespace */
	for (; isspace(line->data[i]); i++)
		;

	/* Read to end of value */
	val->ptr = &line->data[i];
	for (; line->data[i] && !isspace(line->data[i]); i++) {
		if (!line->data[i])
			goto invalid;
	}
	key->length = &line->data[i] - val->ptr;
	return 0;

invalid:
	fprintf(stderr, "%s:%u: Line not in form \"[KEY] [VALUE]\".\n",
		filename,
		lineno);
	return -1;
}

static int str_to_bool(const struct string *s,
		       const char *filename,
		       unsigned int lineno)
{
	if (caseless_eq(s, "yes", 3) ||
	    caseless_eq(s, "true", 4) ||
	    caseless_eq(s, "1", 1))
		return 1;
	else if (caseless_eq(s, "no", 2) ||
		 caseless_eq(s, "false", 5) ||
		 caseless_eq(s, "0", 1))
		return 0;

	fprintf(stderr, "%s:%u: not a boolean value: ",
		filename, lineno);
	print_str(stderr, s);
	return -1;
}

static int get_port(const struct string *s,
		    const char *filename,
		    unsigned int lineno)
{
	size_t i;
	int port;

	port = 0;
	for (i = 0; i < s->length; i++)
		port += (s->ptr[i]) - '0';

	if (unlikely(0 >= port || port >= PORT_MAX)) {
		fprintf(stderr, "%s:%d: invalid port number: ",
			filename, lineno);
		print_str(stderr, s);
		return -1;
	}
	return port;
}

static int process_line(struct options *opt,
			const char *conf_file,
			unsigned int lineno,
			const struct buffer *line)
{
	struct string key, val;
	int n;

	if (read_kv(conf_file, lineno, line, &key, &val))
		return key.ptr == NULL;

	/* Check each possible option */
	if (caseless_eq(&key, "Daemonize", 9)) {
		n = str_to_bool(&val, conf_file, lineno);
		if (unlikely(NOT_BOOL(n)))
			return -1;
		opt->daemonize = n;
	} else if (caseless_eq(&key, "TransportProtocol", 17)) {
		if (caseless_eq(&val, "tcp", 3)) {
			opt->tproto = PROTOCOL_TCP;
		} else if (caseless_eq(&val, "udp", 3)) {
			opt->tproto = PROTOCOL_UDP;
		} else {
			fprintf(stderr, "%s:%d: invalid transport protocol: ",
				conf_file, lineno);
			print_str(stderr, &val);
			return -1;
		}
	} else if (caseless_eq(&key, "InternetProtocol", 16)) {
		if (caseless_eq(&val, "both", 4)) {
			opt->iproto = PROTOCOL_BOTH;
		} else if (caseless_eq(&val, "ipv4", 4)) {
			opt->iproto = PROTOCOL_IPv4;
		} else if (caseless_eq(&val, "ipv6", 4)) {
			opt->iproto = PROTOCOL_IPv6;
		} else {
			fprintf(stderr, "%s:%d: invalid internet protocol: ",
				conf_file, lineno);
			print_str(stderr, &val);
			return -1;
		}
	} else if (caseless_eq(&key, "Port", 4)) {
		n = get_port(&val, conf_file, lineno);
		if (unlikely(n < 0))
			return -1;
		opt->port = n;
	} else if (caseless_eq(&key, "StrictChecking", 14)) {
		n = str_to_bool(&val, conf_file, lineno);
		if (unlikely(NOT_BOOL(n)))
			return -1;
		opt->strict = n;
	} else if (caseless_eq(&key, "DropPrivileges", 14)) {
		n = str_to_bool(&val, conf_file, lineno);
		if (unlikely(NOT_BOOL(n)))
			return -1;
		opt->drop_privileges = n;
	} else if (caseless_eq(&key, "PidFile", 7)) {
		if (caseless_eq(&val, "none", 4)) {
			opt->pid_file = NULL;
			return 0;
		}

		opt->pid_file = dup_str(&val);
		if (unlikely(!opt->pid_file)) {
			perror("Unable to allocate memory for config value");
			cleanup(EXIT_MEMORY, 1);
		}
	} else if (caseless_eq(&key, "RequirePidFile", 14)) {
		n = str_to_bool(&val, conf_file, lineno);
		if (unlikely(NOT_BOOL(n)))
			return -1;
		opt->require_pidfile = n;
	} else if (caseless_eq(&key, "JournalFile", 11)) {
		if (caseless_eq(&val, "-", 1)) {
			opt->journal_file = NULL;
		} else if (!caseless_eq(&val, "none", 4)) {
			opt->journal_file = dup_str(&val);
			if (unlikely(!opt->journal_file)) {
				perror("Unable to allocate memory for config value");
				cleanup(EXIT_MEMORY, 1);
			}
		}
	} else if (caseless_eq(&key, "QuotesFile", 10)) {
		opt->quotes_file = dup_str(&val);
		if (unlikely(!opt->quotes_file)) {
			perror("Unable to allocate memory for config value");
			cleanup(EXIT_MEMORY, true);
		}
	} else if (caseless_eq(&key, "QuoteDivider", 12)) {
		if (caseless_eq(&val, "line", 4)) {
			opt->linediv = DIV_EVERYLINE;
		} else if (caseless_eq(&val, "percent", 7)) {
			opt->linediv = DIV_PERCENT;
		} else if (caseless_eq(&val, "file", 4)) {
			opt->linediv = DIV_WHOLEFILE;
		} else {
			fprintf(stderr, "%s:%u: unsupported division type: ",
				conf_file, lineno);
			print_str(stderr, &val);
			return -1;
		}
	} else if (caseless_eq(&key, "PadQuotes", 9)) {
		n = str_to_bool(&val, conf_file, lineno);
		if (unlikely(NOT_BOOL(n)))
			return -1;
		opt->pad_quotes = n;
	} else if (caseless_eq(&key, "DailyQuotes", 11)) {
		n = str_to_bool(&val, conf_file, lineno);
		if (unlikely(NOT_BOOL(n)))
			return -1;
		opt->is_daily = n;
	} else if (caseless_eq(&key, "AllowBigQuotes", 14)) {
		n = str_to_bool(&val, conf_file, lineno);
		if (unlikely(NOT_BOOL(n)))
			return -1;
		opt->allow_big = n;
	} else {
		fprintf(stderr, "%s:%u: unknown config option: ",
			conf_file, lineno);
		print_str(stderr, &key);
		return -1;
	}
	return 0;
}

void parse_config(struct options *opt, const char *conf_file)
{
	FILE *fh;
	struct buffer line;
	unsigned int lineno, errors;

	/* Journal hasn't been opened yet */
	printf("Reading configuration file at \"%s\"...\n", conf_file);

	if (opt->strict)
		security_conf_file_check(conf_file);

	fh = fopen(conf_file, "r");
	if (!fh) {
		fprintf(stderr, "Unable to open configuration file \"%s\": %s.\n",
			conf_file, strerror(errno));
		cleanup(EXIT_IO, true);
	}

#if DEBUG
	printf("Raw key/value pairs from config file:\n");
#endif /* DEBUG */

	lineno = 1;
	errors = 0;
	while (!read_line(fh, conf_file, &lineno, &line))
		if (process_line(conf_file, lineno, &line))
			errors++;
		lineno++;
	}
	fclose(fh);

	if (opt->strict && errors) {
		fprintf(stderr,
			"Your configuration file has %d issue%s. The daemon will not start.\n"
			"(To disable this behavior, use the --lax flag when running).\n",
			errors, PLURAL(errors));
		cleanup(EXIT_SECURITY, true);
	}
}

void check_config(const struct options *opt)
{
	if (opt->port < MIN_NORMAL_PORT && geteuid() != ROOT_USER_ID) {
		journal("Only root can bind to ports below %d.\n", MIN_NORMAL_PORT);
		cleanup(EXIT_ARGUMENTS, 1);
	}
	if (opt->pid_file && opt->pid_file[0] != '/') {
		journal("Specified pid file is not an absolute path.\n");
		cleanup(EXIT_ARGUMENTS, 1);
	}
	if (access(opt->quotes_file, R_OK)) {
		ERR_TRACE();
		journal("Unable to access quotes file '%s': %s.\n",
			opt->quotes_file, strerror(errno));
		cleanup(EXIT_IO, 1);
	}
}
