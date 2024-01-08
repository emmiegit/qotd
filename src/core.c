/*
 * core.h
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

#include <stdio.h>

#include "core.h"

void print_version(void)
{
	printf("%s v%d.%d.%d [%s]\n"
	       "(%s %s) on %s.\n"
	       "Built %s, %s.\n"
	       "\n"
	       "%s is free software: you can redistribute it and/or modify\n"
	       "it under the terms of the GNU General Public License as published by\n"
	       "the Free Software Foundation, version 2 or later.\n",
	       PROGRAM_NAME,
	       PROGRAM_VERSION_MAJOR,
	       PROGRAM_VERSION_MINOR,
	       PROGRAM_VERSION_PATCH,
	       GIT_HASH,
	       COMPILER_NAME,
	       COMPILER_VERSION,
	       PLATFORM_NAME,
	       __DATE__,
	       __TIME__,
	       PROGRAM_NAME);
}
