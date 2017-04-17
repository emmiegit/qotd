/*
 * quotes.h
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

#ifndef _QUOTES_H_
#define _QUOTES_H_

#include <stddef.h>

#include "config.h"

int open_quotes_file(const struct options *opt);
void close_quotes_file(void);

void destroy_quote_buffers(void);
int get_quote_of_the_day(const char **buffer, size_t *length);

#endif /* _QUOTES_H_ */

