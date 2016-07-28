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

#ifndef __QUOTES_H
# define __QUOTES_H

# include <sys/socket.h>

# include "options.h"

# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

int open_quotes_file(const char *path);
int close_quotes_file();
void destroy_quote_buffers();
int tcp_send_quote(int fd, const struct options *opt);
int udp_send_quote(int fd, const struct sockaddr *cli_addr, socklen_t cli_len, const struct options *opt);

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __QUOTES_H */

