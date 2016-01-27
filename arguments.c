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

static void help_and_exit(const char* program_name);
static void usage_and_exit(const char* program_name);
static void version_and_exit(const char* program_name);

options* parse_args(const int argc, const char* argv[])
{
    const char* program_name = basename((char*)argv[0]);
    char* conf_file = "/etc/qotd.conf";
    char* pid_file = NULL;

    /* Parse arguments */
    int i;
    for (i = 1; i < argc; i++) {
        if (STREQ(argv[i], "--help")) {
            help_and_exit(program_name);
        } else if (STREQ(argv[i], "--version")) {
            version_and_exit(program_name);
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
        } else {
            usage_and_exit(program_name);
        }
    }

    options* opt = malloc(sizeof(options));
    opt->port = 17;
    opt->quotesfile = "/usr/share/qotd/quotes.txt";
    opt->pidfile = "/var/run/qotd.pid";
    opt->quotesmalloc = false;
    opt->pidmalloc = false;
    opt->is_daily = true;
    opt->allow_big = false;

    if (conf_file) {
        parse_config(conf_file, opt);
    }

    if (pid_file) {
        opt->pidfile = pid_file;
    }

    return opt;
}

static void help_and_exit(const char* program_name)
{
    printf("%s - A simple QOTD daemon.\n"
           "Usage: %s [-c config-file | -N]\n"
           " -c, --config (file)   Specify an alternate configuration file location. The default\n"
           "                       is at /etc/qotd.conf\n"
           " -N, --noconfig        Do not read from a configuration file.\n",
           program_name, program_name);
    cleanup(0);
}

static void usage_and_exit(const char* program_name)
{
    printf("Usage: %s [-c config-file | -N]\n",
           program_name);
    cleanup(1);
}

static void version_and_exit(const char* program_name)
{
    printf("%s - A simple QOTD daemon, version %s\n",
            program_name, VERSION_STRING);
    cleanup(0);
}

