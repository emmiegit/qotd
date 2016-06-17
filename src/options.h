/*
 * options.h
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

#ifndef __OPTIONS_H
# define __OPTIONS_H

# include <stdbool.h>

# define DIV_EVERYLINE 0
# define DIV_PERCENT   1
# define DIV_WHOLEFILE 2

# define PROTOCOL_IPV4 1
# define PROTOCOL_IPV6 2
# define PROTOCOL_BOTH 3

# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

struct options {
    char *quotesfile;        /* string containing path to quotes file */
    char *pidfile;           /* string containing path to pid file */
    unsigned int port;       /* port to listen on */
    unsigned char linediv;   /* how to read the quotes file */
    unsigned char protocol;  /* which ip protocol(s) to use */

    bool quotesmalloc;       /* if quotesfile needs to be free'd */
    bool pidmalloc;          /* if pidfile needs to be free'd */

    bool daemonize;          /* whether to fork to the background or not */
    bool require_pidfile;    /* whether to quit if the pidfile cannot be made */
    bool is_daily;           /* whether quotes are random every day or every visit */
    bool allow_big;          /* ignore 512-byte limit */
    bool chdir_root;         /* whether to chdir to / */
};

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __OPTIONS_H */

