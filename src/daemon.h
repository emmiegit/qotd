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

#ifndef __DAEMON_H
# define __DAEMON_H

# include <stdbool.h>

# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

# define EXIT_ARGUMENTS			17
# define EXIT_CONFIGURATION		18
# define EXIT_SECURITY			19
# define EXIT_MEMORY			20
# define EXIT_IO			21
# define EXIT_SIGNAL			22
# define EXIT_INTERNAL			23

void cleanup(int ret, bool quiet);

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __DAEMON_H */

