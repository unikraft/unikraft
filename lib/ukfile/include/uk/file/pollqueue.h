/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Multi-event poll/wait queue with update chaining support */

#ifndef __UKFILE_POLLQUEUE_H__
#define __UKFILE_POLLQUEUE_H__

#include <uk/config.h>

#include <uk/assert.h>
#include <uk/atomic.h>
#include <uk/rwlock.h>
#include <uk/plat/time.h>
#include <uk/wait.h>

#if CONFIG_LIBUKFILE_CHAINUPDATE
#include <uk/list.h>
#include <uk/mutex.h>
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

/*
 * Bitmask of event flags.
 *
 * Should be large enough to accomodate what userspace will use as event flags
 * in the least significant bits, along with Unikraft-internal flags (if any)
 * in the more significant bits.
 */
typedef unsigned int uk_pollevent;

struct uk_file;

#if CONFIG_LIBUKFILE_POLLED
/**
 * Callback that fetches events in `mask` currently set on file `f`.
 *
 * This function cannot (meaningfully) fail, must not block indefinitely, and
 * should avoid taking locks or yielding execution when possible.
 * This function may be called arbitrarily concurrent.
 *
 * Drivers may choose to not provide this callback, in which case they are
 * responsible for updating the current event levels with `uk_pollq_set`,
 * `uk_pollq_clear`, and/or `uk_pollq_assign` in-band with I/O operations.
 *
 * If drivers do provide this, it will be called every time the instantaneous
 * level of events is queried. Drivers are then responsible only for notifying
 * the rising edges of events via `uk_pollq_set`.
 *
 * @param f File to fetch events for.
 * @param mask Bitmask of events to fetch.
 *
 * @return
 *   Bitwise AND between `mask` and the presently set events on `f`
 */
typedef uk_pollevent (*uk_poll_func)(const struct uk_file *f,
				     uk_pollevent mask);
#endif /* CONFIG_LIBUKFILE_POLLED */

#if CONFIG_LIBUKFILE_CHAINUPDATE

/* Update chaining */

enum uk_poll_chain_type {
	UK_POLL_CHAINTYPE_UPDATE,
	UK_POLL_CHAINTYPE_CALLBACK
};

enum uk_poll_chain_op {
	UK_POLL_CHAINOP_CLEAR,
	UK_POLL_CHAINOP_SET
};

struct uk_poll_chain;

/**
 * Update chaining callback function; called on event propagations.
 *
 * @param ev The events that triggered this update.
 * @param op Whether `events` are being set or cleared.
 * @param tick The update chaining ticket this callback is registered with.
 */
typedef void (*uk_poll_chain_callback_fn)(uk_pollevent ev,
					  enum uk_poll_chain_op op,
					  struct uk_poll_chain *tick);

/**
 * Ticket for registering on the update chaining list.
 *
 * If newly modified events overlap with those in `mask`, perform a chain update
 * of these overlapping bits according to `type`:
 *   - UK_POLL_CHAINTYPE_UPDATE: propagate events to `queue`.
 *   - UK_POLL_CHAINTYPE_CALLBACK: call `callback`
 */
struct uk_poll_chain {
	UK_STAILQ_ENTRY(struct uk_poll_chain) list_entry;
	uk_pollevent mask; /* Events to register for */
	enum uk_poll_chain_type type;
	union {
		struct uk_pollq *queue; /* Where to propagate updates */
		struct {
			uk_poll_chain_callback_fn callback;
			void *arg;
		};
	};
};

/* See comment for main queue below on initializers vs initial values */

/* Initializer for a chain ticket that propagates events to another queue */
#define UK_POLL_CHAIN_UPDATE_INITIALZER(msk, to) { \
	.mask = (msk), \
	.type = UK_POLL_CHAINTYPE_UPDATE, \
	.queue = (to), \
}

#define UK_POLL_CHAIN_UPDATE(msk, to) \
	((struct uk_poll_chain)UK_POLL_CHAIN_UPDATE_INITIALZER((msk), (to)))

/* Initializer for a chain ticket that calls a custom callback */
#define UK_POLL_CHAIN_CALLBACK_INITIALIZER(msk, cb, dat) { \
	.mask = (msk), \
	.type = UK_POLL_CHAINTYPE_CALLBACK, \
	.callback = (cb), \
	.arg = (dat) \
}

#define UK_POLL_CHAIN_CALLBACK(msk, cb, dat) ((struct uk_poll_chain) \
	UK_POLL_CHAIN_CALLBACK_INITIALIZER((msk), (cb), (dat)))

#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

/* Main queue */
struct uk_pollq {
#if CONFIG_LIBUKFILE_POLLED
	uk_poll_func poll_fn;
#endif /* CONFIG_LIBUKFILE_POLLED */
	uk_pollevent events;
	uk_pollevent waitmask;
	struct uk_waitq waitq; /* Polling threads */
	struct uk_rwlock waitlock;
#if CONFIG_LIBUKFILE_CHAINUPDATE
	void *_tag; /* Internal use */
	UK_STAILQ_HEAD(uk_pollq_chain_head, struct uk_poll_chain) prop;
	struct uk_mutex proplock;
	uk_pollevent propmask;
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
};

/*
 * Pollqueues come in two varieties: managed and polled.
 * Polled queues require drivers to only notify rising edges of events, while
 * providing a callback for fetching instantaneous levels (.poll_fn).
 * Managed queues require drivers to notify both rising and falling edges
 * of events, with the queue itself maintaining event levels.
 * See description of `uk_poll_func` for more details.
 *
 * Polled queues require setting LIBUKFILE_POLLED during configuration.
 */
/*
 * We define initializers separate from initial values.
 * The former can only be used in (static) variable initializations, while the
 * latter is meant for assigning to variables or as anonymous data structures.
 */
#if CONFIG_LIBUKFILE_CHAINUPDATE
#if CONFIG_LIBUKFILE_POLLED
#define __UK_POLLQ_INIT(q, pollfunc, ev) { \
	.poll_fn = (pollfunc),					\
	.events = (ev),						\
	.waitmask = 0,						\
	.waitq = UK_WAIT_QUEUE_INITIALIZER((q).waitq),		\
	.waitlock = UK_RWLOCK_INITIALIZER((q).waitlock, 0),	\
	.prop = UK_STAILQ_HEAD_INITIALIZER((q).prop),		\
	.proplock = UK_MUTEX_INITIALIZER((q).proplock),		\
	.propmask = 0,						\
}
#else /* !CONFIG_LIBUKFILE_POLLED */
#define __UK_POLLQ_INIT(q, pollfunc, ev) { \
	.events = (ev),						\
	.waitmask = 0,						\
	.waitq = UK_WAIT_QUEUE_INITIALIZER((q).waitq),		\
	.waitlock = UK_RWLOCK_INITIALIZER((q).waitlock, 0),	\
	.prop = UK_STAILQ_HEAD_INITIALIZER((q).prop),		\
	.proplock = UK_MUTEX_INITIALIZER((q).proplock),		\
	.propmask = 0,						\
}
#endif /* !CONFIG_LIBUKFILE_POLLED */
#else /* !CONFIG_LIBUKFILE_CHAINUPDATE */
#if CONFIG_LIBUKFILE_POLLED
#define __UK_POLLQ_INIT(q, pollfunc, ev) { \
	.poll_fn = (pollfunc),					\
	.events = (ev),						\
	.waitmask = 0,						\
	.waitq = UK_WAIT_QUEUE_INITIALIZER((q).waitq),		\
	.waitlock = UK_RWLOCK_INITIALIZER((q).waitlock, 0),	\
}
#else /* !CONFIG_LIBUKFILE_POLLED */
#define __UK_POLLQ_INIT(q, pollfunc, ev) { \
	.events = (ev),						\
	.waitmask = 0,						\
	.waitq = UK_WAIT_QUEUE_INITIALIZER((q).waitq),		\
	.waitlock = UK_RWLOCK_INITIALIZER((q).waitlock, 0),	\
}
#endif /* !CONFIG_LIBUKFILE_POLLED */
#endif /* !CONFIG_LIBUKFILE_CHAINUPDATE */

#if CONFIG_LIBUKFILE_POLLED
#define UK_POLLQ_POLLED_INITIALIZER(q, pollfunc) __UK_POLLQ_INIT(q, pollfunc, 0)

#define UK_POLLQ_POLLED_INIT_VALUE(q, pollfunc) \
	((struct uk_pollq)UK_POLLQ_POLLED_INITIALIZER(q, pollfunc))
#endif /* CONFIG_LIBUKFILE_POLLED */

#define UK_POLLQ_MANAGED_EVENTS_INITIALIZER(q, ev) __UK_POLLQ_INIT(q, NULL, ev)
#define UK_POLLQ_MANAGED_INITIALIZER(q) \
	UK_POLLQ_MANAGED_EVENTS_INITIALIZER(q, 0)

#define UK_POLLQ_MANAGED_EVENTS_INIT_VALUE(q, ev) \
	((struct uk_pollq)UK_POLLQ_MANAGED_EVENTS_INITIALIZER(q, ev))
#define UK_POLLQ_MANAGED_INIT_VALUE(q) \
	((struct uk_pollq)UK_POLLQ_MANAGED_INITIALIZER(q))

/* Polling */

#if CONFIG_LIBUKFILE_POLLED

#define UK_POLLQ_IS_POLLED(q) (!!((q)->poll_fn))

/**
 * INTERNAL. Poll for the events in `req`; never block or take locks,
 * always return immediately.
 */
static inline
uk_pollevent _uk_pollq_poll_immediate(struct uk_pollq *q, uk_pollevent req)
{
	return UK_POLLQ_IS_POLLED(q) ? 0 : (q->events & req);
}

/**
 * INTERNAL. Poll for the events in `req` with the waitlock held; may block.
 */
static inline
uk_pollevent _uk_pollq_poll_locked(struct uk_pollq *q, uk_pollevent req,
				   const struct uk_file *f)
{
	return UK_POLLQ_IS_POLLED(q) ? q->poll_fn(f, req) : (q->events & req);
}
#else /* !CONFIG_LIBUKFILE_POLLED */
/**
 * INTERNAL. Poll for the events in `req`; never block or take locks,
 * always return immediately.
 */
static inline
uk_pollevent _uk_pollq_poll_immediate(struct uk_pollq *q, uk_pollevent req)
{
	return q->events & req;
}

/**
 * INTERNAL. Poll for the events in `req` with the waitlock held; may block.
 */
static inline
uk_pollevent _uk_pollq_poll_locked(struct uk_pollq *q, uk_pollevent req,
				   const struct uk_file *f __unused)
{
	return _uk_pollq_poll_immediate(q, req);
}
#endif

/**
 * INTERNAL. Atomically poll & lock if required.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 * @param f File whose events to poll.
 *
 * @return
 *   non-zero evmask with lock released if events appeared
 *   0 with lock held otherwise.
 */
static inline
uk_pollevent _uk_pollq_lock(struct uk_pollq *q, uk_pollevent req,
			    const struct uk_file *f)
{
	uk_pollevent ev;

	uk_rwlock_rlock(&q->waitlock);
	/* Check if events were set while acquiring the lock */
	if ((ev = _uk_pollq_poll_locked(q, req, f)))
		uk_rwlock_runlock(&q->waitlock);
	return ev;
}

/**
 * INTERNAL. Wait for events or until a timeout.
 *
 * Must be called only after `_pollq_lock` returns 0 (read waitlock held).
 * Returns with read waitlock held.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 * @param deadline Max number of nanoseconds to wait or, or 0 if forever
 *
 * @return
 *   0 on timeout
 *   non-zero if awoken
 */
static inline
int _uk_pollq_wait(struct uk_pollq *q, uk_pollevent req, __nsec deadline)
{
	/* Mark request in waitmask */
	(void)uk_or(&q->waitmask, req);
	/* Set events as cookie & wait */
	uk_waitq_set_cookie(req);
	return !uk_waitq_wait_deadline_locked(&q->waitq, deadline,
					      uk_rwlock_rlock,
					      uk_rwlock_runlock,
					      &q->waitlock);
}

/**
 * Poll for the events in `req`, returning the present levels of events.
 *
 * May yield execution or acquire locks, but will never wait on events.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 * @param f File to poll for events, in case of an edge-triggered `q`.
 *
 * @return
     Bitwise AND between `req` and the events set in `q`
 */
static inline
uk_pollevent uk_pollq_poll_level(struct uk_pollq *q, uk_pollevent req,
				 const struct uk_file *f __maybe_unused)
{
	uk_pollevent ev;

	if ((ev = _uk_pollq_poll_immediate(q, req)))
		return ev;
#if CONFIG_LIBUKFILE_POLLED
	if (UK_POLLQ_IS_POLLED(q)) {
		ev = _uk_pollq_lock(q, req, f);
		if (!ev)
			uk_rwlock_runlock(&q->waitlock);
	}
#endif /* CONFIG_LIBUKFILE_POLLED */
	return ev;
}

/**
 * Poll for the events in `req`, blocking until `deadline` or an event is set.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 * @param deadline Max number of nanoseconds to wait for, or 0 if forever
 *
 * @return
 *   Bitwise AND between `req` and the events set in `q`, or 0 if timed out
 */
static inline
uk_pollevent uk_pollq_poll_until(struct uk_pollq *q, uk_pollevent req,
				 __nsec deadline, const struct uk_file *f)
{
	uk_pollevent ev;

	if ((ev = _uk_pollq_poll_immediate(q, req)))
		return ev;
	if ((ev = _uk_pollq_lock(q, req, f)))
		return ev;
	while (_uk_pollq_wait(q, req, deadline)) {
		if ((ev = _uk_pollq_poll_locked(q, req, f)))
			break;
	}
	uk_rwlock_runlock(&q->waitlock);
	return ev;
}

/**
 * Poll for the events in `req`, blocking until an event is set.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 *
 * @return
 *   Bitwise AND between `req` and the events set in `q`
 */
#define uk_pollq_poll(q, req, f) uk_pollq_poll_until(q, req, 0, f)

#if CONFIG_LIBUKFILE_CHAINUPDATE
/* Propagation */

/**
 * INTERNAL. Register update chaining ticket.
 *
 * Must be called with prop lock held.
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to register.
 */
static inline
void _uk_pollq_register(struct uk_pollq *q, struct uk_poll_chain *tick)
{
	q->propmask |= tick->mask;
	UK_STAILQ_INSERT_TAIL(&q->prop, tick, list_entry);
}

/**
 * Register ticket `tick` for event propagations on `q`.
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to register.
 */
static inline
void uk_pollq_register(struct uk_pollq *q, struct uk_poll_chain *tick)
{
	uk_mutex_lock(&q->proplock);
	_uk_pollq_register(q, tick);
	uk_mutex_unlock(&q->proplock);
}

/**
 * Unregister ticket `tick` from event propagations on `q`.
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to unregister.
 */
static inline
void uk_pollq_unregister(struct uk_pollq *q, struct uk_poll_chain *tick)
{
	uk_mutex_lock(&q->proplock);
	UK_STAILQ_REMOVE(&q->prop, tick, struct uk_poll_chain, list_entry);
	uk_mutex_unlock(&q->proplock);
}

/**
 * Poll for events and/or register for propagation on `q`.
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to register, if needed.
 * @param always_register If 0, will immediately return without registering if
 *   any of the requested events are set. If non-zero, always register.
 *
 * @return
 *   Requested events that are currently active.
 */
static inline
uk_pollevent uk_pollq_poll_register(struct uk_pollq *q,
				    struct uk_poll_chain *tick,
				    int always_register,
				    const struct uk_file *f)
{
	uk_pollevent ev;
	uk_pollevent req = tick->mask;

	if (!always_register && (ev = _uk_pollq_poll_immediate(q, req)))
		return ev;
	/* Might need to register */
	uk_mutex_lock(&q->proplock);
	if ((ev = _uk_pollq_poll_locked(q, req, f)) && !always_register)
		goto out;
	_uk_pollq_register(q, tick);
out:
	uk_mutex_unlock(&q->proplock);
	return ev;
}
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

/* Updating */

/**
 * Update events, setting those in `set` and handling notifications.
 *
 * @param q Target queue.
 * @param set Events to set.
 * @param one If zero, notify all waiting threads, if non-zero notify at most 1.
 *
 * @return
 *   The previous event set.
 */
uk_pollevent uk_pollq_set_n(struct uk_pollq *q, uk_pollevent set, int one);

/**
 * Update events, clearing those in `clr`.
 *
 * Only meaningful on managed queues, no-op on polled queues.
 *
 * @param q Target queue.
 * @param clr Events to clear.
 *
 * @return
 *   The previous event set.
 */
uk_pollevent uk_pollq_clear(struct uk_pollq *q, uk_pollevent clr);

/**
 * Replace the events in `q` with `val` and handle notifications.
 *
 * Only meaningful on managed queues, identical to set on polled queues.
 *
 * @param q Target queue.
 * @param val New event set.
 * @param one If zero, notify all waiting threads, if non-zero notify at most 1.
 *
 * @return
 *   The previous event set.
 */
uk_pollevent uk_pollq_assign_n(struct uk_pollq *q, uk_pollevent val, int one);

#define uk_pollq_set(q, s) uk_pollq_set_n(q, s, 0)

#define uk_pollq_assign(q, s) uk_pollq_assign_n(q, s, 0)

#endif /* __UKFILE_POLLQUEUE_H__ */
