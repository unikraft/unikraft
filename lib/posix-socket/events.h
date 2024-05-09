/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_POSIX_SOCKET_EVENT_HELPERS_H__
#define __UK_POSIX_SOCKET_EVENT_HELPERS_H__

#include <uk/config.h>

#if CONFIG_LIBPOSIX_SOCKET_EVENTS
#include <uk/socket_event.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/essentials.h>

enum uk_socket_state;

struct uk_socket_event_data {
	uintptr_t id;
	int family;
	int type;
	int protocol;
	enum uk_socket_state state;
	/* NOTE: We use `struct sockaddr_storage` instead of `struct sockaddr *`
	 *       to have statically allocated memory of enough space to store
	 *       any socket address without calling malloc.
	 *       1) This enables us to provide the event handler functions
	 *       without requiring any error handling at calling code (e.g.,
	 *       ENOMEM). 2) We can keep the integration with low overhead.
	 */
	struct sockaddr_storage laddr;
	socklen_t laddr_len;
	struct sockaddr_storage raddr;
	socklen_t raddr_len;
	unsigned int raise_cnt;
};

static inline void uk_socket_evd_init(struct uk_socket_event_data *evd,
				      int family, int type, int protocol)
{
	UK_ASSERT(evd);

	evd->id         = (uintptr_t)evd; /* use ptr as unique id */
	evd->raise_cnt  = 0;
	evd->family     = family;
	evd->type       = type;
	evd->protocol   = protocol;
	evd->state      = UK_SOCKET_STATE_CLOSE;
	evd->laddr_len  = 0;
	evd->raddr_len  = 0;
}

/**
 * Initializes socket event data by copying family, type, and protocol from
 * another socket event data space. Please note that no address is copied.
 * Depending on the case, this should be done with
 * `uk_socket_evd_laddr_set_from()` or `uk_socket_evd_raddr_set_from()`.
 *
 * @param evd
 *  Reference to even data area to initialize
 * @param from_evd
 *  Reference to even data area to copy from
 */
static inline void uk_socket_evd_init_from(struct uk_socket_event_data *evd,
					   const struct uk_socket_event_data
					   *from_evd)
{
	UK_ASSERT(evd);
	UK_ASSERT(from_evd);

	evd->id         = (uintptr_t)evd; /* use ptr as unique id */
	evd->raise_cnt  = 0;
	evd->family     = from_evd->family;
	evd->type       = from_evd->type;
	evd->protocol   = from_evd->protocol;
	evd->state      = UK_SOCKET_STATE_CLOSE;
	evd->laddr_len  = 0;
	evd->raddr_len  = 0;
}

static inline void uk_socket_evd_laddr_set(struct uk_socket_event_data *evd,
					   const struct sockaddr *laddr,
					   socklen_t laddr_len)
{
	UK_ASSERT(evd);

	if (!laddr || (laddr_len == 0)) {
		/* Special case: we unset the address */
		evd->laddr_len = 0;
		return;
	}

	memcpy(&evd->laddr, laddr, laddr_len);
	evd->laddr_len = laddr_len;
}

static inline void uk_socket_evd_laddr_set_from(struct uk_socket_event_data
						*evd,
						const
						struct uk_socket_event_data
						*from_evd)
{
	uk_socket_evd_laddr_set(evd, (const struct sockaddr *)&from_evd->laddr,
				from_evd->laddr_len);
}

static inline void uk_socket_evd_raddr_set(struct uk_socket_event_data *evd,
					   const struct sockaddr *raddr,
					   socklen_t raddr_len)
{
	UK_ASSERT(evd);

	if (!raddr || (raddr_len == 0)) {
		/* Special case: we unset the address */
		evd->raddr_len = 0;
		return;
	}

	memcpy(&evd->raddr, raddr, raddr_len);
	evd->raddr_len = raddr_len;
}

static inline void uk_socket_evd_raddr_set_from(struct uk_socket_event_data
						*evd,
						const
						struct uk_socket_event_data
						*from_evd)
{
	uk_socket_evd_raddr_set(evd, (const struct sockaddr *)&from_evd->raddr,
				from_evd->raddr_len);
}

static inline void socket_event_raise(struct uk_socket_event_data *evd,
				      enum uk_socket_state new_state,
				      struct uk_event *evt)
{
	struct uk_socket_event emsg = {
		.id        = evd->id,
		.state     = evd->state,
		.family    = evd->family,
		.type      = evd->type,
		.protocol  = evd->protocol,
		.laddr_len = evd->laddr_len,
		.laddr     = (evd->laddr_len == 0) ?
				NULL : (const struct sockaddr *)&evd->laddr,
		.raddr_len = evd->raddr_len,
		.raddr     = (evd->raddr_len == 0) ?
				NULL : (const struct sockaddr *)&evd->raddr,
	};

	evd->raise_cnt++;
	uk_raise_event_ptr(evt, &emsg);

	/* Update the state to reflect the change */
	evd->state = new_state;
}

static inline unsigned int uk_socket_event_raise_count(struct
						       uk_socket_event_data
						       *evd)
{
	UK_ASSERT(evd);

	return evd->raise_cnt;
}

UK_EVENT(UK_POSIX_SOCKET_EVENT_CLOSE);
UK_EVENT(UK_POSIX_SOCKET_EVENT_CREATE);
UK_EVENT(UK_POSIX_SOCKET_EVENT_BIND);
UK_EVENT(UK_POSIX_SOCKET_EVENT_LISTEN);
UK_EVENT(UK_POSIX_SOCKET_EVENT_ACCEPT);
UK_EVENT(UK_POSIX_SOCKET_EVENT_CONNECT);

#define __uk_socket_event_raise(evd, event)				\
	socket_event_raise(evd, UK_SOCKET_STATE_ ## event,		\
			   UK_EVENT_PTR(UK_POSIX_SOCKET_EVENT_ ## event))

#define _uk_socket_event_raise(evd, event) \
	__uk_socket_event_raise(evd, event)

#define uk_socket_event_raise(evd, event) \
	_uk_socket_event_raise(evd, event)

#else /* !CONFIG_LIBPOSIX_SOCKET_EVENTS */

/*
 * Stubs
 */

struct uk_socket_event_data;

#define uk_socket_evd_init(evd, family, type, protocol) \
	do {} while (0)
#define uk_socket_evd_init_from(evd, from_evd) \
	do {} while (0)
#define uk_socket_evd_laddr_set(evd, laddr, laddr_len) \
	do {} while (0)
#define uk_socket_evd_laddr_set_from(evd, from_evd) \
	do {} while (0)
#define uk_socket_evd_raddr_set(evd, raddr, raddr_len) \
	do {} while (0)
#define uk_socket_evd_raddr_set_from(evd, from_evd) \
	do {} while (0)
#define uk_socket_event_raise(evd, event) \
	do {} while (0)
#define uk_socket_event_raise_count(evd) \
	({ 0; })

#endif /* !CONFIG_LIBPOSIX_SOCKET_EVENTS */

#define uk_socket_event_has_raised(evd)		\
	(uk_socket_event_raise_count(evd) != 0)

#endif /* __UK_POSIX_SOCKET_EVENT_HELPERS_H__ */
