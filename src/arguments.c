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
#include "journal.h"
#include "options.h"
#include "standard.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BOOLEAN_UNSET       2

#if DEBUG
# define DEBUG_BUFFER_SIZE 50
# define BOOLSTR(x) ((x) ? "true" : "false")
#endif /* DEBUG */

struct argument_flags {
    const char *program_name;
    const char *conf_file;
    const char *quotes_file;
    const char *pid_file;
    const char *journal_file;
    char daemonize;
    enum transport_protocol tproto;
    enum internet_protocol iproto;
};

static void parse_short_options(const char *argument, const char *next_arg, struct argument_flags *flags);
static void parse_long_option(const int argc, const char *argv[], int *i, struct argument_flags *flags);
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
    struct argument_flags flags;
    int i;

    /* Set override flags */
    flags.program_name = basename((char *)argv[0]),
    flags.conf_file = NULL;
    flags.quotes_file = NULL,
    flags.pid_file = NULL,
    flags.journal_file = NULL,
    flags.daemonize = BOOLEAN_UNSET,
    flags.tproto = PROTOCOL_TNONE,
    flags.iproto = PROTOCOL_INONE,

    /* Set default options */
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
        if (strncmp(argv[i], "--", 2) == 0) {
            parse_long_option(argc, argv, &i, &flags);
        } else if (argv[i][0] == '-') {
            const char *next_arg = (i + 1 == argc) ? argv[i + 1] : NULL;
            parse_short_options(argv[i] + 1, next_arg, &flags);
        } else {
            printf("Unrecognized option: %s.\n", argv[i]);
            usage_and_exit(flags.program_name);
        }
    }

    /* Override config file options */
    if (flags.conf_file) {
        if (flags.conf_file[0] != '/') {
            opt->chdir_root = false;
        }

        parse_config(flags.conf_file, opt);
    }

    if (flags.pid_file) {
        if (opt->pidmalloc) {
            free((char *)opt->pidfile);
        }

        opt->pidfile = flags.pid_file;
        opt->pidmalloc = false;
    }

    if (flags.quotes_file) {
        if (opt->quotesmalloc) {
            free((char *)opt->quotesfile);
        }

        opt->quotesfile = flags.quotes_file;
        opt->quotesmalloc = false;
    }

    if (flags.journal_file) {
        close_journal();
        open_journal(flags.journal_file);
    }

    if (flags.iproto != PROTOCOL_INONE) {
        opt->iproto = flags.iproto;
    }

    if (flags.tproto != PROTOCOL_TNONE) {
        opt->tproto = flags.tproto;
    }

    if (flags.daemonize != BOOLEAN_UNSET) {
        opt->daemonize = flags.daemonize;
    }

#if DEBUG
    printf("\nContents of struct 'opt':\n");
    printf("opt = {\n");
    printf("    Daemonize: %s\n",       BOOLSTR(opt->daemonize));
    printf("    Protocol: %s\n",        name_option_protocol(opt));
    printf("    Port: %d\n",            opt->port);
    printf("    QuotesMalloc: %s\n",    BOOLSTR(opt->quotesmalloc));
    printf("    PidMalloc: %s\n",       BOOLSTR(opt->pidmalloc));
    printf("    QuotesFile: %s\n",      opt->quotesfile);
    printf("    QuoteDivider: %s\n",    name_option_quote_divider(opt->linediv));
    printf("    PidFile: %s\n",         opt->pidfile);
    printf("    RequirePidFile: %s\n",  BOOLSTR(opt->require_pidfile));
    printf("    DailyQuotes: %s\n",     BOOLSTR(opt->is_daily));
    printf("    AllowBigQuotes: %s\n",  BOOLSTR(opt->allow_big));
    printf("}\n\n");
#endif /* DEBUG */
}

static void parse_short_options(const char *argument, const char *next_arg, struct argument_flags *flags)
{
    size_t i;

    for (i = 0; argument[i]; i++) {
        switch (argument[i]) {
            case 'f':
                flags->daemonize = false;
                break;
            case 'c':
                if (next_arg == NULL) {
                    fprintf(stderr, "You must specify a configuration file.\n");
                    cleanup(1, true);
                }

                flags->conf_file = next_arg;
                break;
            case 'N':
                flags->conf_file = NULL;
            case 'P':
                if (next_arg == NULL) {
                    fprintf(stderr, "You must specify a pid file.\n");
                    cleanup(1, true);
                }

                flags->pid_file = next_arg;
                break;
            case 's':
                if (next_arg == NULL) {
                    fprintf(stderr, "You must specify a quotes file.\n");
                    cleanup(1, true);
                }

                flags->quotes_file = next_arg;
                break;
            case 'j':
                if (next_arg == NULL) {
                    fprintf(stderr, "You must specify a journal file.\n");
                    cleanup(1, true);
                }

                flags->journal_file = next_arg;
                break;
            case '4':
                if (flags->iproto == PROTOCOL_IPv6) {
                    fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
                    cleanup(1, true);
                }

                flags->iproto = PROTOCOL_IPv4;
                break;
            case '6':
                if (flags->iproto == PROTOCOL_IPv4) {
                    fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
                    cleanup(1, true);
                }

                flags->iproto = PROTOCOL_IPv6;
            case 'T':
                if (flags->tproto == PROTOCOL_UDP) {
                    fprintf(stderr, "Conflicting options passed: -T and -U.\n");
                    cleanup(1, true);
                }

                flags->tproto = PROTOCOL_TCP;
                break;
            case 'U':
                if (flags->tproto == PROTOCOL_TCP) {
                    fprintf(stderr, "Conflicting options passed: -T and -U.\n");
                    cleanup(1, true);
                }

                flags->tproto = PROTOCOL_UDP;
                break;
            case 'q':
                close_journal();
                break;
            default:
                fprintf(stderr, "Unknown short option: -%c.\n", argument[i]);
                usage_and_exit(flags->program_name);
        }
    }
}

static void parse_long_option(const int argc, const char *argv[], int *i, struct argument_flags *flags)
{
    if (strcmp(argv[*i], "--help") == 0) {
        help_and_exit(flags->program_name);
    } else if (strcmp(argv[*i], "--version") == 0) {
        version_and_exit();
    } else if (strcmp(argv[*i], "--foreground") == 0) {
        flags->daemonize = false;
    } else if (strcmp(argv[*i], "--config") == 0) {
        if (++(*i) == argc) {
            fprintf(stderr, "You must specify a configuration file.\n");
            cleanup(1, true);
        }

        flags->conf_file = argv[*i];
    } else if (strcmp(argv[*i], "--noconfig") == 0) {
        flags->conf_file = NULL;
    } else if (strcmp(argv[*i], "--pidfile") == 0) {
        if (++(*i) == argc) {
            fprintf(stderr, "You must specify a pid file.\n");
            cleanup(1, true);
        }

        flags->pid_file = argv[*i];
    } else if (strcmp(argv[*i], "--quotes") == 0) {
        if (++(*i) == argc) {
            fprintf(stderr, "You must specify a quotes file.\n");
            cleanup(1, true);
        }

        flags->quotes_file = argv[*i];
    } else if (strcmp(argv[*i], "--journal") == 0) {
        if (++(*i) == argc) {
            fprintf(stderr, "You must specify a journal file.\n");
            cleanup(1, true);
        }

        flags->journal_file = argv[*i];
    } else if (strcmp(argv[*i], "--ipv4") == 0) {
        if (flags->iproto == PROTOCOL_IPv6) {
            fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
            cleanup(1, true);
        }

        flags->iproto = PROTOCOL_IPv4;
    } else if (strcmp(argv[*i], "--ipv6") == 0) {
        if (flags->iproto == PROTOCOL_IPv4) {
            fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
            cleanup(1, true);
        }

        flags->iproto = PROTOCOL_IPv6;
    } else if (strcmp(argv[*i], "--tcp") == 0) {
        if (flags->tproto == PROTOCOL_UDP) {
            fprintf(stderr, "Conflicting options passed: -T and -U.\n");
            cleanup(1, true);
        }

        flags->tproto = PROTOCOL_TCP;
    } else if (strcmp(argv[*i], "--udp") == 0) {
        if (flags->tproto == PROTOCOL_TCP) {
            fprintf(stderr, "Conflicting options passed: -T and -U.\n");
            cleanup(1, true);
        }

        flags->tproto = PROTOCOL_UDP;
    } else if (strcmp(argv[*i], "--quiet") == 0) {
        close_journal();
    } else {
        printf("Unrecognized long option: %s.\n", argv[*i]);
        usage_and_exit(flags->program_name);
    }
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
    /* Split into sections to comply with -pedantic */
    printf("%s - A simple QOTD daemon.\n"
           "Usage: %s [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file] [-4 | -6] [-T | -U] [-q]\n"
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
           " -j, --journal (file)  Override the journal file given in the configuration file with\n"
           "                       the given filename instead.\n"
           " -4, --ipv4            Only listen on IPv4. The default behavior is to listen on both\n");
    printf("                       IPv4 and IPv6.\n"
           " -6, --ipv6            Only listen on IPv6.\n"
           " -T, --tcp             Use TCP. This is the default behavior.\n"
           " -U, --udp             Use UDP instead of TCP. (Not fully implemented yet)\n"
           " -q, --quiet           Only output error messages. This is the same as using\n"
           "                       \"--journal /dev/null\".\n"
           " --help                List all options and what they do.\n"
           " --version             Print the version and some basic license information.\n");
    cleanup(0, false);
}

static void usage_and_exit(const char *program_name)
{
    printf("Usage: %s [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file] [-4 | -6] [-T | -U] [-q]\n",
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
