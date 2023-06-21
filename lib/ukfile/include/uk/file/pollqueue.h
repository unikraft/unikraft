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
#include <uk/thread.h>

/*
 * Bitmask of event flags.
 *
 * Should be large enough to accomodate what userspace will use as event flags
 * in the least significant bits, along with Unikraft-internal flags (if any)
 * in the more significant bits.
 */
typedef unsigned int uk_pollevent;

/**
 * Ticket for registering on the poll waiting list.
 *
 * If the newly set events overlap with those in `mask`, wake up `thread`.
 * Tickets are atomically released from the wait queue when waking.
 */
struct uk_poll_ticket {
	struct uk_poll_ticket *next;
	struct uk_thread *thread; /* Thread to wake up */
	uk_pollevent mask; /* Events to register for */
};

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
 *     If `set` != 0 set/clear events in `set`, instead of original
 *   - UK_POLL_CHAINTYPE_CALLBACK: call `callback`
 */
struct uk_poll_chain {
	struct uk_poll_chain *next;
	uk_pollevent mask; /* Events to register for */
	enum uk_poll_chain_type type;
	union {
		struct {
			struct uk_pollq *queue; /* Where to propagate updates */
			uk_pollevent set; /* Events to set */
		};
		struct {
			uk_poll_chain_callback_fn callback;
			void *arg;
		};
	};
};

/* Initializer for a chain ticket that propagates events to another queue */
#define UK_POLL_CHAIN_UPDATE(msk, to, ev) ((struct uk_poll_chain){ \
	.next = NULL, \
	.mask = (msk), \
	.type = UK_POLL_CHAINTYPE_UPDATE, \
	.queue = (to), \
	.set = (ev) \
})

/* Initializer for a chain ticket that calls a custom callback */
#define UK_POLL_CHAIN_CALLBACK(msk, cb, dat) ((struct uk_poll_chain){ \
	.next = NULL, \
	.mask = (msk), \
	.type = UK_POLL_CHAINTYPE_CALLBACK, \
	.callback = (cb), \
	.arg = (dat) \
})

#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

/* Main queue */
struct uk_pollq {
	/* Notification lists */
	struct uk_poll_ticket *wait; /* Polling threads */
	struct uk_poll_ticket **waitend;
#if CONFIG_LIBUKFILE_CHAINUPDATE
	struct uk_poll_chain *prop; /* Registrations for chained updates */
	struct uk_poll_chain **propend;
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

	/* Events */
	volatile uk_pollevent events; /* Instantaneous event levels */
	uk_pollevent waitmask; /* Events waited on by threads */
#if CONFIG_LIBUKFILE_CHAINUPDATE
	uk_pollevent propmask; /* Events registered for chaining */
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	/* Locks & sundry */
#if CONFIG_LIBUKFILE_CHAINUPDATE
	void *_tag; /* Internal use */
	struct uk_rwlock proplock; /* Chained updates list lock */
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	struct uk_rwlock waitlock; /* Wait list lock */
};

#if CONFIG_LIBUKFILE_CHAINUPDATE
#define UK_POLLQ_INITIALIZER(q) \
	((struct uk_pollq){ \
		.wait = NULL, \
		.waitend = &(q).wait, \
		.prop = NULL, \
		.propend = &(q).prop, \
		.events = 0, \
		.waitmask = 0, \
		.propmask = 0, \
		.proplock = UK_RWLOCK_INITIALIZER((q).proplock, 0), \
		.waitlock = UK_RWLOCK_INITIALIZER((q).waitlock, 0), \
	})
#else /* !CONFIG_LIBUKFILE_CHAINUPDATE */
#define UK_POLLQ_INITIALIZER(q) \
	((struct uk_pollq){ \
		.wait = NULL, \
		.waitend = &(q).wait, \
		.events = 0, \
		.waitmask = 0, \
		.waitlock = UK_RWLOCK_INITIALIZER((q).waitlock, 0), \
	})
#endif /* !CONFIG_LIBUKFILE_CHAINUPDATE */

/**
 * Initialize the fields of `q` to a valid empty state.
 */
static inline
void uk_pollq_init(struct uk_pollq *q)
{
	q->wait = NULL;
	q->waitend = &q->wait;
	q->events = 0;
	q->waitmask = 0;
	uk_rwlock_init(&q->waitlock);
#if CONFIG_LIBUKFILE_CHAINUPDATE
	q->prop = NULL;
	q->propend = &q->prop;
	q->propmask = 0;
	uk_rwlock_init(&q->proplock);
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
}

/* Polling cancellation */

/**
 * Remove a specific `ticket` from the wait list.
 */
static inline
void uk_pollq_cancel_ticket(struct uk_pollq *q, struct uk_poll_ticket *ticket)
{
	uk_rwlock_wlock(&q->waitlock);
	for (struct uk_poll_ticket **p = &q->wait; *p; p = &(*p)->next)
		if (*p == ticket) {
			*p = ticket->next;
			ticket->next = NULL;
			if (!*p)
				q->waitend = p;
			break;
		}
	uk_rwlock_wunlock(&q->waitlock);
}

/**
 * Remove the ticket of a specific `thread` from the wait list.
 */
static inline
void uk_pollq_cancel_thread(struct uk_pollq *q, struct uk_thread *thread)
{
	uk_rwlock_wlock(&q->waitlock);
	for (struct uk_poll_ticket **p = &q->wait; *p; p = &(*p)->next) {
		struct uk_poll_ticket *t = *p;

		if (t->thread == thread) {
			*p = t->next;
			t->next = NULL;
			if (!*p)
				q->waitend = p;
			break;
		}
	}
	uk_rwlock_wunlock(&q->waitlock);
}

/**
 * Remove the ticket of the current thread from the wait list.
 */
#define uk_pollq_cancel(q) uk_pollq_cancel_thread((q), uk_thread_current())

/* Polling */

/**
 * Poll for the events in `req`; never block, always return immediately.
 *
 * @return
 *   Bitwise AND between `req` and the events set in `q`.
 */
static inline
uk_pollevent uk_pollq_poll_immediate(struct uk_pollq *q, uk_pollevent req)
{
	return q->events & req;
}

/**
 * INTERNAL. Atomically poll & lock if required.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 * @param exp Events expected to be already set.
 *
 * @return
 *   non-zero evmask with lock released if events appeared
 *   0 with lock held otherwise.
 */
static inline
uk_pollevent _pollq_lock(struct uk_pollq *q, uk_pollevent req,
			 uk_pollevent exp)
{
	uk_pollevent ev;

	uk_rwlock_rlock(&q->waitlock);
	/* Check if events were set while acquiring the lock */
	if ((ev = uk_pollq_poll_immediate(q, req) & ~exp))
		uk_rwlock_runlock(&q->waitlock);
	return ev;
}

/**
 * INTERNAL. Wait for events until a timeout.
 *
 * Must be called only after `_pollq_lock` returns 0.
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
int _pollq_wait(struct uk_pollq *q, uk_pollevent req, __nsec deadline)
{
	struct uk_poll_ticket **tail;
	struct uk_thread *__current;
	struct uk_poll_ticket tick;
	int timeout;

	/* Mark request in waitmask */
	(void)uk_or(&q->waitmask, req);
	/* Compete to register */

	__current = uk_thread_current();
	tick = (struct uk_poll_ticket){
		.next = NULL,
		.thread = __current,
		.mask = req,
	};
	tail = uk_exchange_n(&q->waitend, &tick.next);
	/* tail is ours alone, safe to link in */
	UK_ASSERT(!*tail); /* Should be a genuine list tail */
	*tail = &tick;

	/* Block until awoken */
	uk_thread_block_until(__current, deadline);
	uk_rwlock_runlock(&q->waitlock);
	uk_sched_yield();
	/* Back, wake up, check if timed out & try again */
	timeout = deadline && ukplat_monotonic_clock() >= deadline;
	if (timeout)
		uk_pollq_cancel_ticket(q, &tick);
	return !timeout;
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
				 __nsec deadline)
{
	uk_pollevent ev;

	do {
		if ((ev = uk_pollq_poll_immediate(q, req)))
			return ev;
		if ((ev = _pollq_lock(q, req, 0)))
			return ev;
	} while (_pollq_wait(q, req, deadline));
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
#define uk_pollq_poll(q, req) uk_pollq_poll_until(q, req, 0)

/**
 * Poll for event rising edges in `req`, blocking until `deadline` or an edge.
 *
 * In contrast to normal poll, will not return immediately if events are set,
 * nor return which events were detected.
 * Use `uk_pollq_poll_immediate` to check the current set events, however events
 * may have been modified in the meantime, potentially leading to lost edges.
 * To correctly handle these missed edges, use update chaining.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 * @param deadline Max number of nanoseconds to wait for, or 0 if forever
 *
 * @return
 *   1 if a rising edge was detected,
 *   0 if timed out
 */
static inline
int uk_pollq_edge_poll_until(struct uk_pollq *q, uk_pollevent req,
			     __nsec deadline)
{
	uk_pollevent level = uk_pollq_poll_immediate(q, req);

	/* Acquire lock & check for new events */
	if (_pollq_lock(q, req, level))
		return 1;
	/* Wait for notification */
	return _pollq_wait(q, req, deadline);
}

/**
 * Poll for event rising edges in `req`, blocking until a rising edge.
 *
 * In contrast to normal poll, will not return immediately if events are set,
 * nor return which events were detected.
 * Use `uk_pollq_poll_immediate` to check the current set events.
 * To correctly handle missed edges, use update chaining.
 *
 * @param q Target queue.
 * @param req Events to poll for.
 *
 * @return
 *   1 if a rising edge was detected,
 *   0 if timed out
 */
#define uk_pollq_edge_poll(q, req) uk_pollq_edge_poll_until(q, req, 0)


#if CONFIG_LIBUKFILE_CHAINUPDATE
/* Propagation */

/**
 * INTERNAL. Register update chaining ticket.
 *
 * Must be called with appropriate locks held
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to register.
 */
static inline
void _pollq_register(struct uk_pollq *q, struct uk_poll_chain *tick)
{
	struct uk_poll_chain **tail;

	(void)uk_or(&q->propmask, tick->mask);
	tail = uk_exchange_n(&q->propend, &tick->next);
	UK_ASSERT(!*tail); /* Should be genuine list tail */
	*tail = tick;
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
	uk_rwlock_rlock(&q->proplock);
	_pollq_register(q, tick);
	uk_rwlock_runlock(&q->proplock);
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
	uk_rwlock_wlock(&q->proplock);
	for (struct uk_poll_chain **p = &q->prop; *p; p = &(*p)->next)
		if (*p == tick) {
			*p = tick->next;
			tick->next = NULL;
			if (!*p) /* We unlinked last node */
				q->propend = p;
			break;
		}
	uk_rwlock_wunlock(&q->proplock);
}

/**
 * Update the registration ticket `tick` with values from `ntick` atomically.
 *
 * `ntick` should first be initialized from `tick`, then have values updated.
 * Supplying a `tick` that is not registered with `q` or `ntick` with a `next`
 * field different from the one in `tick` is undefined behavior.
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to update.
 * @param ntick New values for fields in `tick`.
 */
static inline
void uk_pollq_reregister(struct uk_pollq *q, struct uk_poll_chain *tick,
			 const struct uk_poll_chain *ntick)
{
	UK_ASSERT(tick->next == ntick->next);
	uk_rwlock_rlock(&q->proplock);
	uk_or(&q->propmask, ntick->mask);
	*tick = *ntick;
	uk_rwlock_runlock(&q->proplock);
}

/**
 * Poll for events and/or register for propagation on `q`.
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to register, if needed.
 * @param force If 0, will immediately return without registering if any of the
 *   requested events are set. If non-zero, always register.
 *
 * @return
 *   Requested events that are currently active.
 */
static inline
uk_pollevent uk_pollq_poll_register(struct uk_pollq *q,
				    struct uk_poll_chain *tick, int force)
{
	uk_pollevent ev;
	uk_pollevent req = tick->mask;

	if (!force && (ev = uk_pollq_poll_immediate(q, req)))
		return ev;
	/* Might need to register */
	uk_rwlock_rlock(&q->proplock);
	if ((ev = uk_pollq_poll_immediate(q, req)) && !force)
		goto out;
	_pollq_register(q, tick);
out:
	uk_rwlock_runlock(&q->proplock);
	return ev;
}

/**
 * Poll for event rising edges and/or register for propagation on `q`.
 *
 * @param q Target queue.
 * @param tick Update chaining ticket to register, if needed.
 * @param force If 0, will immediately return without registering if any of the
 *   requested event rising edges are detected. If non-zero, always register.
 *
 * @return
 *   Detected rising edges of requested events.
 */
static inline
uk_pollevent uk_pollq_edge_poll_register(struct uk_pollq *q,
					 struct uk_poll_chain *tick,
					 int force)
{
	uk_pollevent ev;
	uk_pollevent req = tick->mask;
	uk_pollevent level = uk_pollq_poll_immediate(q, req);

	uk_rwlock_rlock(&q->proplock);
	if ((ev = uk_pollq_poll_immediate(q, req) & ~level) && !force)
		goto out;
	_pollq_register(q, tick);
out:
	uk_rwlock_runlock(&q->proplock);
	return ev;
}
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

/* Updating */

/**
 * Update events, clearing those in `clr`.
 *
 * @param q Target queue.
 * @param clr Events to clear.
 *
 * @return
 *   The previous event set.
 */
uk_pollevent uk_pollq_clear(struct uk_pollq *q, uk_pollevent clr);

/**
 * Update events, setting those in `set` and handling notifications.
 *
 * @param q Target queue.
 * @param set Events to set.
 * @param n Maximum number of threads to wake up. If < 0 wake up all threads.
 *   Chained updates have their own defined notification semantics and may
 *   notify more threads than specified in `n`.
 *
 * @return
 *   The previous event set.
 */
uk_pollevent uk_pollq_set_n(struct uk_pollq *q, uk_pollevent set, int n);

/**
 * Replace the events in `q` with `val` and handle notifications.
 *
 * @param q Target queue.
 * @param val New event set.
 * @param n Maximum number of threads to wake up. If < 0 wake up all threads.
 *   Chained updates have their own defined notification semantics and may
 *   notify more threads than specified in `n`
 *
 * @return
 *   The previous event set.
 */
uk_pollevent uk_pollq_assign_n(struct uk_pollq *q, uk_pollevent val, int n);

#define UK_POLLQ_NOTIFY_ALL -1

/**
 * Update events, setting those in `set` and handling notifications.
 *
 * @param q Target queue.
 * @param set Events to set.
 *
 * @return
 *   The previous event set.
 */
#define uk_pollq_set(q, s) uk_pollq_set_n(q, s, UK_POLLQ_NOTIFY_ALL)

/**
 * Replace the events in `q` with `val` and handle notifications.
 *
 * @param q Target queue.
 * @param val New event set.
 *
 * @return
 *   The previous event set.
 */
#define uk_pollq_assign(q, s) uk_pollq_assign_n(q, s, UK_POLLQ_NOTIFY_ALL)

#endif /* __UKFILE_POLLQUEUE_H__ */
