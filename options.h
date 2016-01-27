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
typedef struct {
    unsigned int port;
    char* quotesfile;
    char* pidfile;
    bool quotesmalloc;
    bool pidmalloc;
    bool daemonize;
    bool require_pidfile;
    bool is_daily;
    bool allow_big;
} options;
#define __OPTIONS_H
#endif /* __OPTIONS_H */

