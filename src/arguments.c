/*
 * arguments.c
 *
 * qotd - A simple QOTD daemon.
 * Copyright (c) 2015-2016 Emmie Smith
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

#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "config.h"
#include "core.h"
#include "daemon.h"
#include "journal.h"

#define BOOLEAN_UNSET		2u

#if DEBUG
# define DEBUG_BUFFER_SIZE	48
# define BOOLSTR(x)		\
	((x) ? "true" : "false")
# define BOOLSTR2(x)		\
	(((x) == BOOLEAN_UNSET) ? "(unset)" : ((x) ? "true" : "false"))

static const char *name_option_protocol(enum transport_protocol tproto, enum internet_protocol iproto)
{
	switch (tproto) {
	case PROTOCOL_TCP:
		switch (iproto) {
		case PROTOCOL_IPv4:
			return "TCP IPv4 only";
		case PROTOCOL_IPv6:
			return "TCP IPv6 only";
		case PROTOCOL_BOTH:
			return "TCP IPv4 and IPv6";
		case PROTOCOL_INONE:
			return "TCP <UNSET>";
		default:
			return "TCP ???";
		}
	case PROTOCOL_UDP:
		switch (iproto) {
		case PROTOCOL_IPv4:
			return "UDP IPv4 only";
		case PROTOCOL_IPv6:
			return "UDP IPv6 only";
		case PROTOCOL_BOTH:
			return "UDP IPv4 and IPv6";
		case PROTOCOL_INONE:
			return "UDP <UNSET>";
		default:
			return "UDP ???";
		}
	case PROTOCOL_TNONE:
		switch (iproto) {
		case PROTOCOL_IPv4:
			return "<UNSET> IPv4 only";
		case PROTOCOL_IPv6:
			return "<UNSET> IPv6 only";
		case PROTOCOL_BOTH:
			return "<UNSET> IPv4 and IPv6";
		case PROTOCOL_INONE:
			return "<UNSET> <UNSET>";
		default:
			return "<UNSET> ???";
		}
	default:
		return "???";
	}
}

static const char *name_option_quote_divider(enum quote_divider value)
{
	static char buf[32];

	switch (value) {
	case DIV_EVERYLINE:
		return "DIV_EVERYLINE";
	case DIV_PERCENT:
		return "DIV_PERCENT";
	case DIV_WHOLEFILE:
		return "DIV_WHOLEFILE";
	default:
		sprintf(buf, "(%u) UNKNOWN", value);
		return buf;
	}
}
#endif /* DEBUG */

struct argument_flags {
	char _program_name[PATH_MAX];
	const char *program_name;
	const char *conf_file;
	const char *quotes_file;
	const char *pid_file;
	const char *journal_file;
	enum transport_protocol tproto;
	enum internet_protocol iproto;
	unsigned daemonize : 2;
	unsigned strict    : 1;
};

static const char *default_pidfile(void)
{
	access("/run", F_OK);
	return (errno == ENOENT)
		? "/var/run/qotd.pid"
		: "/run/qotd.pid";
}

static void print_usage(const char *program_name)
{
	printf("Usage: %s [OPTION]...\n"
	       "Usage: %s [--help | --version]\n\n",
		program_name, program_name);
}

static void help_and_exit(const char *program_name)
{
	/* Split into sections to comply with -pedantic */

	print_usage(program_name);
	fputs("  -f, --foreground      Do not fork, but run in the foreground.\n"
	      "  -c, --conf (file)     Specify an alternate configuration file location. The default\n"
	      "      --config (file)   is at \"/etc/qotd.conf\".\n"
	      "  -N, --noconf          Do not read from a configuration file, but use the default\n"
	      "      --noconfig        options instead.\n"
	      "      --lax             When parsing the configuration, don't check file permissions\n"
	      "                        or perform other security checks.\n",
	      stdout);
	fputs("  -P, --pidfile (file)  Override the pidfile name given in the configuration file with\n"
	      "                        the given file instead.\n"
	      "  -s, --quotes (file)   Override the quotes file given in the configuration file with\n"
	      "                        the given filename instead.\n"
	      "  -j, --journal (file)  Override the journal file given in the configuration file with\n"
	      "                        the given filename instead.\n"
	      "  -4, --ipv4            Only listen on IPv4.\n",
	      stdout);
	fputs("  -6, --ipv6            Only listen on IPv6. (By default the daemon listens on both)\n"
	      "  -t, --tcp             Use TCP. This is the default behavior.\n"
	      "  -u, --udp             Use UDP instead of TCP. (Not fully implemented yet)\n"
	      "  -q, --quiet           Only output error messages. This is the essentially the same as\n"
	      "                        using \"--journal /dev/null\".\n"
	      "  --help                List all options and what they do.\n"
	      "  --version             Print the version and some basic license information.\n",
	      stdout);
	cleanup(EXIT_SUCCESS, 0);
}

static void usage_and_exit(const char *program_name)
{
	print_usage(program_name);
	cleanup(EXIT_ARGUMENTS, 0);
}

static void version_and_exit()
{
	print_version();
	cleanup(EXIT_SUCCESS, 0);
}

static void parse_short_options(const char *argument,
				const char *next_arg,
				int *index,
				struct argument_flags *flags)
{
	size_t i;

	if (DEBUG) {
		journal("Parsing options in \"-%s\":\n", argument);
	}

	for (i = 0; argument[i]; i++) {
#if DEBUG
		journal("	Parsing flag \"-%c\".\n", argument[i]);
		journal("	flags = {\n");
		journal("		ProgramName: %s\n",	flags->program_name);
		journal("		ConfFile: %s\n",	flags->conf_file);
		journal("		QuotesFile: %s\n",	flags->quotes_file);
		journal("		PidFile: %s\n",		flags->pid_file);
		journal("		JournalFile: %s\n",	flags->journal_file);
		journal("		Daemonize: %s\n",	BOOLSTR2(flags->daemonize));
		journal("		Protocol: %s\n",	\
				name_option_protocol(flags->tproto, flags->iproto));
		journal("	}\n\n");
#endif

		switch (argument[i]) {
		case 'f':
			flags->daemonize = 0;
			break;
		case 'c':
			if (!next_arg) {
				fprintf(stderr, "You must specify a configuration file.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			(*index)++;
			flags->conf_file = next_arg;
			break;
		case 'N':
			flags->conf_file = NULL;
			break;
		case 'P':
			if (!next_arg) {
				fprintf(stderr, "You must specify a pid file.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			(*index)++;
			flags->pid_file = next_arg;
			break;
		case 's':
			if (!next_arg) {
				fprintf(stderr, "You must specify a quotes file.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			(*index)++;
			flags->quotes_file = next_arg;
			break;
		case 'j':
			if (!next_arg) {
				fprintf(stderr, "You must specify a journal file.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			(*index)++;
			flags->journal_file = next_arg;
			break;
		case '4':
			if (flags->iproto == PROTOCOL_IPv6) {
				fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			flags->iproto = PROTOCOL_IPv4;
			break;
		case '6':
			if (flags->iproto == PROTOCOL_IPv4) {
				fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			flags->iproto = PROTOCOL_IPv6;
			break;
		case 't':
			if (flags->tproto == PROTOCOL_UDP) {
				fprintf(stderr, "Conflicting options passed: -t and -u.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			flags->tproto = PROTOCOL_TCP;
			break;
		case 'u':
			if (flags->tproto == PROTOCOL_TCP) {
				fprintf(stderr, "Conflicting options passed: -t and -u.\n");
				cleanup(EXIT_ARGUMENTS, 1);
			}

			flags->tproto = PROTOCOL_UDP;
			break;
		case 'q':
			close_journal();
			break;
		default:
			fprintf(stderr, "Unknown short option: \"-%c\".\n", argument[i]);
			usage_and_exit(flags->program_name);
		}
	}
}

static void parse_long_option(const char *argument,
			      const char *next_arg,
			      int *i,
			      struct argument_flags *flags)
{
	if (!strcmp(argument, "help")) {
		help_and_exit(flags->program_name);
	} else if (!strcmp(argument, "version")) {
		version_and_exit();
	} else if (!strcmp(argument, "foreground")) {
		flags->daemonize = 0;
	} else if (!strcmp(argument, "conf") || !strcmp(argument, "config")) {
		if (!next_arg) {
			fprintf(stderr, "You must specify a configuration file.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->conf_file = next_arg;
		(*i)++;
	} else if (!strcmp(argument, "noconf") || !strcmp(argument, "noconfig")) {
		flags->conf_file = NULL;
	} else if (!strcmp(argument, "lax")) {
		fprintf(stderr, "Note: --lax has been enabled. Security checks will *not* be performed.\n");
		flags->strict = 0;
	} else if (!strcmp(argument, "pidfile")) {
		if (!next_arg) {
			fprintf(stderr, "You must specify a pid file.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->pid_file = next_arg;
		(*i)++;
	} else if (!strcmp(argument, "quotes")) {
		if (!next_arg) {
			fprintf(stderr, "You must specify a quotes file.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->quotes_file = argument;
	} else if (!strcmp(argument, "journal")) {
		if (!next_arg) {
			fprintf(stderr, "You must specify a journal file.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->journal_file = next_arg;
		(*i)++;
	} else if (!strcmp(argument, "ipv4")) {
		if (flags->iproto == PROTOCOL_IPv6) {
			fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->iproto = PROTOCOL_IPv4;
	} else if (!strcmp(argument, "ipv6")) {
		if (flags->iproto == PROTOCOL_IPv4) {
			fprintf(stderr, "Conflicting options passed: -4 and -6.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->iproto = PROTOCOL_IPv6;
	} else if (!strcmp(argument, "tcp")) {
		if (flags->tproto == PROTOCOL_UDP) {
			fprintf(stderr, "Conflicting options passed: -t and -u.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->tproto = PROTOCOL_TCP;
	} else if (!strcmp(argument, "udp")) {
		if (flags->tproto == PROTOCOL_TCP) {
			fprintf(stderr, "Conflicting options passed: -t and -u.\n");
			cleanup(EXIT_ARGUMENTS, 1);
		}

		flags->tproto = PROTOCOL_UDP;
	} else if (!strcmp(argument, "quiet")) {
		close_journal();
	} else {
		printf("Unrecognized long option: \"--%s\".\n", argument);
		usage_and_exit(flags->program_name);
	}
}

void parse_args(struct options *const opt,
		const int argc,
		const char *const argv[])
{
	struct argument_flags flags;
	int i;

	/* Set override flags */
	strncpy(flags._program_name, argv[0], sizeof(flags._program_name));
	flags.program_name = basename(flags._program_name);
	flags.conf_file = DEFAULT_CONFIG_FILE;
	flags.quotes_file = NULL;
	flags.pid_file = NULL;
	flags.journal_file = NULL;
	flags.tproto = PROTOCOL_TNONE;
	flags.iproto = PROTOCOL_INONE;
	flags.daemonize = BOOLEAN_UNSET;
	flags.strict = 1;

	/* Set default options, defined in options.h */
	opt->port = DEFAULT_PORT;
	opt->tproto = DEFAULT_TRANSPORT_PROTOCOL;
	opt->iproto = DEFAULT_INTERNET_PROTOCOL;
	opt->quotes_file = DEFAULT_QUOTES_FILE;
	opt->linediv = DEFAULT_LINE_DIVIDER;
	opt->pid_file = default_pidfile();
	opt->require_pidfile = DEFAULT_REQUIRE_PIDFILE;
	opt->daemonize = DEFAULT_DAEMONIZE;
	opt->drop_privileges = DEFAULT_DROP_PRIVILEGES;
	opt->is_daily = DEFAULT_IS_DAILY;
	opt->pad_quotes = DEFAULT_PAD_QUOTES;
	opt->allow_big = DEFAULT_ALLOW_BIG;
	opt->chdir_root = DEFAULT_CHDIR_ROOT;

	/* Parse arguments */
	for (i = 1; i < argc; i++) {
		const char *arg, *next_arg;

		next_arg = (i + 1 < argc) ? argv[i + 1] : NULL;
		arg = argv[i];
		if (arg[0] != '-' || !strcmp(arg, "-"))
			break;

		arg++;
		if (arg[0] == '-')
			parse_long_option(arg + 1, next_arg, &i, &flags);
		else
			parse_short_options(arg, next_arg, &i, &flags);
	}

	/* Override config file options */
	opt->strict = flags.strict;

	if (flags.conf_file) {
		if (flags.conf_file[0] != '/') {
			opt->chdir_root = 0;
		}
		parse_config(opt, flags.conf_file);
	}
	if (flags.pid_file) {
		opt->pid_file = strcmp(flags.pid_file, "none") ? flags.pid_file : NULL;
	}
	if (flags.quotes_file) {
		opt->quotes_file = flags.quotes_file;
	}
	if (flags.journal_file && !strcmp(flags.journal_file, "-")) {
		opt->journal_file = flags.journal_file;
	} else {
		opt->journal_file = NULL;
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
	journal("\nContents of struct 'opt':\n");
	journal("opt = {\n");
	journal("	QuotesFile: %s\n",		BOOLSTR(opt->quotes_file));
	journal("	PidFile: %s\n",			opt->pid_file);
	journal("	Port: %u\n",			opt->port);
	journal("	QuoteDivider: %s\n",		name_option_quote_divider(opt->linediv));
	journal("	Protocol: %s\n",		name_option_protocol(opt->tproto, opt->iproto));
	journal("	Daemonize: %s\n",		BOOLSTR(opt->daemonize));
	journal("	RequirePidfile: %s\n",	  	BOOLSTR(opt->require_pidfile));
	journal("	DropPrivileges: %s\n",	  	BOOLSTR(opt->drop_privileges));
	journal("	DailyQuotes: %s\n",	  	BOOLSTR(opt->is_daily));
	journal("	AllowBigQuotes: %s\n",	  	BOOLSTR(opt->allow_big));
	journal("	ChdirRoot: %s\n",		BOOLSTR(opt->chdir_root));
	journal("}\n\n");
#endif /* DEBUG */
}
