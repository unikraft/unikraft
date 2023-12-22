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

#endif /* CONFIG_LIBPOSIX_SOCKET_EVENTS */
#endif /* __UK_POSIX_SOCKET_EVENT_H__ */
