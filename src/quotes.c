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

#include <sys/stat.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core.h"
#include "daemon.h"
#include "journal.h"
#include "security.h"
#include "quotes.h"

#define QUOTE_SIZE		512  /* Set by RFC 865 */

/* Static data */

static FILE *quotes_fh;
static const struct options *opt;

static struct {
	char **array;
	size_t length;

	char *buffer;
	size_t buf_length;
} quote_file_data;

static struct {
	char *data;
	size_t length;
	size_t str_length;
} quote_buffer;

/* Utilites */

static unsigned long djb2_hash(const char *str)
{
	unsigned long hash;
	char ch;

	hash = 5381;
	while ((ch = *str++)) {
		hash = ((hash << 5) + hash) + ch;
	}
	return hash;
}

static void seed_randgen(void)
{
	char hostname[256];
	time_t rawtime;
	const struct tm *ltime;
	unsigned int seed;

	rawtime = time(NULL);
	if (!opt->is_daily) {
		srand(rawtime);
		return;
	}

	/* Create random seed based on current day and hostname */

	ltime = localtime(&rawtime);
	seed = ltime->tm_year << 16 | ltime->tm_yday;
	if (!gethostname(hostname, sizeof(hostname))) {
		hostname[sizeof(hostname) - 1] = '\0';
		seed ^= djb2_hash(hostname);
	}
	srand(seed);
}

static int format_quote(void)
{
	size_t quoteno, length, i;

	i = quoteno = rand() % quote_file_data.length;
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
			length,
			length - QUOTE_SIZE,
			QUOTE_SIZE);
		length = QUOTE_SIZE;
	}

	if (length > quote_buffer.length) {
		void *ptr;

		ptr = realloc(quote_buffer.data, length);
		if (unlikely(!ptr)) {
			journal("Unable to allocate formatted quote buffer: %s.\n", strerror(errno));
			return -1;
		}
		quote_buffer.data = ptr;
		quote_buffer.length = length;
	}

	if (opt->pad_quotes)
		snprintf(quote_buffer.data, length, "\n%s\n\n", quote_file_data.array[i]);
	else
		strncpy(quote_buffer.data, quote_file_data.array[i], length);

	if (opt->pad_quotes)
		journal("Sending quotation:%s<end>\n", quote_buffer.data);
	else
		journal("Sending quotation:\n%s<end>\n", quote_buffer.data);

	quote_buffer.str_length = length;
	return 0;
}

static size_t get_file_size(void)
{
	long size;

	if (fseeko(quotes_fh, 0, SEEK_END)) {
		const int errsave = errno;
		JTRACE();
		journal("Unable to seek to the end within the quotes file: %s.\n", strerror(errsave));
		return -1;
	}
	if ((size = ftello(quotes_fh)) < 0) {
		const int errsave = errno;
		JTRACE();
		journal("Unable to determine file size: %s.\n", strerror(errsave));
		return -1;
	}
	return size;
}

static int readquotes_file(void)
{
	size_t i, fsize;

	fsize = get_file_size();
	if (unlikely(fsize == (size_t)-1))
		return -1;

	/* Allocate file buffer */
	if (fsize > quote_file_data.buf_length) {
		void *ptr;

		ptr = realloc(quote_file_data.buffer, fsize + 1);
		if (unlikely(!ptr)) {
			journal("Unable to allocate memory for file buffer: %s.\n",
				strerror(errno));
			return -1;
		}
		quote_file_data.buffer = ptr;
		quote_file_data.buf_length = fsize;
	}

	rewind(quotes_fh);
	if (fread(quote_file_data.buffer, fsize, 1, quotes_fh) != 1) {
		journal("Unable to read from quotes file.\n");
		return -1;
	}
	for (i = 0; i < fsize; i++) {
		char *c;

		c = &quote_file_data.buffer[i];
		if (!*c)
			*c = ' ';
	}
	quote_file_data.buffer[fsize] = '\0';
	quote_file_data.length = fsize;

	/* Allocate the array of strings */
	if (quote_file_data.length < 1) {
		void *ptr;

		ptr = realloc(quote_file_data.array, sizeof(char *));
		if (unlikely(!ptr)) {
			journal("Unable to allocate quotes array: %s.\n",
				strerror(errno));
			return -1;
		}
		quote_file_data.array = ptr;
		quote_file_data.length = 1;
	}
	quote_file_data.array[0] = &quote_file_data.buffer[0];
	return 0;
}

static int readquotes_line(void)
{
	unsigned int i, j;
	size_t fsize, quotes;

	fsize = get_file_size();
	if (unlikely(fsize == (size_t)-1))
		return -1;

	/* Allocate file buffer */
	if (fsize > quote_file_data.buf_length) {
		void *ptr;

		ptr = realloc(quote_file_data.buffer, fsize + 1);
		if (unlikely(!ptr)) {
			journal("Unable to allocate memory for file buffer: %s.\n",
				strerror(errno));
			quote_file_data.buf_length = 0;
			return -1;
		}
		quote_file_data.buffer = ptr;
		quote_file_data.buf_length = fsize;
	}

	/* Count number of newlines in the file */
	rewind(quotes_fh);
	if (fread(quote_file_data.buffer, fsize, 1, quotes_fh) != 1) {
		journal("Unable to read from quotes file.\n");
		return -1;
	}
	for (i = 0, quotes = 0; i < fsize; i++) {
		char *c;

		c = &quote_file_data.buffer[i];
		switch (*c) {
		case '\0':
			*c = ' ';
			break;
		case '\n':
			*c = '\0';
			quotes++;
		}
	}

	/*
	 * Account for the fact that the last line doesn't
	 * have a newline at the end.
	 */
	quotes++;

	/* Allocate the array of strings */
	if (quotes > quote_file_data.length) {
		void *ptr;

		ptr = realloc(quote_file_data.array, quotes * sizeof(char *));
		if (unlikely(!ptr)) {
			journal("Unable to allocate quotes array: %s.\n",
				strerror(errno));
			return -1;
		}
		quote_file_data.array = ptr;
		quote_file_data.length = quotes;
	}
	quote_file_data.array[0] = &quote_file_data.buffer[0];
	for (i = 0, j = 1; i < fsize; i++) {
		if (!quote_file_data.buffer[i]) {
			assert(j < quotes);
			quote_file_data.array[j++] = &quote_file_data.buffer[i + 1];
		}
	}
	return 0;
}

static int readquotes_percent(void)
{
	unsigned int i, j;
	size_t fsize, quotes;
	int watch, has_percent;

	watch = 0;
	quotes = 0;
	has_percent = 0;
	fsize = get_file_size();
	if (unlikely(fsize == (size_t)-1))
		return -1;

	/* Allocate file buffer */
	if (fsize > quote_file_data.buf_length) {
		void *ptr;

		ptr = realloc(quote_file_data.buffer, fsize + 1);
		if (unlikely(!ptr)) {
			journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
			quote_file_data.buf_length = 0;
			return -1;
		}

		quote_file_data.buffer = ptr;
		quote_file_data.buf_length = fsize;
	}

	rewind(quotes_fh);
	for (i = 0; i < fsize; i++) {
		char *c;

		c = &quote_file_data.buffer[i];
		if (*c == '\0') {
			*c = ' ';
			watch = 0;
		} else if (*c == '\n' && watch == 0) {
			watch++;
		} else if (*c == '%' && watch == 1) {
			has_percent = 1;
			watch++;
		} else if (*c == '\n' && watch == 2) {
			watch = 0;
			*(c - 2) = '\0';
			quotes++;
		} else {
			if (watch > 0)
				watch = 0;
		}
	}
	quote_file_data.buffer[fsize] = '\0';

	if (!has_percent) {
		journal("No dividing percent signs (%%) were found in the quotes file. This\n"
			"means that the whole file will be treated as one quote, which is\n"
			"probably not what you want. If this is what you want, use the `file'\n"
			"option for `QuoteDivider' in the config file.\n");
		return -1;
	}

	/* Allocate the array of strings */
	if (quotes > quote_file_data.length) {
		void *ptr;

		ptr = realloc(quote_file_data.array, quotes * sizeof(char *));
		if (unlikely(!ptr)) {
			journal("Unable to allocate quotes array: %s.\n",
				strerror(errno));
			return -1;
		}
		quote_file_data.array = ptr;
		quote_file_data.length = quotes;
	}
	quote_file_data.array[0] = &quote_file_data.buffer[0];
	for (i = 0, j = 1; i < fsize; i++) {
		if (quote_file_data.buffer[i] == '\0') {
			assert(j < quotes);
			quote_file_data.array[j++] = &quote_file_data.buffer[i + 3];
		}
	}
	return 0;
}

/* Externals */

int open_quotes_file(const struct options *const local_opt)
{
	opt = local_opt;

	if (quotes_fh) {
		journal("Internal error: quotes file handle is already open\n");
		cleanup(EXIT_INTERNAL, 1);
	}
	if (opt->strict)
		security_quotes_file_check(opt->quotes_file);

	quotes_fh = fopen(opt->quotes_file, "r");
	if (!quotes_fh)
		return -1;
	return 0;
}

int close_quotes_file(void)
{
	int ret;

	if (!quotes_fh)
		return 0;
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

	seed_randgen();
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
		JTRACE();
		journal("Internal error: invalid enum value for quote_divider: %d.\n", opt->linediv);
		cleanup(EXIT_INTERNAL, 1);
		return -1;
	}

	if (readquotes()) {
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

	if (format_quote())
		return -1;
	*buffer = quote_buffer.data;
	*length = quote_buffer.str_length;
	return 0;
}

