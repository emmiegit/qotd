/*
 * config.c
 *
 * qotd - A simple QOTD server.
 * Copyright (c) 2015 Ammon Smith
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

#include "arguments.h"
#include "config.h"
#include "qotdd.h"

#define STREQ(x, y) (!strcmp((x), (y)))
#define BUFFER_SIZE 256
#define keystr ((char*)key)
#define valstr ((char*)val)

static char* readline(FILE* fh, const char* filename, unsigned int* lineno);
static bool str_to_bool(const char* string, const char* filename, const unsigned int lineno);
static void confcleanup(const int ret);

static FILE* fh;
static char* line;

void parse_config(const char* conf_file, options* opt)
{
    fh = fopen(conf_file, "r");

    if (fh == NULL) {
        fprintf(stderr, "Unable to open configuration file (%s): %s\n", conf_file, strerror(errno));
        cleanup(1);
    }

    char key[BUFFER_SIZE], val[BUFFER_SIZE];
    bool firsthalf = true;
    unsigned int lineno = 1;
    while ((line = readline(fh, conf_file, &lineno)) != NULL) {
        /* Ignore comments */
        if (line[0] == '#') {
            free(line);
            continue;
        }

        /* Read each line */
        int i = 0, j = 0;
        char ch;
        bool ignoring_whitespace = false;
        while ((ch = line[i++]) != '\0') {
            if (firsthalf) {
                if (ch == ' ' || ch == '\t') {
                    ignoring_whitespace = true;
                } else if (ignoring_whitespace) {
                    ignoring_whitespace = false;
                    firsthalf = false;
                    key[j] = '\0';
                    j = 0;
                    val[j++] = ch;
                } else {
                    key[j++] = ch;
                }
            } else {
                val[j++] = ch;
            }
        }
        val[j] = '\0';

        /* Parse each line */
        if (STREQ(keystr, "Port")) {
            int port = atoi(valstr);
            if (port <= 0) {
                fprintf(stderr, "%s:%d: invalid port number: \"%s\"\n", conf_file, lineno, valstr);
                confcleanup(1);
            }

            opt->port = port;
        } else if (STREQ(keystr, "QuotesFile")) {
            opt->quotesfile = valstr;
        } else if (STREQ(keystr, "Delimeter")) {
            if (STREQ(valstr, "newline")) {
                opt->delimeter = DELIM_NEWLINE;
            } else if (STREQ(valstr, "emptyline")) {
                opt->delimeter = DELIM_EMPTYLINE;
            } else {
                fprintf(stderr, "%s:%d: unknown delimiter: \"%s\"\n", conf_file, lineno, valstr);
                confcleanup(1);
            }
        } else if (STREQ(keystr, "DailyQuotes")) {
            opt->is_daily = str_to_bool(valstr, conf_file, lineno);
        } else if (STREQ(keystr, "AllowBigQuotes")) {
            opt->allow_big = str_to_bool(valstr, conf_file, lineno);
        } else {
            fprintf(stderr, "%s:%d: ignoring unknown conf option: \"%s\"\n", conf_file, lineno, keystr);
        }

        lineno++;
        firsthalf = true;
    }

    if (line) free(line);
    fclose(fh);
}

static char* readline(FILE* fh, const char* filename, unsigned int* lineno)
{
    char* buffer = malloc(BUFFER_SIZE);
    int ch, i;

    for (i = 0; i < BUFFER_SIZE; i++) {
        ch = fgetc(fh);

        if (ch == '\n') {
            if (i) {
                buffer[i] = '\0';
                return (char*)buffer;
            } else {
                /* Ignore empty lines */
                i--;
                (*lineno)++;
            }
        } else if (ch == EOF) {
            if (i) {
                buffer[i] = '\0';
                return (char*)buffer;
            } else {
                return NULL;
            }
        } else {
            buffer[i] = (char)ch;
        }
    }

    /* Ran out of buffer space */
    fprintf(stderr, "%s:%i: line is too long, must be under %d bytes.\n", filename, *lineno, BUFFER_SIZE);
    free(buffer);
    confcleanup(1);
    return NULL;
}

static bool str_to_bool(const char* string, const char* filename, const unsigned int lineno)
{
    if (STREQ(string, "yes") || STREQ(string, "YES")
     || STREQ(string, "true") || STREQ(string, "TRUE")) {
        return true;
    } else if (STREQ(string, "no") || STREQ(string, "NO")
            || STREQ(string, "false") || STREQ(string, "FALSE")) {
        return false;
    } else {
        fprintf(stderr, "%s:%d: not a boolean value: \"%s\"\n", filename, lineno, string);
        confcleanup(1);
        return false;
    }
}

static void confcleanup(const int ret)
{
    if (line) free(line);
    fclose(fh);
    cleanup(ret);
}

