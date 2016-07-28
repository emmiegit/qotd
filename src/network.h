/*
 * network.h
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

#ifndef __NETWORK_H
# define __NETWORK_H

# include "options.h"

# ifdef __cplusplus
extern "C" {
# endif /* __cplusplus */

void set_up_ipv4_socket(const struct options *opt);
void set_up_ipv6_socket(const struct options *opt);
void close_socket();

void tcp_accept_connection(const struct options *opt);
void udp_accept_connection(const struct options *opt);

# ifdef __cplusplus
}
# endif /* __cplusplus */
#endif /* __NETWORK_H */

