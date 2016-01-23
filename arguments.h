/*
 * arguments.h
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

#include <stdbool.h>

#define DELIM_NEWLINE    0
#define DELIM_EMPTYLINE  1

#ifndef __OPTIONS_STRUCT
typedef struct {
    unsigned int port;
    char* quotesfile;
    char delimeter;
    bool is_daily;
    bool allow_big;
} options;
#define __OPTIONS_STRUCT
#endif /* __OPTIONS_STRUCT */

options* parse_args(const int argc, const char* argv[]);

