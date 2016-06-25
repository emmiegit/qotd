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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if DEBUG
# define PLURAL(x)      ((x == 1) ? "" : "s")
#endif /* DEBUG */

#define QUOTE_SIZE      512  /* Set by RFC 856 */
#define EMPTYSTR(x)     (((x)[0]) == '\0')
#define MIN(x, y)       (((x) < (y)) ? (x) : (y))

/* Static declarations */
static struct selected_quote get_quote_of_the_day(const struct options *opt);
static int get_file_size(const char *fn);
static char **readquotes_file(const char *fn, size_t *quotes);
static char **readquotes_line(const char *fn, size_t *quotes);
static char **readquotes_percent(const char *fn, size_t *quotes);

struct selected_quote {
    char **array;
    char *buffer;
    size_t length;
};

int tcp_send_quote(int fd, const struct options *opt)
{
    struct selected_quote quote;
    int ret;

    quote = get_quote_of_the_day(opt);
    ret = write(fd, quote.buffer, quote.length);
    if (ret < 0) {
        journal("Unable to write to TCP connection socket: %s.\n", strerror(errno));
    }

    free((char *)quote.array[0]);
    free(quote.array);
    free(quote.buffer);
    return ret;
}

int udp_send_quote(int fd, const struct sockaddr *cli_addr, socklen_t clilen, const struct options *opt)
{
    struct selected_quote quote;
    int ret;

    quote = get_quote_of_the_day(opt);
    ret = sendto(fd, quote.buffer, quote.length, 0, cli_addr, clilen);
    if (ret < 0) {
        journal("Unable to write to UDP socket: %s.\n", strerror(errno));
    }

    free((char *)quote.array[0]);
    free(quote.array);
    free(quote.buffer);
    return ret;
}

static struct selected_quote get_quote_of_the_day(const struct options *opt)
{
    struct selected_quote selected_quote;
    size_t quotes, quoteno, len, i;
    char **array;
    char *buf;
    int ret;

    selected_quote.array = NULL;

    if (opt->is_daily) {
        /* Create random seed based on the current day */
        time_t rawtime;
        struct tm *ltime;
        time(&rawtime);
        ltime = localtime(&rawtime);
        srand(ltime->tm_year << 16 | ltime->tm_yday);
    } else {
        srand(time(NULL));
    }

    quotes = 0;
    switch (opt->linediv) {
        case DIV_EVERYLINE:
            array = readquotes_line(opt->quotesfile, &quotes);
            break;
        case DIV_PERCENT:
            array = readquotes_percent(opt->quotesfile, &quotes);
            break;
        case DIV_WHOLEFILE:
            array = readquotes_file(opt->quotesfile, &quotes);
            break;
        default:
            journal("Internal error: invalid enum value for quote_divider: %d.\n", opt->linediv);
            cleanup(1, true);
            return selected_quote;
    }

    if (array == NULL) {
        /* Reading from quotes file failed */
        return selected_quote;
    }

    if (quotes == 0) {
        journal("Quotes file is empty.\n");
        cleanup(1, true);
    }

#if DEBUG
    journal("Printing %lu quote%s:\n", quotes, PLURAL(quotes));

    for (i = 0; i < quotes; i++) {
        journal("#%lu: %s<end>\n", i, array[i]);
    }
#endif /* DEBUG */

    quoteno = rand() % quotes;
    i = quoteno;
    while (EMPTYSTR(array[i])) {
        i = (i + 1) % quotes;

        if (i == quoteno) {
            /* All the lines are blank, will cause an infinite loop */
            journal("Quotes file has only empty entries.\n");
            cleanup(1, true);
        }
    }

    /* The pattern is "\n%s\n\n", so the total length is strlen + 3,
       with one byte for the null byte. */
    len = strlen(array[i]) + 4;

    if (!opt->allow_big && len > QUOTE_SIZE) {
        journal("Quote is %u bytes, which is %u bytes too long! Truncating to %u bytes.\n",
                len, len - QUOTE_SIZE, QUOTE_SIZE);
        len = QUOTE_SIZE;
    }

    buf = malloc(len * sizeof(char));

    if (buf == NULL) {
        int errno_ = errno;
        journal("Unable to allocate variable buffer: %s\n", strerror(errno));
        errno = errno_;
        return selected_quote;
    }

    ret = snprintf(buf, len, "\n%s\n\n", array[i]);

    if (ret < 0) {
        int errno_ = errno;
        journal("Unable to format quotation in char buffer.\n");
        errno = errno_;
        return selected_quote;
    }

    journal("Sending quotation:%s\n", buf);
    selected_quote.array = array;
    selected_quote.buffer = buf;
    selected_quote.length = MIN(len, (unsigned int)ret);
    return selected_quote;
}

static int get_file_size(const char *fn)
{
    struct stat statbuf;

    if (stat(fn, &statbuf) < 0) {
        journal("Unable to stat quotes file: %s.\n", strerror(errno));
        return -1;
    }

    return statbuf.st_size;
}

static char **readquotes_file(const char *fn, size_t *quotes)
{
    FILE *fh;
    char **array;
    char *buf;
    int ch, ret, i;

    const int size = get_file_size(fn);
    if (size < 0) {
        return NULL;
    }

    buf = malloc((size + 1) * sizeof(char));
    if (buf == NULL) {
        journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
        return NULL;
    }

    fh = fopen(fn, "r");
    if (fh == NULL) {
        journal("Unable to open \"%s\": %s\n", fn, strerror(errno));
        free(buf);
        return NULL;
    }

    i = 0;
    while ((ch = fgetc(fh)) != EOF) {
        if (ch == '\0') {
            ch = ' ';
        }

        buf[i++] = (char)ch;
    }
    buf[size] = '\0';

    ret = fclose(fh);
    if (ret) {
        journal("Unable to close \"%s\": %s\n", fn, strerror(errno));
    }

    (*quotes) = 1;
    array = malloc(sizeof(char *));
    array[0] = &buf[0];

    return array;
}

static char **readquotes_line(const char *fn, size_t *quotes)
{
    FILE *fh;
    char **array;
    char *buf;
    int ch, ret;

    int i;
    unsigned int j;

    const int size = get_file_size(fn);
    if (size < 0) {
        return NULL;
    }

    buf = malloc((size + 1) * sizeof(char));
    if (buf == NULL) {
        journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
        return NULL;
    }

    fh = fopen(fn, "r");
    if (fh == NULL) {
        journal("Unable to open \"%s\": %s\n", fn, strerror(errno));
        free(buf);
        return NULL;
    }

    /* Count number of newlines in the file */
    i = 0;
    while ((ch = fgetc(fh)) != EOF) {
        if (ch == '\0') {
            ch = ' ';
        } else if (ch == '\n') {
            (*quotes)++;
        }

        buf[i++] = (char)ch;
    }
    buf[size] = '\0';

    ret = fclose(fh);
    if (ret) {
        journal("Unable to close \"%s\": %s\n", fn, strerror(errno));
    }

    /* Allocate the array of strings */
    array = malloc((*quotes) * sizeof(char *));
    array[0] = &buf[0];

    j = 1;
    for (i = 0; i < size; i++) {
        if (buf[i] == '\n') {
            buf[i] = '\0';

            if (j < *quotes) {
                array[j++] = &buf[i + 1];
            }
        }
    }

    return array;
}

static char **readquotes_percent(const char *fn, size_t *quotes)
{
    FILE *fh;
    char **array;
    char *buf;
    int watch, ch, ret;
    bool has_percent;

    int i;
    unsigned int j;

    const int size = get_file_size(fn);
    if (size < 0) {
        return NULL;
    }

    buf = malloc(size + 1);
    if (buf == NULL) {
        journal("Unable to allocate memory for file buffer: %s.\n", strerror(errno));
        return NULL;
    }

    fh = fopen(fn, "r");
    if (fh == NULL) {
        journal("Unable to open \"%s\": %s\n", fn, strerror(errno));
        free(buf);
        return NULL;
    }

    has_percent = false;
    i = 0;
    watch = 0;
    /* Count number of dividers in the file */
    while ((ch = fgetc(fh)) != EOF) {
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
            buf[i - 2] = '\0';
            (*quotes)++;
        } else if (watch > 0) {
            watch = 0;
        }

        buf[i++] = (char)ch;
    }
    buf[size] = '\0';

    ret = fclose(fh);
    if (ret) {
        journal("Unable to close \"%s\": %s\n", fn, strerror(errno));
    }

    if (!has_percent) {
        journal("No percent signs (%%) were found in the quotes file. This means that the whole file\n"
                    "will be treated as one quote, which is probably not what you want. If this is what\n"
                    "you want, use the `file' option for `QuoteDivider' in the config file.\n");
        free(buf);
        return NULL;
    }

    if (i == size) {
        (*quotes)++;
    }

    /* Allocate the array of strings */
    array = malloc((*quotes) * sizeof(char *));
    array[0] = &buf[0];

    for (i = 0, j = 1; i < size && j < (*quotes); i++) {
        if (buf[i] == '\0') {
            array[j++] = &buf[i + 3];
        }
    }

    return array;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

