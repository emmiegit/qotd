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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "arguments.h"
#include "configuration.h"
#include "daemon.h"
#include "info.h"
#include "options.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define STREQUALS(x, y)     (strcmp((x), (y)) == 0)
#define BOOLEAN_UNSET       2

#if DEBUG
# define DEBUG_BUFFER_SIZE 50
# define BOOLSTR(x) ((x) ? "true" : "false")
#endif /* DEBUG */

static char *default_pidfile();
static void help_and_exit(const char *program_name);
static void usage_and_exit(const char *program_name);
static void version_and_exit();

#if DEBUG
static const char *name_option_protocol(struct options *opt);
static const char *name_option_quote_divider(enum quote_divider value);
#endif /* DEBUG */

void parse_args(struct options *opt, const int argc, const char *argv[])
{
    const char *program_name = basename((char *)argv[0]);
    char *conf_file = "/etc/qotd.conf";
    const char *quotes_file = NULL;
    const char *pid_file = NULL;
    char daemonize = BOOLEAN_UNSET;
    enum transport_protocol tproto = PROTOCOL_TNONE;
    enum internet_protocol iproto = PROTOCOL_INONE;
    int i;

    opt->port = 17;
    opt->tproto = PROTOCOL_TCP;
    opt->iproto = PROTOCOL_BOTH;
    opt->quotesfile = "/usr/share/qotd/quotes.txt";
    opt->linediv = DIV_EVERYLINE;
    opt->pidfile = default_pidfile();
    opt->quotesmalloc = false;
    opt->pidmalloc = false;
    opt->daemonize = true;
    opt->is_daily = true;
    opt->allow_big = false;
    opt->chdir_root = true;

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (STREQUALS(argv[i], "--help")) {
            help_and_exit(program_name);
        } else if (STREQUALS(argv[i], "--version")) {
            version_and_exit();
        } else if (STREQUALS(argv[i], "-f") ||
                   STREQUALS(argv[i], "--foreground")) {
            daemonize = false;
        } else if (STREQUALS(argv[i], "-c") ||
                   STREQUALS(argv[i], "--config")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a configuration file.\n");
                cleanup(1, true);
            } else {
                conf_file = (char *)argv[i];
            }
        } else if (STREQUALS(argv[i], "-N") ||
                   STREQUALS(argv[i], "--noconfig")) {
            conf_file = NULL;
        } else if (STREQUALS(argv[i], "-P") ||
                   STREQUALS(argv[i], "--pidfile")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a pid file.\n");
                cleanup(1, true);
            } else {
                pid_file = argv[i];
            }
        } else if (STREQUALS(argv[i], "-s") ||
                   STREQUALS(argv[i], "--quotes")) {
            if (++i == argc) {
                fprintf(stderr, "You must specify a quotes file.\n");
                cleanup(1, true);
            } else {
                quotes_file = argv[i];
            }
        } else if (STREQUALS(argv[i], "-4") ||
                   STREQUALS(argv[i], "--ipv4")) {
            if (iproto == PROTOCOL_IPv6) {
                fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
                cleanup(1, true);
            }

            iproto = PROTOCOL_IPv4;
        } else if (STREQUALS(argv[i], "-6") ||
                   STREQUALS(argv[i], "--ipv6")) {
            if (opt->iproto == PROTOCOL_IPv4) {
                fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
                cleanup(1, true);
            }

            iproto = PROTOCOL_IPv6;
        } else if (STREQUALS(argv[i], "-U") ||
                   STREQUALS(argv[i], "--udp")) {
            tproto = PROTOCOL_UDP;
        } else {
            printf("Unrecognized option: %s.\n", argv[i]);
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
            free((char *)opt->pidfile);
        }

        opt->pidfile = pid_file;
        opt->pidmalloc = false;
    }

    if (quotes_file) {
        if (opt->quotesmalloc) {
            free((char *)opt->quotesfile);
        }

        opt->quotesfile = quotes_file;
        opt->quotesmalloc = false;
    }

    if (iproto != PROTOCOL_INONE) {
        opt->iproto = iproto;
    }

    if (tproto != PROTOCOL_TNONE) {
        opt->tproto = tproto;
    }

    if (daemonize != BOOLEAN_UNSET) {
        opt->daemonize = daemonize;
    }

#if DEBUG
    printf("\nContents of struct 'opt':\n");
    printf("opt = {\n");
    printf("  Daemonize: %s\n",       BOOLSTR(opt->daemonize));
    printf("  Protocol: %s\n",        name_option_protocol(opt));
    printf("  Port: %d\n",            opt->port);
    printf("  QuotesMalloc: %s\n",    BOOLSTR(opt->quotesmalloc));
    printf("  PidMalloc: %s\n",       BOOLSTR(opt->pidmalloc));
    printf("  QuotesFile: %s\n",      opt->quotesfile);
    printf("  QuoteDivider: %s\n",    name_option_quote_divider(opt->linediv));
    printf("  PidFile: %s\n",         opt->pidfile);
    printf("  RequirePidFile: %s\n",  BOOLSTR(opt->require_pidfile));
    printf("  DailyQuotes: %s\n",     BOOLSTR(opt->is_daily));
    printf("  AllowBigQuotes: %s\n",  BOOLSTR(opt->allow_big));
    printf("}\n\n");
#endif /* DEBUG */
}

static char *default_pidfile()
{
    struct stat statbuf;
    int ret = stat("/run", &statbuf);

    return (ret == 0) ? "/run/qotd.pid" : "/var/run/qotd.pid";
}

#if DEBUG
static const char *name_option_protocol(struct options *opt)
{
    switch (opt->tproto) {
        case PROTOCOL_TCP:
            switch (opt->iproto) {
                case PROTOCOL_IPv4:
                    return "TCP IPv4 only";
                case PROTOCOL_IPv6:
                    return "TCP IPv6 only";
                case PROTOCOL_BOTH:
                    return "TCP IPv4 and IPv6";
                default:
                    return "TCP ???";
            }
        case PROTOCOL_UDP:
            switch (opt->iproto) {
                case PROTOCOL_IPv4:
                    return "UDP IPv4 only";
                case PROTOCOL_IPv6:
                    return "UDP IPv6 only";
                case PROTOCOL_BOTH:
                    return "UDP IPv4 and IPv6";
                default:
                    return "UDP ???";
            }
        default:
            return "???";
    }
}

static const char *name_option_quote_divider(enum quote_divider value)
{
    switch (value) {
        case DIV_EVERYLINE:
            return "DIV_EVERYLINE";
        case DIV_PERCENT:
            return "DIV_PERCENT";
        case DIV_WHOLEFILE:
            return "DIV_WHOLEFILE";
        default:
            printf("(%u) ", value);
            return "UNKNOWN";
    }
}
#endif /* DEBUG */

static void help_and_exit(const char *program_name)
{
    printf("%s - A simple QOTD daemon.\n"
           "Usage: %s [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file] [-4 | -6] [-U]\n"
           "Usage: %s [--help | --version]\n"
           " -f, --foreground      Do not fork, but run in the foreground.\n"
           " -c, --config (file)   Specify an alternate configuration file location. The default\n"
           "                       is at /etc/qotd.conf\n"
           " -N, --noconfig        Do not read from a configuration file, but use the default\n"
           "                       options instead.\n",
           PROGRAM_NAME, program_name, program_name);
    printf(" -P, --pidfile (file)  Override the pidfile name given in the configuration file with\n"
           "                       the given file instead.\n"
           " -s, --quotes (file)   Override the quotes file given in the configuration file with\n"
           "                       the given filename instead.\n"
           " -4, --ipv4            Only listen on IPv4.\n"
           " -6, --ipv6            Only listen on IPv6.\n"
           " -U, --udp             Use UDP instead of TCP/IP.\n");
    printf(" --help                List all options and what they do.\n"
           " --version             Print the version and some basic license information.\n");
    cleanup(0, false);
}

static void usage_and_exit(const char *program_name)
{
    printf("Usage: %s [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file] [-4 | -6] [-U]\n",
           program_name);
    cleanup(1, false);
}

static void version_and_exit()
{
    printf("%s - A simple QOTD daemon, version %s\n"
           "Copyright (C) 2015-2016 Ammon Smith\n"
           "\n"
           "%s is free software: you can redistribute it and/or modify\n"
           "it under the terms of the GNU General Public License as published by\n"
           "the Free Software Foundation, either version 2 of the License, or\n"
           "(at your option) any later version.\n",
            PROGRAM_NAME, VERSION_STRING, PROGRAM_NAME);
    cleanup(0, false);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
