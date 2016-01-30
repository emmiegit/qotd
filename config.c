/*
 * config.c
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

#include "config.h"
#include "info.h"
#include "qotdd.h"

#define STREQ(x, y) (strcmp((x), (y)) == 0)
#define BUFFER_SIZE 256

#if DEBUG == 1
# define NULLSTR(x) ((x) ? (x) : "<null>")
# define BOOLSTR(x) ((x) ? "<true>" : "<false>")
static void print_options(options* opt);
#endif /* DEBUG */

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

#if DEBUG == 1
    printf("Raw key/value pairs from config file:\n");
#endif /* DEBUG */

    char key[BUFFER_SIZE], val[BUFFER_SIZE];
    bool firsthalf = true;
    unsigned int lineno = 1;
    while ((line = readline(fh, conf_file, &lineno)) != NULL) {
        /* Ignore comments, blank lines are already ignored by readline() */
        if (line[0] == '#') {
            free(line);
            line = NULL;
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
        free(line);
        line = NULL;

        char* keystr = (char*)key;
        char* valstr = (char*)val;

#if DEBUG == 1
        printf("[%s] = [%s]\n", keystr, valstr);
#endif /* DEBUG */

        const size_t vallen = strlen(valstr);
        if (vallen > 1 && ((valstr[0] == '\'' && valstr[vallen - 1] == '\'') ||
                           (valstr[0] == '\"' && valstr[vallen - 1] == '\"'))) {
            valstr[vallen - 1] = '\0';
            valstr++;

#if DEBUG == 1
            printf("[%s] = [%s] <\n", keystr, valstr);
#endif /* DEBUG */
        }

        /* Parse each line */
        if (STREQ(keystr, "Port")) {
            int port = atoi(valstr);
            if (port <= 0) {
                fprintf(stderr, "%s:%d: invalid port number: \"%s\"\n", conf_file, lineno, valstr);
                confcleanup(1);
            }

            opt->port = port;
        } else if (STREQ(keystr, "PidFile")) {
            opt->pidfile = malloc(strlen(valstr) + 1);
            opt->pidmalloc = true;
            strcpy(opt->pidfile, valstr);
        } else if (STREQ(keystr, "RequirePidFile")) {
            opt->require_pidfile = str_to_bool(valstr, conf_file, lineno);
        } else if (STREQ(keystr, "QuotesFile")) {
            opt->quotesfile = malloc(strlen(valstr) + 1);
            opt->quotesmalloc = true;
            strcpy(opt->quotesfile, valstr);
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

#if DEBUG == 1
    print_options(opt);
#endif /* DEBUG */

    fclose(fh);
}

#if DEBUG == 1
static void print_options(options* opt)
{
    printf("\nContents of struct 'opt':\n");
    printf("Port: %d\n", opt->port);
    printf("QuotesFile: %s\n", NULLSTR(opt->quotesfile));
    printf("PidFile: %s\n", NULLSTR(opt->pidfile));
    printf("QuotesMalloc: %s\n", BOOLSTR(opt->quotesmalloc));
    printf("PidMalloc: %s\n", BOOLSTR(opt->pidmalloc));
    printf("Daemonize: %s\n", BOOLSTR(opt->daemonize));
    printf("DailyQuotes: %s\n", BOOLSTR(opt->is_daily));
    printf("AllowBigQuotes: %s\n", BOOLSTR(opt->allow_big));
    printf("End of 'opt'.\n");
}
#endif /* DEBUG */

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
                /* Return rest of line when EOF hits before newline */
                buffer[i] = '\0';
                return (char*)buffer;
            } else {
                /* Return NULL when a EOF hits */
                free(buffer);
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
    if (line) {
        free(line);
        line = NULL;
    }

    fclose(fh);
    cleanup(ret);
}

