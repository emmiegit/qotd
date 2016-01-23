/*
 * arguments.c
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

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "config.h"
#include "qotdd.h"

#define STREQ(x, y) (!strcmp((x), (y)))

static char* program_name;

static void help_and_exit();
static void usage_and_exit();
static void version_and_exit();

options* parse_args(const int argc, char* argv[])
{
    program_name = basename(argv[0]);
    char* conf_file = "/etc/qotd.conf";

    /* Parse arguments */
    int i;
    for (i = 1; i < argc; i++) {
        if (STREQ(argv[i], "--help")) {
            help_and_exit();
        } else if (STREQ(argv[i], "--version")) {
            version_and_exit();
        } else if (STREQ(argv[i], "-c") ||
                   STREQ(argv[i], "--config")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a configuration file.\n");
                cleanup(1);
            } else {
                conf_file = argv[i];
            }
        } else if (STREQ(argv[i], "-N") ||
                   STREQ(argv[i], "--noconfig")) {
            conf_file = NULL;
        } else {
            usage_and_exit();
        }
    }

    options* opt = malloc(sizeof(options));

    if (conf_file) {
        parse_config(conf_file, opt);
    } else {
        /* No config file, use default options */
        opt->port = 17;
        opt->quotesfile = "/usr/share/qotd/quotes.txt";
        opt->delimeter = DELIM_NEWLINE;
        opt->is_daily = true;
        opt->allow_big = false;
    }

    return opt;
}

static void help_and_exit()
{
    printf("%s - A simple QOTD server.\n"
           "Usage: %s [-c config-file | -N]\n"
           " -c, --config (file)   Specify an alternate configuration file location. The default\n"
           "                       is at /etc/qotd.conf\n"
           " -N, --noconfig        Do not read from a configuration file.\n",
           program_name, program_name);
    cleanup(0);
}

static void usage_and_exit()
{
    printf("Usage: %s [-c config-file | -N]\n",
           program_name);
    cleanup(1);
}

static void version_and_exit()
{
    printf("%s - A simple QOTD server, version 0.1\n",
            program_name);
    cleanup(0);
}

