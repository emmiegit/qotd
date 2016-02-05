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

#if DEBUG == 1
# define NULLSTR(x) ((x) ? (x) : "<null>")
# define BOOLSTR(x) ((x) ? "true" : "false")
static void print_options(options* opt);
#endif /* DEBUG */

static void help_and_exit(const char* program_name);
static void usage_and_exit(const char* program_name);
static void version_and_exit(const char* program_name);

options* parse_args(const int argc, const char* argv[])
{
    const char* program_name = basename((char*)argv[0]);
    char* conf_file = "/etc/qotd.conf";
    char* quotes_file = NULL;
    char* pid_file = NULL;
    char daemonize = BOOLEAN_UNSET;

    options* opt = malloc(sizeof(options));
    opt->port = 17;
    opt->quotesfile = "/usr/share/qotd/quotes.txt";
    opt->linediv = DIV_EVERYLINE;
    opt->pidfile = "/var/run/qotd.pid";
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
                conf_file = (char*)argv[i];
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
                pid_file = (char*)argv[i];
            }
        } else if (STREQ(argv[i], "-s") ||
                   STREQ(argv[i], "--quotes")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a quotes file.\n");
                cleanup(1);
            } else {
                quotes_file = (char*)argv[i];
            }
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

    if (daemonize != BOOLEAN_UNSET) {
        opt->daemonize = daemonize;
    }

#if DEBUG == 1
    print_options(opt);
#endif /* DEBUG */

    return opt;
}

#if DEBUG == 1
static void print_options(options* opt)
{
    printf("\nContents of struct 'opt':\n");
    printf("Daemonize: %s\n", BOOLSTR(opt->daemonize));
    printf("Port: %d\n", opt->port);
    printf("QuotesMalloc: %s\n", BOOLSTR(opt->quotesmalloc));
    printf("PidMalloc: %s\n", BOOLSTR(opt->pidmalloc));
    printf("QuotesFile: %s\n", NULLSTR(opt->quotesfile));
    printf("QuoteDivider: %d\n", opt->linediv);
    printf("PidFile: %s\n", NULLSTR(opt->pidfile));
    printf("RequirePidFile: %s\n", BOOLSTR(opt->require_pidfile));
    printf("DailyQuotes: %s\n", BOOLSTR(opt->is_daily));
    printf("AllowBigQuotes: %s\n", BOOLSTR(opt->allow_big));
    printf("End of 'opt'.\n\n");
}
#endif /* DEBUG */

static void help_and_exit(const char* program_name)
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
           "                       the given filename instead.\n",
           program_name, program_name);
    cleanup(0);
}

static void usage_and_exit(const char* program_name)
{
    printf("Usage: %s [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file]\n",
           program_name);
    cleanup(1);
}

static void version_and_exit(const char* program_name)
{
    printf("%s - A simple QOTD daemon, version %s\n"
           "Copyright (C) 2015-2016 Ammon Smith\n"
           "\n"
           "qotd is free software: you can redistribute it and/or modify\n"
           "it under the terms of the GNU General Public License as published by\n"
           "the Free Software Foundation, either version 2 of the License, or\n"
           "(at your option) any later version.\n",
            program_name, VERSION_STRING);
    cleanup(0);
}

