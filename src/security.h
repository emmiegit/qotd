/*
 * security.h
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

#ifndef __SECURITY_H
# define __SECURITY_H

# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

# include "options.h"

# define ROOT_USER_ID				0
# define ROOT_GROUP_ID				0
# define DAEMON_GROUP_NAME			"daemon"

/* Any port lower than this requires root permissions */
# define MIN_NORMAL_PORT			1024

void drop_privileges(void);
void security_options_check(const struct options *opt);
void security_file_check(const char *path, const char *file_type);

# define security_conf_file_check(path)		security_file_check((path), "configuration")
# define security_quotes_file_check(path)	security_file_check((path), "quotes")

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __SECURITY_H */

