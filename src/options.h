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

#include <stdbool.h>

#ifndef __OPTIONS_H
# define __OPTIONS_H

# define DIV_EVERYLINE 0
# define DIV_PERCENT   1
# define DIV_WHOLEFILE 2

typedef struct {
    char* quotesfile;        /* string containing path to quotes file */
    char* pidfile;           /* string containing path to pid file */
    unsigned int port;       /* port to listen on */
    unsigned char linediv;   /* how to read the quotes file */

    bool quotesmalloc;       /* if quotesfile needs to be free'd */
    bool pidmalloc;          /* if pidfile needs to be free'd */

    bool daemonize;          /* whether to fork to the background or not */
    bool require_pidfile;    /* whether to quit if the pidfile cannot be made */
    bool is_daily;           /* whether quotes are random every day or every visit */
    bool allow_big;          /* ignored 512-byte limit */
} options;

#endif /* __OPTIONS_H */

