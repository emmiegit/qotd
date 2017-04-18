/*
 * network.c
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

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <signal.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "core.h"
#include "daemon.h"
#include "journal.h"
#include "network.h"
#include "quotes.h"

#define IPPROTO_PART_STRING(opt)	(((opt)->iproto == PROTOCOL_BOTH) ? "4/" : "")
#define TCP_CONNECTION_BACKLOG		50

static int sockfd = -1;

/*
 * If the returned error is one of these, then you
 * should not attempt to remake the socket.
 */
static void check_socket_error(const int error)
{
	switch (error) {
	case EBADF:
	case EFAULT:
	case EINVAL:
	case EMFILE:
	case ENFILE:
	case ENOBUFS:
	case ENOMEM:
	case EPERM:
	case ENOSR:
	case EPIPE:
	case ENOTSOCK:
	case ENOTCONN:
	case EPROTO:
	case ECONNRESET:

#if defined(EOPNOTSUPP)
	case EOPNOTSUPP:
#endif /* EOPNOTSUPP */

#if defined(ESOCKTNOSUPPORT)
	case ESOCKTNOSUPPORT:
#endif /* ESOCKTNOSUPPORT */

#if defined(EPROTONOSUPPORT)
	case EPROTONOSUPPORT:
#endif /* EPROTONOSUPPORT */
		cleanup(EXIT_IO, 1);

	case EINTR:
		raise(SIGSTOP);
	}
}

void set_up_ipv4_socket(const struct options *const opt)
{
	struct sockaddr_in serv_addr;
	const int one = 1;

	if (opt->tproto == PROTOCOL_TCP) {
		journal("Setting up IPv4 socket over TCP...\n");
		sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	} else {
		journal("Setting up IPv4 socket over UDP...\n");
		sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}

	if (unlikely(sockfd < 0)) {
		const int errsave = errno;
		assert(errno != 0);
		JTRACE();
		journal("Unable to create IPv4 socket: %s.\n",
			strerror(errsave));
		cleanup(EXIT_IO, 1);
	}
	if (unlikely(setsockopt(sockfd,
				SOL_SOCKET,
				SO_REUSEADDR,
				(const void *)(&one),
				sizeof(one)) < 0)) {
		const int errsave = errno;
		assert(errno != 0);
		JTRACE();
		journal("Unable to set the socket to allow address reuse: %s.\n",
			strerror(errsave));
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(opt->port);

	if (unlikely(bind(sockfd,
			 (const struct sockaddr *)(&serv_addr),
			 sizeof(struct sockaddr_in)) < 0)) {
		const int errsave = errno;
		assert(errno != 0);
		JTRACE();
		journal("Unable to bind to IPv4 socket: %s.\n",
			strerror(errsave));
		cleanup(EXIT_IO, 1);
	}
}

void set_up_ipv6_socket(const struct options *const opt)
{
	struct sockaddr_in6 serv_addr;
	const int one = 1;

	if (opt->tproto == PROTOCOL_TCP) {
		journal("Setting up IPv%s6 socket over TCP...\n",
			IPPROTO_PART_STRING(opt));
		sockfd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
	} else {
		journal("Setting up IPv%s6 socket over UDP...\n",
			IPPROTO_PART_STRING(opt));
		sockfd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	}

	if (sockfd < 0) {
		const int errsave = errno;
		assert(errno != 0);
		journal("Unable to create IPv6 socket: %s.\n", strerror(errsave));
		cleanup(EXIT_IO, 1);
	}

	if (opt->iproto == PROTOCOL_IPv6) {
		if (unlikely(setsockopt(sockfd,
					IPPROTO_IPV6,
					IPV6_V6ONLY,
					(const void *)(&one),
					sizeof(one)) < 0)) {
			const int errsave = errno;
			assert(errno != 0);
			JTRACE();
			journal("Unable to set IPv4 compatibility option: %s.\n", strerror(errsave));
			cleanup(EXIT_IO, 1);
		}
	}
	if (unlikely(setsockopt(sockfd,
				SOL_SOCKET,
				SO_REUSEADDR,
				(const void *)(&one),
				sizeof(one)) < 0)) {
		const int errsave = errno;
		assert(errno != 0);
		JTRACE();
		journal("Unable to set the socket to allow address reuse: %s.\n", strerror(errsave));
	}

	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_addr = in6addr_any;
	serv_addr.sin6_port = htons(opt->port);
	serv_addr.sin6_flowinfo = htonl(0);
	serv_addr.sin6_scope_id = 0;

	if (unlikely(bind(sockfd,
			  (const struct sockaddr *)(&serv_addr),
			  sizeof(struct sockaddr_in6)) < 0)) {
		const int errsave = errno;
		assert(errno != 0);
		journal("Unable to bind to socket: %s.\n",
			strerror(errsave));
		cleanup(EXIT_IO, 1);
	}
}

void close_socket(void)
{
	if (sockfd < 0)
		return;
	if (unlikely(close(sockfd))) {
		const int errsave = errno;
		assert(errno != 0);
		journal("Unable to close socket file descriptor %d: %s.\n",
			sockfd, strerror(errsave));
	}
}

static void tcp_write(const char *buf,
		      size_t *len,
		      int consockfd)
{
	while (*len > 0) {
		ssize_t bytes;

		bytes = write(consockfd, buf, *len);
		if (unlikely(bytes < 0)) {
			const int errsave = errno;
			JTRACE();
			journal("Unable to write to TCP socket: %s.\n",
				strerror(errsave));
			check_socket_error(errsave);
			return;
		}
		buf += bytes;
		*len -= (size_t)bytes;
	}
}

static void udp_write(const char *buf,
		      size_t *len,
		      const struct sockaddr *cli_addr,
		      socklen_t cli_len)
{
	while (*len > 0) {
		ssize_t bytes;

		bytes = sendto(sockfd, buf, *len, 0, cli_addr, cli_len);
		if (unlikely(bytes < 0)) {
			const int errsave = errno;
			JTRACE();
			journal("Unable to write to UDP socket: %s.\n",
				strerror(errsave));
			check_socket_error(errsave);
			return;
		}
		buf += bytes;
		*len -= (size_t)bytes;
	}
}

void tcp_accept_connection(void)
{
	struct sockaddr_in cli_addr;
	socklen_t cli_len;
	int consockfd;
	const char *buffer;
	size_t length;

	journal("Listening for connection...\n");
	if (unlikely(listen(sockfd, TCP_CONNECTION_BACKLOG))) {
		const int errsave = errno;
		assert(errno != 0);
		JTRACE();
		journal("Unable to listen on socket: %s.\n", strerror(errsave));
		cleanup(EXIT_IO, 1);
	}

	cli_len = sizeof(cli_addr);
	consockfd = accept(sockfd, (struct sockaddr *)(&cli_addr), &cli_len);
	if (consockfd < 0) {
		const int errsave = errno;
		assert(errno != 0);
		JTRACE();
		journal("Unable to accept connection: %s.\n", strerror(errsave));
		check_socket_error(errsave);
		return;
	}

	if (get_quote_of_the_day(&buffer, &length))
		goto end;

	tcp_write(buffer,
		  &length,
		  consockfd);

end:
	close(consockfd);
}

void udp_accept_connection(void)
{
	struct sockaddr_in cli_addr;
	socklen_t cli_len;
	const char *buffer;
	size_t length;

	journal("Listening for connection...\n");
	cli_len = sizeof(cli_addr);
	if (unlikely(recvfrom(sockfd,
			      NULL, 0, 0,
			      (struct sockaddr *)(&cli_addr),
			      &cli_len) < 0)) {
		const int errsave = errno;
		assert(errno != 0);
		JTRACE();
		journal("Unable to read from socket: %s.\n", strerror(errsave));
		check_socket_error(errsave);
		return;
	}

	if (get_quote_of_the_day(&buffer, &length))
		return;

	udp_write(buffer,
		 &length,
		 (struct sockaddr *)(&cli_addr),
		 cli_len);
}
