/*
 * daemon.h
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

#ifndef _DAEMON_H_
#define _DAEMON_H_

/* For EXIT_SUCCESS and EXIT_FAILURE */
#include <stdlib.h>

enum {
	EXIT_ARGUMENTS = 17,
	EXIT_CONFIGURATION,
	EXIT_SECURITY,
	EXIT_MEMORY,
	EXIT_IO,
	EXIT_SIGNAL,
	EXIT_INTERNAL
};

void cleanup(int ret, int quiet);

#endif /* _DAEMON_H_ */

