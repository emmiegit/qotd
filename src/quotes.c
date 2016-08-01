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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <unistd.h>

#include "daemon.h"
#include "journal.h"
#include "security.h"
#include "standard.h"
#include "quotes.h"

#define QUOTE_SIZE				512  /* Set by RFC 865 */

/* Static functions */
static int format_quote(void);
static long get_file_size(void);
static int readquotes_file(void);
static int readquotes_line(void);
static int readquotes_percent(void);

/* Static variables */
static FILE *quotes_fh;
static const struct options *opt;

static struct {
	char **array;
	size_t length;
	size_t capacity;
	char *buffer;
	size_t buf_length;
} quote_file_data;

static struct {
	char *data;
	size_t length;
	size_t str_length;
} quote_buffer;

int open_quotes_file(const struct options *const local_opt)
{
	opt = local_opt;

	if (quotes_fh) {
		journal("Internal error: quotes file handle is already open\n");
		cleanup(EXIT_INTERNAL, true);
	}

	if (opt->strict) {
		security_quotes_file_check(opt->quotesfile);
	}

	quotes_fh = fopen(opt->quotesfile, "r");
	if (!quotes_fh) {
		return -1;
	}

	return 0;
}

int close_quotes_file(void)
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

void destroy_quote_buffers(void)
{
	FINAL_FREE(quote_file_data.array);
	FINAL_FREE(quote_file_data.buffer);
	FINAL_FREE(quote_buffer.data);
}

int get_quote_of_the_day(char **const buffer, size_t *const length)
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

	if (quote_file_data.length == 0) {
		journal("Quotes file is empty.\n");
		return -1;
	}

#if DEBUG
	journal("Printing %lu quote%s:\n", quote_file_data.length, PLURAL(quote_file_data.length));

	for (_i = 0; _i < quote_file_data.length; _i++) {
		journal("#%lu: %s<end>\n", _i, quote_file_data.array[_i]);
	}
#endif /* DEBUG */

	ret = format_quote();
	if (ret) {
		return -1;
	}

	*buffer = quote_buffer.data;
	*length = quote_buffer.str_length;

	return 0;
}

static int format_quote(void)
{
	const size_t quoteno = rand() % quote_file_data.length;
	size_t length, i = quoteno;

	while (unlikely(EMPTYSTR(quote_file_data.array[i]))) {
		i = (i + 1) % quote_file_data.length;

		if (i == quoteno) {
			/* All the lines are blank, this will cause an infinite loop. */
			journal("Quotes file has only empty entries.\n");
			return -1;
		}
	}

	if (opt->pad_quotes) {
		/*
		 * The pattern is "\n%s\n\n", so the total length is strlen + 3,
		 * with one byte for the null byte.
		 */
		length = strlen(quote_file_data.array[i]) + 4;
	} else {
		length = strlen(quote_file_data.array[i]) + 1;
	}

	if (!opt->allow_big && length > QUOTE_SIZE) {
		journal("Quote is %u bytes, which is %u bytes too long. Truncating to %u bytes.\n",
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

	if (opt->pad_quotes) {
		snprintf(quote_buffer.data, length, "\n%s\n\n", quote_file_data.array[i]);
	} else {
		strncpy(quote_buffer.data, quote_file_data.array[i], length);
	}

	if (opt->pad_quotes) {
		journal("Sending quotation:%s<end>\n", quote_buffer.data);
	} else {
		journal("Sending quotation:\n%s<end>\n", quote_buffer.data);
	}

	quote_buffer.str_length = length;

	return 0;
}

static long get_file_size(void)
{
	long size;
	int ret;

	ret = fseek(quotes_fh, 0, SEEK_END);
	if (ret) {
		ERR_TRACE();
		journal("Unable to seek to the end within the quotes file: %s.\n", strerror(errno));
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

static int readquotes_file(void)
{
	int ch, i;
	const long size = get_file_size();
	if (unlikely(size < 0)) {
		return -1;
	}

	/* Allocate file buffer */
	if ((size_t)size > quote_file_data.buf_length) {
		free(quote_file_data.buffer);
		quote_file_data.buffer = malloc(size + 1);
		if (!quote_file_data.buffer) {
			journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
			quote_file_data.buf_length = 0;
			return -1;
		}
		quote_file_data.buf_length = size;
	}

	rewind(quotes_fh);
	for (i = 0; (ch = fgetc(quotes_fh)) != EOF; i++) {
		if (ch == '\0') {
			ch = ' ';
		}

		quote_file_data.buffer[i] = (char)ch;
	}
	quote_file_data.buffer[size] = '\0';
	quote_file_data.length = 1;

	/* Allocate the array of strings */
	if (1 > quote_file_data.capacity) {
		free(quote_file_data.array);
		quote_file_data.array = malloc(sizeof(char *));
		if (!quote_file_data.array) {
			journal("Unable to allocate quotes array: %s.\n", strerror(errno));
			quote_file_data.capacity = 0;
			return -1;
		}
		quote_file_data.length = 1;
		quote_file_data.capacity = 1;
	}
	quote_file_data.array[0] = &quote_file_data.buffer[0];

	return 0;
}

static int readquotes_line(void)
{
	unsigned int i, j;
	size_t quotes;
	int ch;
	const long size = get_file_size();
	if (unlikely(size < 0)) {
		return -1;
	}

	/* Allocate file buffer */
	if ((size_t)size > quote_file_data.buf_length) {
		free(quote_file_data.buffer);
		quote_file_data.buffer = malloc(size + 1);
		if (!quote_file_data.buffer) {
			journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
			quote_file_data.buf_length = 0;
			return -1;
		}
		quote_file_data.buf_length = size;
	}

	/* Count number of newlines in the file */
	rewind(quotes_fh);
	for (i = 0, quotes = 0; (ch = fgetc(quotes_fh)) != EOF; i++) {
		if (ch == '\0') {
			ch = ' ';
		} else if (ch == '\n') {
			ch = '\0';
			quotes++;
		}

		quote_file_data.buffer[i] = (char)ch;
	}
	quote_file_data.buffer[size] = '\0';

	/*
	 * Account for the fact that the last line doesn't
	 * have a newline at the end.
	 */
	quotes++;

	/* Allocate the array of strings */
	if (quotes > quote_file_data.capacity) {
		free(quote_file_data.array);
		quote_file_data.array = malloc(quotes * sizeof(char *));
		if (!quote_file_data.array) {
			journal("Unable to allocate quotes array: %s.\n", strerror(errno));
			quote_file_data.capacity = 0;
			return -1;
		}
		quote_file_data.length = quotes;
		quote_file_data.capacity = quotes;
	}
	quote_file_data.array[0] = &quote_file_data.buffer[0];

	for (i = 0, j = 1; i < size; i++) {
		if (quote_file_data.buffer[i] == '\0') {
			assert(j < quotes);
			quote_file_data.array[j++] = &quote_file_data.buffer[i + 1];
		}
	}

	return 0;
}

static int readquotes_percent(void)
{
	unsigned int i, j;
	int ch, watch = 0;
	size_t quotes = 0;
	bool has_percent = false;
	const long size = get_file_size();
	if (unlikely(size < 0)) {
		return -1;
	}

	/* Allocate file buffer */
	if ((size_t)size > quote_file_data.buf_length) {
		free(quote_file_data.buffer);
		quote_file_data.buffer = malloc(size + 1);
		if (!quote_file_data.buffer) {
			journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
			quote_file_data.buf_length = 0;
			return -1;
		}
		quote_file_data.buf_length = size;
	}

	rewind(quotes_fh);
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
			quote_file_data.buffer[i - 2] = '\0';
			quotes++;
		} else if (watch > 0) {
			watch = 0;
		}

		quote_file_data.buffer[i] = (char)ch;
	}
	quote_file_data.buffer[size] = '\0';

	if (!has_percent) {
		journal("No percent signs (%%) were found in the quotes file. This means that\n"
			"the whole file will be treated as one quote, which is probably not\n"
			"what you want. If this is what you want, use the `file' option for\n"
			"`QuoteDivider' in the config file.\n");
		return -1;
	}

	/* Allocate the array of strings */
	if (quotes > quote_file_data.capacity) {
		free(quote_file_data.array);
		quote_file_data.array = malloc(quotes * sizeof(char *));
		if (!quote_file_data.array) {
			journal("Unable to allocate quotes array: %s.\n", strerror(errno));
			quote_file_data.capacity = 0;
			return -1;
		}
		quote_file_data.length = quotes;
		quote_file_data.capacity = quotes;
	}
	quote_file_data.array[0] = &quote_file_data.buffer[0];

	for (i = 0, j = 0; i < size; i++) {
		if (quote_file_data.buffer[i] == '\0') {
			assert(j < quotes);
			quote_file_data.array[j++] = &quote_file_data.buffer[i + 3];
		}
	}

	return 0;
}

