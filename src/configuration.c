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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configuration.h"
#include "daemon.h"
#include "standard.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PORT_MAX         65535    /* Not in limits.h */
#define BUFFER_SIZE      PATH_MAX /* The longest possible value is the longest possible path */

static char *file_read_line(FILE *fh, const char *filename, unsigned int *lineno);
static bool str_equals_ignore_case(const char *string, const char *lowercase_string);
static bool str_to_bool(const char *string, const char *filename, const unsigned int lineno);
static void confcleanup(const int ret);

static FILE *fh;
static char *line;

void parse_config(const char *conf_file, struct options *opt)
{
    char key[BUFFER_SIZE], val[BUFFER_SIZE];
    char *keystr, *valstr;
    unsigned int lineno = 1;
    size_t vallen;

    fh = fopen(conf_file, "r");

    if (fh == NULL) {
        fprintf(stderr, "Unable to open configuration file (%s): %s\n", conf_file, strerror(errno));
        cleanup(1, true);
    }

#if DEBUG
    printf("Raw key/value pairs from config file:\n");
#endif /* DEBUG */

    while ((line = file_read_line(fh, conf_file, &lineno)) != NULL) {
        int i, j;
        char ch;

        /* Eat up whitespace */
        for (i = 0; isspace(line[i]); i++);

        /* Ignore comments and blank lines */
        if (line[i] == '\0' || line[i] == '#') {
            free(line);
            line = NULL;
            continue;
        }

        /* Read the key */
        for (j = 0; !isspace((ch = line[i++])); j++) {
            key[j] = ch;

            if (ch == '\0') {
                fprintf(stderr, "%s:%i: unexpected end of line.\n", conf_file, lineno);
                free(line);
                line = NULL;
                break;
            }
        }

        if (line == NULL) {
            continue;
        }

        key[j] = '\0';

        /* Eat up whitespace */
        for (; isspace(line[i]); i++);

        /* Read the value */
        for (j = 0; !isspace((ch = line[i++])); j++) {
            val[j] = ch;

            if (ch == '\0') {
                break;
            }
        }

        if (line == NULL) {
            continue;
        }

        free(line);
        line = NULL;

        keystr = (char *)key;
        valstr = (char *)val;

#if DEBUG
        printf("[%s] = [%s]\n", keystr, valstr);
#endif /* DEBUG */

        vallen = strlen(valstr);
        if (vallen > 1 && ((valstr[0] == '\'' && valstr[vallen - 1] == '\'') ||
                           (valstr[0] == '\"' && valstr[vallen - 1] == '\"'))) {
            /* Strip quotation marks from string */
            valstr[vallen - 1] = '\0';
            valstr++;
            vallen -= 2;

#if DEBUG
            printf("[%s] = [%s] <\n", keystr, valstr);
#endif /* DEBUG */
        }

        /* Parse each line */
        if (str_equals_ignore_case(keystr, "daemonize")) {
            opt->daemonize = str_to_bool(valstr, conf_file, lineno);
        } else if (str_equals_ignore_case(keystr, "transportprotocol")) {

        } else if (str_equals_ignore_case(keystr, "internetprotocol")) {
            if (str_equals_ignore_case(valstr, "both")) {
                opt->iproto = PROTOCOL_BOTH;
            } else if (str_equals_ignore_case(valstr, "ipv4")) {
                opt->iproto = PROTOCOL_IPv4;
            } else if (str_equals_ignore_case(valstr, "ipv6")) {
                opt->iproto = PROTOCOL_IPv6;
            } else {
                fprintf(stderr, "%s:%d: invalid protocol: \"%s\"\n", conf_file, lineno, valstr);
                confcleanup(1);
            }
        } else if (str_equals_ignore_case(keystr, "port")) {
            int port = atoi(valstr);
            if (0 >= port || port > PORT_MAX) {
                fprintf(stderr, "%s:%d: invalid port number: \"%s\"\n", conf_file, lineno, valstr);
                confcleanup(1);
            }

            opt->port = port;
        } else if (str_equals_ignore_case(keystr, "pidfile")) {
            char *ptr = malloc(vallen + 1);
            if (ptr == NULL) {
                perror("Unable to allocate memory for config value");
                confcleanup(1);
            }

            opt->pidfile = ptr;
            opt->pidmalloc = true;
            strcpy((char *)opt->pidfile, valstr);
        } else if (str_equals_ignore_case(keystr, "requirepidfile")) {
            opt->require_pidfile = str_to_bool(valstr, conf_file, lineno);
        } else if (str_equals_ignore_case(keystr, "quotesfile")) {
            char *ptr = malloc(vallen + 1);
            if (ptr == NULL) {
                perror("Unable to allocate memory for config value");
                confcleanup(1);
            }

            opt->quotesfile = ptr;
            opt->quotesmalloc = true;
            strcpy((char *)opt->quotesfile, valstr);
        } else if (str_equals_ignore_case(keystr, "quotedivider")) {
            if (str_equals_ignore_case(valstr, "line")) {
                opt->linediv = DIV_EVERYLINE;
            } else if (str_equals_ignore_case(valstr, "percent")) {
                opt->linediv = DIV_PERCENT;
            } else if (str_equals_ignore_case(valstr, "file")) {
                opt->linediv = DIV_WHOLEFILE;
            } else {
                fprintf(stderr, "%s:%d: unsupported division type: \"%s\"\n", conf_file, lineno, valstr);
                confcleanup(1);
            }
        } else if (str_equals_ignore_case(keystr, "dailyquotes")) {
            opt->is_daily = str_to_bool(valstr, conf_file, lineno);
        } else if (str_equals_ignore_case(keystr, "allowbigquotes")) {
            opt->allow_big = str_to_bool(valstr, conf_file, lineno);
        } else {
            fprintf(stderr, "%s:%d: ignoring unknown conf option: \"%s\"\n", conf_file, lineno, keystr);
        }

        lineno++;
    }

    fclose(fh);
}

static char *file_read_line(FILE *fh, const char *filename, unsigned int *lineno)
{
    char *buffer = malloc(BUFFER_SIZE);
    int ch, i;

    if (buffer == NULL) {
        perror("Unable to allocate memory for readline");
        confcleanup(1);
    }

    for (i = 0; i < BUFFER_SIZE; i++) {
        ch = fgetc(fh);

        if (ch == '\n') {
            if (i) {
                buffer[i] = '\0';
                return (char *)buffer;
            } else {
                /* Ignore empty lines */
                i--;
                (*lineno)++;
            }
        } else if (ch == EOF) {
            if (i) {
                /* Return rest of line when EOF hits before newline */
                buffer[i] = '\0';
                return (char *)buffer;
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

static bool str_equals_ignore_case(const char *string, const char *lowercase_string)
{
    size_t i;
    for (i = 0; string[i] || lowercase_string[i]; i++) {
        if (tolower(string[i]) != lowercase_string[i]) {
            return false;
        }
    }

    return !string[i] && !lowercase_string[i];
}

static bool str_to_bool(const char *string, const char *filename, const unsigned int lineno)
{
    if (str_equals_ignore_case(string, "yes") || str_equals_ignore_case(string, "true")) {
        return true;
    } else if (str_equals_ignore_case(string, "no") || str_equals_ignore_case(string, "false")) {
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
    cleanup(ret, true);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

