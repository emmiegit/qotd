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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "qotdd.h"
#include "quotes.h"

#define QUOTE_SIZE       512 /* Set by RFC 856 */
#define MAX_BUFFER_SIZE 4096
#define STREMPTY(x)     (((x)[0]) == '\0')
#define MIN(x, y)       (((x) < (y)) ? (x) : (y))

int snprintf(char* str, size_t size, const char* format, ...);
static void freelines(const char** array);
static const char** readlines(const char* fn, size_t* lines);

int send_quote(const int fd, const options* opt)
{
    if (opt->is_daily) {
        time_t rawtime;
        struct tm *ltime;
        time(&rawtime);
        ltime = localtime(&rawtime);
        srand(ltime->tm_year << 16 | ltime->tm_yday);
    } else {
        srand(time(NULL));
    }

    size_t lines = 0;
    const char** array = readlines(opt->quotesfile, &lines);

    if (lines == 0) {
        fprintf(stderr, "Quotes file is empty.\n");
        freelines(array);
        cleanup(1);
    }

    const size_t lineno = rand() % lines;
    size_t i = lineno;
    while (STREMPTY(array[i])) {
        i = (i + 1) % lines;

        if (i == lineno) {
            /* All the lines are blank, will cause an infinite loop */
            fprintf(stderr, "Quotes file is empty.\n");
            freelines(array);
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
    freelines(array);
    return ret;
}

static void freelines(const char** array)
{
    free((char*)array[0]);
    free((char**)array);
}

static const char** readlines(const char* fn, size_t* lines)
{
    /* Determine file size */
    struct stat* statbuf = malloc(sizeof(struct stat));
    if (statbuf == NULL) {
        perror("Unable to allocate memory for stat");
        cleanup(1);
    }

    int ret = stat(fn, statbuf);
    if (ret < 0) {
        perror("Unable to stat quotes file");
        free(statbuf);
        cleanup(1);
    }

    const int size = statbuf->st_size;
    free(statbuf);

    /* Load file into buffer */
    char* buf = malloc(size + 1);
    FILE* fh = fopen(fn, "r");
    if (fh == NULL) {
        perror("Unable to open quotes file");
        cleanup(1);
    }

    int ch, i = 0;
    /* Count number of newlines in the file */
    while ((ch = fgetc(fh)) != EOF) {
        if (ch == '\n') {
            (*lines)++;
        }

        buf[i++] = (char)ch;
    }

    ret = fclose(fh);
    if (ret < 0) {
        perror("Unable to close quotes file");
    }

    /* Allocate the array of strings */
    char** array = malloc((*lines) * sizeof(char*));
    array[0] = &buf[0];

    int j = 1;
    for (i = 0; i < size; i++) {
        if (buf[i] == '\n') {
            buf[i] = '\0';
            array[j++] = &buf[i + 1];
        }
    }

    return (const char**)array;
}

