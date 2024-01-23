/*
 * journal.h
 *
 * qotd - A simple QOTD daemon.
 * Copyright (c) 2015-2024 Emmie Smith
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

#ifndef _JOURNAL_H_
#define _JOURNAL_H_

void open_journal(const char *path);
int close_journal(void);
int journal_is_open(void);

int journal(const char *message, ...);

#define JTRACE()			\
	do {				\
		journal("%s:%d: ",	\
			__FILE__,	\
			__LINE__);	\
	} while (0)

#endif /* _JOURNAL_H_ */
