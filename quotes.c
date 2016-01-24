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

    unsigned int i;
    for (i = 0; i < lines; i++) {
        printf("line %d: %s\n", i, array[i]);
        /* free((char*)array[i]); */
    }

    /* free(array); */

    char buffer[] = "**sample data here**\n";
    int bufsize = 21;

    return write(fd, buffer, bufsize);
}

static const char** readlines(const char* fn, size_t* lines)
{
    /* Determine file size */
    struct stat* statbuf = malloc(sizeof(struct stat));

    if (statbuf == NULL) {
        perror("Unable to allocate memory for stat");
        free(statbuf);
        cleanup(1);
    }

    int ret = stat(fn, statbuf);

    if (ret < 0) {
        perror("Unable to stat quotes file");
        cleanup(1);
    }

    const int size = statbuf->st_size;
    free(statbuf);

    /* Load file into buffer */
    char* buf = malloc(size);
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

