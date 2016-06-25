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
#include <strings.h>
#include <unistd.h>

#include "configuration.h"
#include "daemon.h"
#include "journal.h"
#include "standard.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PORT_MAX         65535    /* Not in limits.h */
#define BUFFER_SIZE      PATH_MAX /* The longest possible value is the longest possible path */

static char *file_read_line(FILE *fh, const char *filename, unsigned int *lineno);
static bool str_to_bool(const char *string, const char *filename, const unsigned int lineno, bool *success);

void parse_config(const char *conf_file, struct options *opt)
{
    FILE *fh;
    char key[BUFFER_SIZE], val[BUFFER_SIZE];
    char *keystr, *valstr, *line;
    unsigned int lineno = 1;
    size_t vallen;

    fh = fopen(conf_file, "r");

    if (fh == NULL) {
        /* Can't write to the journal, it hasn't been opened yet */
        fprintf(stderr, "Unable to open configuration file (%s): %s\n", conf_file, strerror(errno));
        cleanup(1, true);
    }

#if DEBUG
    printf("Raw key/value pairs from config file:\n");
#endif /* DEBUG */

    while ((line = file_read_line(fh, conf_file, &lineno)) != NULL) {
        int i, j;
        char ch;
        bool success;

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
        if (strcasecmp(keystr, "daemonize") == 0) {
            ch = str_to_bool(valstr, conf_file, lineno, &success);

            if (likely(success)) {
                opt->daemonize = ch;
            }
        } else if (strcasecmp(keystr, "transportprotocol") == 0) {
            if (strcasecmp(valstr, "tcp") == 0) {
                opt->tproto = PROTOCOL_TCP;
            } else if (strcasecmp(valstr, "udp") == 0) {
                opt->tproto = PROTOCOL_UDP;
            } else {
                fprintf(stderr, "%s:%d: invalid transport protocol: \"%s\"\n", conf_file, lineno, valstr);
            }
        } else if (strcasecmp(keystr, "internetprotocol") == 0) {
            if (strcasecmp(valstr, "both") == 0) {
                opt->iproto = PROTOCOL_BOTH;
            } else if (strcasecmp(valstr, "ipv4") == 0) {
                opt->iproto = PROTOCOL_IPv4;
            } else if (strcasecmp(valstr, "ipv6") == 0) {
                opt->iproto = PROTOCOL_IPv6;
            } else {
                fprintf(stderr, "%s:%d: invalid internet protocol: \"%s\"\n", conf_file, lineno, valstr);
            }
        } else if (strcasecmp(keystr, "port") == 0) {
            /* atoi is ok because it returns 0 in case of failure, and 0 isn't a valid port */
            int port = atoi(valstr);
            if (0 >= port || port > PORT_MAX) {
                fprintf(stderr, "%s:%d: invalid port number: \"%s\"\n", conf_file, lineno, valstr);
            } else {
                opt->port = port;
            }
        } else if (strcasecmp(keystr, "dropprivileges") == 0) {
            ch = str_to_bool(valstr, conf_file, lineno, &success);

            if (likely(success)) {
                opt->drop_privileges = ch;
            }
        } else if (strcasecmp(keystr, "pidfile") == 0) {
            char *ptr;

            if (strcmp(valstr, "none") == 0 || strcmp(valstr, "/dev/null") == 0) {
                opt->pidfile = NULL;
                opt->pidmalloc = false;
                continue;
            }

            ptr = malloc(vallen + 1);
            if (ptr == NULL) {
                perror("Unable to allocate memory for config value");
                fclose(fh);
                cleanup(1, true);
            }

            opt->pidfile = ptr;
            opt->pidmalloc = true;
            strcpy((char *)opt->pidfile, valstr);
        } else if (strcasecmp(keystr, "requirepidfile") == 0) {
            ch = str_to_bool(valstr, conf_file, lineno, &success);

            if (likely(success)) {
                opt->require_pidfile = ch;
            }
        } else if (strcasecmp(keystr, "journalfile") == 0) {
            close_journal();

            if (strcmp(valstr, "-") == 0) {
                open_journal_as_fd(STDOUT_FILENO);
            } else if (strcmp(valstr, "none") != 0) {
                open_journal(valstr);
            }
        } else if (strcasecmp(keystr, "quotesfile") == 0) {
            char *ptr = malloc(vallen + 1);
            if (ptr == NULL) {
                perror("Unable to allocate memory for config value");
            }

            opt->quotesfile = ptr;
            opt->quotesmalloc = true;
            strcpy((char *)opt->quotesfile, valstr);
        } else if (strcasecmp(keystr, "quotedivider") == 0) {
            if (strcasecmp(valstr, "line") == 0) {
                opt->linediv = DIV_EVERYLINE;
            } else if (strcasecmp(valstr, "percent") == 0) {
                opt->linediv = DIV_PERCENT;
            } else if (strcasecmp(valstr, "file") == 0) {
                opt->linediv = DIV_WHOLEFILE;
            } else {
                fprintf(stderr, "%s:%d: unsupported division type: \"%s\"\n", conf_file, lineno, valstr);
            }
        } else if (strcasecmp(keystr, "dailyquotes") == 0) {
            ch = str_to_bool(valstr, conf_file, lineno, &success);

            if (likely(success)) {
                opt->is_daily = ch;
            }
        } else if (strcasecmp(keystr, "allowbigquotes") == 0) {
            ch = str_to_bool(valstr, conf_file, lineno, &success);

            if (likely(success)) {
                opt->allow_big = ch;
            }
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
        cleanup(1, true);
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
    cleanup(1, true);

    /* Should never reach here */
    return NULL;
}

static bool str_to_bool(const char *string, const char *filename, const unsigned int lineno, bool *success)
{
    if (strcasecmp(string, "yes") == 0
            || strcasecmp(string, "true") == 0
            || strcmp(string, "1") == 0) {
        *success = true;
        return true;
    } else if (strcasecmp(string, "no") == 0
            || strcasecmp(string, "false") == 0
            || strcmp(string, "0") == 0) {
        *success = true;
        return false;
    } else {
        fprintf(stderr, "%s:%d: not a boolean value: \"%s\"\n", filename, lineno, string);
        *success = false;
        return false;
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

