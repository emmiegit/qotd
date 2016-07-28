/*
 * journal.h
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

#ifndef __JOURNAL_H
# define __JOURNAL_H

# include <stdbool.h>

# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

void open_journal(const char *path);
int close_journal();
bool journal_is_open();

int journal(const char *message, ...);

# define ERR_TRACE()		journal("%s:%d: ", __FILE__, __LINE__)

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __JOURNAL_H */

