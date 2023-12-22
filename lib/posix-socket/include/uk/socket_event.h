/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_POSIX_SOCKET_EVENT_H__
#define __UK_POSIX_SOCKET_EVENT_H__

#include <uk/config.h>
#include <stdint.h>
#include <sys/socket.h>
#include <uk/event.h>

/*
 * POSIX Socket Events are emitted events that can be used to track
 * connection states and endpoints of sockets
 */

enum uk_socket_event_type {
	UK_SOCKET_EVENT_INVALID = 0,  /**< INVALID: Should never be emitted */
	UK_SOCKET_EVENT_LISTEN,       /**< New listener for incom. connect. */
	UK_SOCKET_EVENT_ACCEPT,       /**< Incoming connection accepted */
	UK_SOCKET_EVENT_CONNECT,      /**< Connection to remote established */
	UK_SOCKET_EVENT_CLOSE         /**< Connection or listener closed */

};

struct uk_socket_event;

#if CONFIG_LIBPOSIX_SOCKET_EVENTS
/*
 * Socket event data structure
 */
struct uk_socket_event {
	uintptr_t id;                 /**< Unique socket identifier
				       *   NOTE: Can be re-used for a new socket
				       *         after CLOSE
				       */
	enum uk_socket_event_type event;
	int family;
	int type;
	int protocol;
	const struct sockaddr *laddr; /**< Local address
				       *   Can be NULL if a socket is not `bind`
				       */
	socklen_t laddr_len;
	const struct sockaddr *raddr; /**< Remote address
				       *   Can be NULL for listening sockets,
				       *   should have a value on ACCEPT and
				       *   CONNECT
				       */
	socklen_t raddr_len;
};

/**
 * Registers a socket event receiver function
 *
 * @param family
 *   The socket family (e.g., AF_INET) to receive events for. Can be set
 *   to AF_UNSPEC to receive any socket event of any address family.
 * @param recfn
 *   Event receiver function, must have the following singature:
 *   `void recvfn(const struct uk_socket_event *)`
 */
#define __UK_SOCKET_EVENT_RECEIVER(afamily, arecvfn)			\
	static int __uk_event ## arecvfn(void *data)			\
	{								\
		const struct uk_socket_event *e =			\
			(const struct uk_socket_event *) data;		\
									\
		if (afamily && (afamily != e->family))			\
			return UK_EVENT_HANDLED_CONT;			\
									\
		arecvfn(e);						\
		return UK_EVENT_HANDLED_CONT;				\
	}								\
									\
	UK_EVENT_HANDLER(UK_POSIX_SOCKET_EVENT, __uk_event ## arecvfn)

#define _UK_SOCKET_EVENT_RECEIVER(family, recvfn) \
	__UK_SOCKET_EVENT_RECEIVER(family, recvfn)

#define UK_SOCKET_EVENT_RECEIVER(family, recvfn) \
	_UK_SOCKET_EVENT_RECEIVER(family, recvfn)

#endif /* CONFIG_LIBPOSIX_SOCKET_EVENTS */
#endif /* __UK_POSIX_SOCKET_EVENT_H__ */
