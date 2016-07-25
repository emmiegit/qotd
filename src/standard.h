/*
 * standard.h
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

#ifndef __STANDARD_H
# define __STANDARD_H

# define UNUSED(x)	((void)(x))
# define EMPTYSTR(x)	(((x)[0]) == '\0')
# define PLURAL(x)	(((x) == 1) ? "" : "s")
# define MIN(x, y)	(((x) < (y)) ? (x) : (y))
# define MAX(x, y)	(((x) > (y)) ? (x) : (y))

# ifndef DEBUG
#  define DEBUG		0
# endif /* DEBUG */

# if DEBUG
#  pragma message("Compiling with debug statements.")
# endif /* DEBUG */

# if defined(__GNUC__) || defined(__clang__)
#  define likely(x)	__builtin_expect(!!(x), 1)
#  define unlikely(x)	__builtin_expect(!!(x), 0)
# else
#  define likely(x)	(x)
#  define unlikely(x)	(x)
# endif /* __GNUC__ || __clang__ */
#endif /* __STANDARD_H */

