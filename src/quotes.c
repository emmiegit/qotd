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

#include "info.h"
#include "qotdd.h"
#include "quotes.h"

#define QUOTE_SIZE      512  /* Set by RFC 856 */
#define MAX_BUFFER_SIZE 4096 /* A reasonable upper (although magical) limit */
#define STREMPTY(x)     (((x)[0]) == '\0')
#define MIN(x, y)       (((x) < (y)) ? (x) : (y))

#ifndef __APPLE__
int snprintf(char *str, size_t size, const char *format, ...);
#endif /* __APPLE__ */
static void freequotes(char **array);
static int get_file_size(const char *fn);
static char **readquotes_file(const char *fn, size_t *quotes);
static char **readquotes_line(const char *fn, size_t *quotes);
static char **readquotes_percent(const char *fn, size_t *quotes);

int send_quote(const int fd, const options *opt)
{
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

    size_t quotes = 0;
    char **array;
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
            fprintf(stderr, "Internal error: invalid value for opt->linediv: %d\n", opt->linediv);
            cleanup(1);
            return 1;
    }

    if (array == NULL) {
        /* Reading from quotes file failed */
        return 1;
    }

    if (quotes == 0) {
        fprintf(stderr, "Quotes file is empty.\n");
        freequotes(array);
        cleanup(1);
    }

#if DEBUG
    printf("Printing all %zd quotes:\n", quotes);

    size_t _i;
    for (_i = 0; _i < quotes; _i++) {
        printf("#%zd: %s<end>\n", _i, array[_i]);
    }
#endif /* DEBUG */

    const size_t quoteno = rand() % quotes;
    size_t i = quoteno;
    while (STREMPTY(array[i])) {
        i = (i + 1) % quotes;

        if (i == quoteno) {
            /* All the lines are blank, will cause an infinite loop */
            fprintf(stderr, "Quotes file has only empty entries.\n");
            freequotes(array);
            cleanup(1);
        }
    }

    const size_t len = opt->allow_big ? MAX_BUFFER_SIZE : QUOTE_SIZE;

    if (strlen(array[i]) + 3 > len) {
        fprintf(stderr, "Quotation is past the %zd-byte limit and will be truncated.\n", len);
    }

    char buf[len];
    int bytes = snprintf(buf, len, "\n%s\n\n", array[i]);

    if (bytes < 0) {
        fprintf(stderr, "Unable to format quotation in char buffer.\n");
        return bytes;
    }

    printf("Sending quotation:\n> %s\n", array[i]);

    const int ret = write(fd, buf, MIN(len, (unsigned int)bytes));
    freequotes(array);
    return ret;
}

static void freequotes(char **array)
{
    free((char *)array[0]);
    free(array);
}

static int get_file_size(const char *fn)
{
    struct stat statbuf;
    int ret = stat(fn, &statbuf);
    if (ret < 0) {
        perror("Unable to stat quotes file");
        return -1;
    }

    return statbuf.st_size;
}

static char **readquotes_file(const char *fn, size_t *quotes)
{
    const int size = get_file_size(fn);
    if (size < 0) {
        return NULL;
    }

    char *buf = malloc((size + 1) * sizeof(char));
    if (buf == NULL) {
        perror("Unable to allocate memory for file buffer");
        return NULL;
    }

    FILE *fh = fopen(fn, "r");
    if (fh == NULL) {
        fprintf(stderr, "Unable to open \"%s\": %s\n", fn, strerror(errno));
        free(buf);
        return NULL;
    }

    int ch, i = 0;
    while ((ch = fgetc(fh)) != EOF) {
        if (ch == '\0') {
            ch = ' ';
        }

        buf[i++] = (char)ch;
    }
    buf[size] = '\0';

    int ret = fclose(fh);
    if (ret) {
        perror("Unable to close quotes file");
    }

    (*quotes) = 1;
    char **array = malloc(sizeof(char *));
    array[0] = &buf[0];

    return array;
}

static char **readquotes_line(const char *fn, size_t *quotes)
{
    const int size = get_file_size(fn);
    if (size < 0) {
        return NULL;
    }

    char *buf = malloc((size + 1) * sizeof(char));
    if (buf == NULL) {
        perror("Unable to allocate memory for file buffer");
        return NULL;
    }

    FILE *fh = fopen(fn, "r");
    if (fh == NULL) {
        fprintf(stderr, "Unable to open \"%s\": %s\n", fn, strerror(errno));
        free(buf);
        return NULL;
    }

    int ch, i = 0;
    /* Count number of newlines in the file */
    while ((ch = fgetc(fh)) != EOF) {
        if (ch == '\0') {
            ch = ' ';
        } else if (ch == '\n') {
            (*quotes)++;
        }

        buf[i++] = (char)ch;
    }
    buf[size] = '\0';

    int ret = fclose(fh);
    if (ret < 0) {
        perror("Unable to close quotes file");
    }

    /* Allocate the array of strings */
    char **array = malloc((*quotes) * sizeof(char *));
    array[0] = &buf[0];

    unsigned int j = 1;
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
    const int size = get_file_size(fn);
    if (size < 0) {
        return NULL;
    }

    char *buf = malloc(size + 1);
    if (buf == NULL) {
        perror("Unable to allocate memory for file buffer");
        return NULL;
    }

    FILE *fh = fopen(fn, "r");
    if (fh == NULL) {
        fprintf(stderr, "Unable to open \"%s\": %s\n", fn, strerror(errno));
        free(buf);
        return NULL;
    }

    bool has_percent = false;
    int ch, i = 0, watch = 0;
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

    if (!has_percent) {
        fprintf(stderr, "No percent signs (%%) were found in the quotes file. This means that the whole file\n"
                        "will be treated as one quote, which is probably not what you want. If this is what\n"
                        "you want, use the `file' option for `QuoteDivider' in the config file.\n");
        free(buf);
        return NULL;
    }

    if (i == size) {
        (*quotes)++;
    }

    /* Allocate the array of strings */
    char **array = malloc((*quotes) * sizeof(char *));
    array[0] = &buf[0];

    unsigned int j = 1;
    for (i = 0; i < size && j < (*quotes); i++) {
        if (buf[i] == '\0') {
            array[j++] = &buf[i + 3];
        }
    }

    return array;
}

