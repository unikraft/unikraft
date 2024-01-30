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

enum uk_socket_state {
	UK_SOCKET_STATE_CLOSE = 0,    /**< Socket closed */
	UK_SOCKET_STATE_CREATE,       /**< New socket created */
	UK_SOCKET_STATE_BIND,         /**< Socket bind to an address */
	UK_SOCKET_STATE_LISTEN,       /**< Listener for incom. connections */
	UK_SOCKET_STATE_ACCEPT,       /**< Incoming connection accepted */
	UK_SOCKET_STATE_CONNECT,      /**< Connection to remote established */

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
	enum uk_socket_state state;
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
 * @param event
 *   The event to register for: CLOSE, CREATE, BIND, LISTEN, ACCEPT, or CONNECT
 * @param recfn
 *   Event receiver function, must have the following singature:
 *   `void recvfn(const struct uk_socket_event *)`
 */
#define __UK_SOCKET_EVENT_RECEIVER(afamily, event, arecvfn)		\
	static int __uk_event ## event ## arecvfn(void *data)		\
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
	UK_EVENT_HANDLER(UK_POSIX_SOCKET_EVENT_ ## event,		\
			 __uk_event ## event ## arecvfn)

#define _UK_SOCKET_EVENT_RECEIVER(family, event, recvfn) \
	__UK_SOCKET_EVENT_RECEIVER(family, event, recvfn)

#define UK_SOCKET_EVENT_RECEIVER(family, event, recvfn) \
	_UK_SOCKET_EVENT_RECEIVER(family, event, recvfn)

#endif /* CONFIG_LIBPOSIX_SOCKET_EVENTS */
#endif /* __UK_POSIX_SOCKET_EVENT_H__ */
