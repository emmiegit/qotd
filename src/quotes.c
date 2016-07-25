/*
 * quotes.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>

#include "daemon.h"
#include "journal.h"
#include "standard.h"
#include "quotes.h"

#define QUOTE_SIZE		512  /* Set by RFC 865 */

/* Static functions */
static int get_quote_of_the_day(const struct options *opt);
static int format_quote(const struct options *opt);
static long get_file_size();
static int readquotes_file();
static int readquotes_line();
static int readquotes_percent();

/* Static variables */
static FILE *quotes_fh;

static struct {
	char **array;
	size_t length;
	size_t capacity;
	char *buffer;
	size_t buf_length;
} quote_list;

static struct {
	char *data;
	size_t length;
	size_t str_length;
} quote_buffer;

int open_quotes_file(const char *path)
{
	if (quotes_fh) {
		journal("Internal error: quotes file handle is already open\n");
		cleanup(EXIT_INTERNAL, true);
	}

	quotes_fh = fopen(path, "r");
	if (!quotes_fh) {
		return -1;
	}

	return 0;
}

int close_quotes_file()
{
	int ret;

	if (!quotes_fh) {
		journal("Internal error: no quotes file handle to close.\n");
		cleanup(EXIT_INTERNAL, true);
	}

	ret = fclose(quotes_fh);
	quotes_fh = NULL;
	return ret;
}

int tcp_send_quote(const int fd, const struct options *opt)
{
	int ret;

	ret = get_quote_of_the_day(opt);
	if (unlikely(!ret)) {
		return -1;
	}

	ret = write(fd, quote_buffer.data, quote_buffer.str_length);
	if (unlikely(ret)) {
		journal("Unable to write to TCP connection socket: %s.\n", strerror(errno));
		return -1;
	}

	return 0;
}

int udp_send_quote(int fd, const struct sockaddr *cli_addr, socklen_t clilen, const struct options *opt)
{
	int ret;

	ret = get_quote_of_the_day(opt);
	if (unlikely(!ret)) {
		return -1;
	}

	ret = sendto(fd, quote_buffer.data, quote_buffer.str_length, 0, cli_addr, clilen);
	if (unlikely(ret)) {
		journal("Unable to write to UDP socket: %s.\n", strerror(errno));
	}

	return 0;
}

static int get_quote_of_the_day(const struct options *opt)
{
#if DEBUG
	unsigned int _i;
#endif /* DEBUG */

	int (*readquotes)();
	int ret;

	if (opt->is_daily) {
		/* Create random seed based on the current day */
		time_t rawtime;
		struct tm *ltime;
		rawtime = time(NULL);
		ltime = localtime(&rawtime);
		srand(ltime->tm_year << 16 | ltime->tm_yday);
	} else {
		srand(time(NULL));
	}

	switch (opt->linediv) {
		case DIV_EVERYLINE:
			readquotes = readquotes_line;
			break;
		case DIV_PERCENT:
			readquotes = readquotes_percent;
			break;
		case DIV_WHOLEFILE:
			readquotes = readquotes_file;
			break;
		default:
			ERR_TRACE();
			journal("Internal error: invalid enum value for quote_divider: %d.\n", opt->linediv);
			cleanup(EXIT_INTERNAL, true);
			return -1;
	}

	ret = readquotes();
	if (ret) {
		return -1;
	}

	if (quote_list.length == 0) {
		journal("Quotes file is empty.\n");
		return -1;
	}

#if DEBUG
	journal("Printing %lu quote%s:\n", quote_list.length, PLURAL(quote_list.length));

	for (_i = 0; _i < quote_list.length; _i++) {
		journal("#%lu: %s<end>\n", _i, quote_list.array[_i]);
	}
#endif /* DEBUG */

	return format_quote(opt);
}

static int format_quote(const struct options *opt)
{
	const size_t quoteno = rand() % quote_list.length;
	size_t length, i = quoteno;

	while (unlikely(EMPTYSTR(quote_list.array[i]))) {
		i = (i + 1) % quote_list.length;

		if (i == quoteno) {
			/* All the lines are blank, will cause an infinite loop */
			journal("Quotes file has only empty entries.\n");
			return -1;
		}
	}

	/* The pattern is "\n%s\n\n", so the total length is strlen + 3,
	   with one byte for the null byte. */
	length = strlen(quote_list.array[i]) + 4;

	if (!opt->allow_big && length > QUOTE_SIZE) {
		journal("Quote is %u bytes, which is %u bytes too long! Truncating to %u bytes.\n",
				length, length - QUOTE_SIZE, QUOTE_SIZE);
		length = QUOTE_SIZE;
	}

	if (length > quote_buffer.length) {
		free(quote_buffer.data);
		quote_buffer.data = malloc(length);

		if (unlikely(!quote_buffer.data)) {
			journal("Unable to allocate formatted quote buffer: %s.\n", strerror(errno));
			quote_buffer.length = 0;
			return -1;
		}

		quote_buffer.length = length;
	}

	snprintf(quote_buffer.data, length, "\n%s\n\n", quote_list.array[i]);

	journal("Sending quotation:%s\n", quote_buffer.data);
	quote_buffer.length = length;
	return 0;
}

static long get_file_size()
{
	long size;
	int ret;

	ret = fseek(quotes_fh, 0, SEEK_END);
	if (ret) {
		ERR_TRACE();
		journal("Unable to seek within quotes file: %s.\n", strerror(errno));
		return -1;
	}

	size = ftell(quotes_fh);
	if (size < 0) {
		ERR_TRACE();
		journal("Unable to determine file size: %s.\n", strerror(errno));
		return -1;
	}

	return size;
}

static int readquotes_file()
{
	int ch, i;
	const long size = get_file_size();
	if (unlikely(size < 0)) {
		return -1;
	}

	/* Allocate file buffer */
	if ((size_t)size > quote_list.buf_length) {
		free(quote_list.buffer);
		quote_list.buffer = malloc(size + 1);
		if (!quote_list.buffer) {
			journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
			quote_list.buf_length = 0;
			return -1;
		}
		quote_list.buf_length = size;
	}

	rewind(quotes_fh);
	for (i = 0; (ch = fgetc(quotes_fh)) != EOF; i++) {
		if (ch == '\0') {
			ch = ' ';
		}

		quote_list.buffer[i] = (char)ch;
	}
	quote_list.buffer[size] = '\0';
	quote_list.length = 1;

	/* Allocate the array of strings */
	if (1 > quote_list.capacity) {
		free(quote_list.array);
		quote_list.array = malloc(sizeof(char *));
		if (!quote_list.array) {
			journal("Unable to allocate quotes array: %s.\n", strerror(errno));
			quote_list.capacity = 0;
			return -1;
		}
		quote_list.length = 1;
		quote_list.capacity = 1;
	}
	quote_list.array[0] = &quote_list.buffer[0];

	return 0;
}

static int readquotes_line()
{
	int ch;

	unsigned int i, j;
	size_t quotes;

	const long size = get_file_size();
	if (size < 0) {
		return -1;
	}

	/* Allocate file buffer */
	if ((size_t)size > quote_list.buf_length) {
		free(quote_list.buffer);
		quote_list.buffer = malloc(size + 1);
		if (!quote_list.buffer) {
			journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
			quote_list.buf_length = 0;
			return -1;
		}
		quote_list.buf_length = size;
	}

	/* Count number of newlines in the file */
	rewind(quotes_fh);
	for (i = 0, quotes = 0; (ch = fgetc(quotes_fh)) != EOF; i++) {
		if (ch == '\0') {
			ch = ' ';
		} else if (ch == '\n') {
			quotes++;
		}

		quote_list.buffer[i] = (char)ch;
	}
	quote_list.buffer[size] = '\0';


	/* Allocate the array of strings */
	if (quotes > quote_list.capacity) {
		free(quote_list.array);
		quote_list.array = malloc(quotes * sizeof(char *));
		if (!quote_list.array) {
			journal("Unable to allocate quotes array: %s.\n", strerror(errno));
			quote_list.capacity = 0;
			return -1;
		}
		quote_list.length = quotes;
		quote_list.capacity = quotes;
	}
	quote_list.array[0] = &quote_list.buffer[0];

	for (i = 0, j = 1; i < size; i++) {
		if (quote_list.buffer[i] == '\n') {
			quote_list.buffer[i] = '\0';

			if (j < quotes) {
				quote_list.array[j++] = &quote_list.buffer[i + 1];
			}
		}
	}

	return 0;
}

static int readquotes_percent()
{
	unsigned int i;
	int ch, watch = 0;
	size_t quotes = 0;
	bool has_percent = false;
	const int size = get_file_size();
	if (size < 0) {
		return -1;
	}

	/* Allocate file buffer */
	if ((size_t)size > quote_list.buf_length) {
		free(quote_list.buffer);
		quote_list.buffer = malloc(size + 1);
		if (!quote_list.buffer) {
			journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
			quote_list.buf_length = 0;
			return -1;
		}
		quote_list.buf_length = size;
	}

	for (i = 0; (ch = fgetc(quotes_fh)) != EOF; i++) {
		if (ch == '\0') {
			ch = ' ';
			watch = 0;
		} else if (ch == '\n' && watch == 0) {
			watch++;
		} else if (ch == '%' && watch == 1) {
			has_percent = true;
			watch++;
		} else if (ch == '\n' && watch == 2) {
			watch = 0;
			quote_list.buffer[i - 2] = '\0';
			quotes++;
		} else if (watch > 0) {
			watch = 0;
		}

		quote_list.buffer[i] = (char)ch;
	}
	quote_list.buffer[size] = '\0';

	if (!has_percent) {
		journal("No percent signs (%%) were found in the quotes file. This means that the whole file\n"
			"will be treated as one quote, which is probably not what you want. If this is what\n"
			"you want, use the `file' option for `QuoteDivider' in the config file.\n");
		return -1;
	}

	if (i == (unsigned long)size) {
		quotes++;
	}

	/* Allocate the array of strings */
	if (quotes > quote_list.capacity) {
		free(quote_list.array);
		quote_list.array = malloc(quotes * sizeof(char *));
		if (!quote_list.array) {
			journal("Unable to allocate quotes array: %s.\n", strerror(errno));
			quote_list.capacity = 0;
			return -1;
		}
		quote_list.length = quotes;
		quote_list.capacity = quotes;
	}
	quote_list.array[0] = &quote_list.buffer[0];

	return 0;
}

