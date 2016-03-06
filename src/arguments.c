/*
 * arguments.c
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

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "config.h"
#include "info.h"
#include "options.h"
#include "qotdd.h"

#define STREQ(x, y) (strcmp((x), (y)) == 0)
#define BOOLEAN_UNSET 2

#if DEBUG
# define DEBUG_BUFFER_SIZE 50
# define NULLSTR(x) ((x) ? (x) : "<null>")
# define BOOLSTR(x) ((x) ? "true" : "false")
#endif /* DEBUG */

static void help_and_exit(const char *program_name);
static void usage_and_exit(const char *program_name);
static void version_and_exit(const char *program_name);

#if DEBUG
static const char *name_option_protocol(const unsigned char value);
static const char *name_option_quote_divider(const unsigned char value);
int snprintf(char *str, size_t size, const char *format, ...);
#endif /* DEBUG */

options *parse_args(const int argc, const char *argv[])
{
    const char *program_name = basename((char *)argv[0]);
    char *conf_file = "/etc/qotd.conf";
    char *quotes_file = NULL;
    char *pid_file = NULL;
    char daemonize = BOOLEAN_UNSET;
    char protocol = 0;

    options *opt = malloc(sizeof(options));
    opt->port = 17;
    opt->protocol = PROTOCOL_BOTH;
    opt->quotesfile = "/usr/share/qotd/quotes.txt";
    opt->linediv = DIV_EVERYLINE;
    opt->pidfile = "/run/qotd.pid";
    opt->quotesmalloc = false;
    opt->pidmalloc = false;
    opt->daemonize = true;
    opt->is_daily = true;
    opt->allow_big = false;
    opt->chdir_root = true;

    /* Parse arguments */
    int i;
    for (i = 1; i < argc; i++) {
        if (STREQ(argv[i], "--help")) {
            help_and_exit(program_name);
        } else if (STREQ(argv[i], "--version")) {
            version_and_exit(program_name);
        } else if (STREQ(argv[i], "-f") ||
                   STREQ(argv[i], "--foreground")) {
            daemonize = false;
        } else if (STREQ(argv[i], "-c") ||
                   STREQ(argv[i], "--config")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a configuration file.\n");
                cleanup(1);
            } else {
                conf_file = (char *)argv[i];
            }
        } else if (STREQ(argv[i], "-N") ||
                   STREQ(argv[i], "--noconfig")) {
            conf_file = NULL;
        } else if (STREQ(argv[i], "-P") ||
                   STREQ(argv[i], "--pidfile")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a pid file.\n");
                cleanup(1);
            } else {
                pid_file = (char *)argv[i];
            }
        } else if (STREQ(argv[i], "-s") ||
                   STREQ(argv[i], "--quotes")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a quotes file.\n");
                cleanup(1);
            } else {
                quotes_file = (char *)argv[i];
            }
        } else if (STREQ(argv[i], "-4") ||
                   STREQ(argv[i], "--ipv4")) {
            if (protocol == PROTOCOL_IPV6) {
                fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
                cleanup(1);
            }

            protocol = PROTOCOL_IPV4;
        } else if (STREQ(argv[i], "-6") ||
                   STREQ(argv[i], "--ipv6")) {
            if (protocol == PROTOCOL_IPV4) {
                fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
                cleanup(1);
            }

            protocol = PROTOCOL_IPV6;
        } else {
            usage_and_exit(program_name);
        }
    }

    if (conf_file) {
        if (conf_file[0] != '/') {
            opt->chdir_root = false;
        }

        parse_config(conf_file, opt);
    }

    if (pid_file) {
        if (opt->pidmalloc) {
            free(opt->pidfile);
        }

        opt->pidfile = pid_file;
        opt->pidmalloc = false;
    }

    if (quotes_file) {
        if (opt->quotesmalloc) {
            free(opt->quotesfile);
        }

        opt->quotesfile = quotes_file;
        opt->quotesmalloc = false;
    }

    if (protocol) {
        opt->protocol = protocol;
    }

    if (daemonize != BOOLEAN_UNSET) {
        opt->daemonize = daemonize;
    }

#if DEBUG
    printf("\nContents of struct 'opt':\n");
    printf("Daemonize: %s\n", BOOLSTR(opt->daemonize));
    printf("Protocol: %s\n", name_option_protocol(opt->protocol));
    printf("Port: %d\n", opt->port);
    printf("QuotesMalloc: %s\n", BOOLSTR(opt->quotesmalloc));
    printf("PidMalloc: %s\n", BOOLSTR(opt->pidmalloc));
    printf("QuotesFile: %s\n", NULLSTR(opt->quotesfile));
    printf("QuoteDivider: %s\n", name_option_quote_divider(opt->linediv));
    printf("PidFile: %s\n", NULLSTR(opt->pidfile));
    printf("RequirePidFile: %s\n", BOOLSTR(opt->require_pidfile));
    printf("DailyQuotes: %s\n", BOOLSTR(opt->is_daily));
    printf("AllowBigQuotes: %s\n", BOOLSTR(opt->allow_big));
    printf("End of 'opt'.\n\n");
#endif /* DEBUG */

    return opt;
}

#if DEBUG
static const char *name_option_protocol(const unsigned char value)
{
    char *buf;
    switch (value) {
        case PROTOCOL_IPV4: return "PROTOCOL_IPV4";
        case PROTOCOL_IPV6: return "PROTOCOL_IPV6";
        case PROTOCOL_BOTH: return "PROTOCOL_BOTH";
        default:
            /* Who cares about memory leaks in debugging code? */
            buf = malloc(DEBUG_BUFFER_SIZE * sizeof(char));
            snprintf(buf, DEBUG_BUFFER_SIZE, "(unknown: %zd)", value);
            return buf;
    }
}

static const char *name_option_quote_divider(const unsigned char value)
{
    char *buf;
    switch (value) {
        case DIV_EVERYLINE: return "DIV_EVERYLINE";
        case DIV_PERCENT:   return "DIV_PERCENT";
        case DIV_WHOLEFILE: return "DIV_WHOLEFILE";
        default:
            buf = malloc(DEBUG_BUFFER_SIZE * sizeof(char));
            snprintf(buf, DEBUG_BUFFER_SIZE, "(unknown: %zd)", value);
            return buf;
    }
}
#endif /* DEBUG */

static void help_and_exit(const char *program_name)
{
    printf("%s - A simple QOTD daemon.\n"
           "Usage: %s [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file]\n"
           " -f, --foreground      Do not fork, but run in the foreground.\n"
           " -c, --config (file)   Specify an alternate configuration file location. The default\n"
           "                       is at /etc/qotd.conf\n"
           " -N, --noconfig        Do not read from a configuration file, but use the default\n"
           "                       options instead.\n"
           " -P, --pidfile (file)  Override the pidfile name given in the configuration file with\n"
           "                       the given file instead.\n"
           " -s, --quotes (file)   Override the quotes file given in the configuration file with\n"
           "                       the given filename instead.\n"
           " -4, --ipv4            Only listen on IPv4.\n"
           " -6, --ipv6            Only listen on IPv6.\n",
           program_name, program_name);
    quietcleanup(0);
}

static void usage_and_exit(const char *program_name)
{
    printf("Usage: %s [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file]\n",
           program_name);
    quietcleanup(1);
}

static void version_and_exit(const char *program_name)
{
    printf("%s - A simple QOTD daemon, version %s\n"
           "Copyright (C) 2015-2016 Ammon Smith\n"
           "\n"
           "qotd is free software: you can redistribute it and/or modify\n"
           "it under the terms of the GNU General Public License as published by\n"
           "the Free Software Foundation, either version 2 of the License, or\n"
           "(at your option) any later version.\n",
            program_name, VERSION_STRING);
    quietcleanup(0);
}

