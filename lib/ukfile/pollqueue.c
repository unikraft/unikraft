/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/file/pollqueue.h>

static void pollq_notify_n(struct uk_pollq *q, uk_pollevent set, int one)
{
	uk_rwlock_wlock(&q->waitlock);
	if (q->waitmask & set) {
		struct uk_waitq_ticket *t;
		uk_pollevent seen = 0;
		int broke_early = 0;

		if (one)
			uk_waitq_wake_up_one_if(&q->waitq, t,
				(seen |= t->cookie,
				 broke_early = !!(t->cookie & set)));
		else
			uk_waitq_wake_up_if(&q->waitq, t,
				(seen |= t->cookie, !!(t->cookie & set)));
		/* Prune waitmask if we've seen all tickets */
		if (!broke_early)
			q->waitmask = seen;
	}
	uk_rwlock_wunlock(&q->waitlock);
}

#if CONFIG_LIBUKFILE_CHAINUPDATE
static void pollq_propagate(struct uk_pollq *q,
			    enum uk_poll_chain_op op, uk_pollevent set)
{
	uk_mutex_lock(&q->proplock);
	if (q->propmask & set) {
		struct uk_poll_chain *t;
		uk_pollevent seen = 0;

		/* Tag this queue in case of chaining loops */
		UK_ASSERT(!q->_tag);
		q->_tag = uk_thread_current();
		/* Walk chain list & propagate updates */
		UK_STAILQ_FOREACH(t, &q->prop, list_entry) {
			uk_pollevent req = set & t->mask;

			seen |= t->mask;
			if (req) {
				switch (t->type) {
				case UK_POLL_CHAINTYPE_UPDATE:
					switch (op) {
					case UK_POLL_CHAINOP_CLEAR:
						uk_pollq_clear(t->queue, req);
						break;
					case UK_POLL_CHAINOP_SET:
						uk_pollq_set(t->queue, req);
						break;
					}
					break;
				case UK_POLL_CHAINTYPE_CALLBACK:
					t->callback(req, op, t);
					break;
				}
			}
		}
		q->propmask = seen; /* Prune propmask */
		q->_tag = NULL; /* Clear tag */
	}
	uk_mutex_unlock(&q->proplock);
}
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

uk_pollevent uk_pollq_clear(struct uk_pollq *q, uk_pollevent clr)
{
	uk_pollevent prev;

#if CONFIG_LIBUKFILE_POLLED
	if (UK_POLLQ_IS_POLLED(q))
		return 0;
#endif /* CONFIG_LIBUKFILE_POLLED */

#if CONFIG_LIBUKFILE_CHAINUPDATE
	if (q->_tag == uk_thread_current()) /* Chaining update loop, return */
		return 0;
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

	prev = uk_and(&q->events, ~clr);
#if CONFIG_LIBUKFILE_CHAINUPDATE
	pollq_propagate(q, UK_POLL_CHAINOP_CLEAR, clr);
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	return prev;
}

uk_pollevent uk_pollq_set_n(struct uk_pollq *q, uk_pollevent set, int one)
{
	uk_pollevent prev;

	if (!set)
		return 0;

#if CONFIG_LIBUKFILE_CHAINUPDATE
	if (q->_tag == uk_thread_current()) /* Chaining update loop, return */
		return 0;
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

#if CONFIG_LIBUKFILE_POLLED
	if (UK_POLLQ_IS_POLLED(q))
		prev = 0;
	else
#endif /* CONFIG_LIBUKFILE_POLLED */
		prev = uk_or(&q->events, set);

	pollq_notify_n(q, set, one);
#if CONFIG_LIBUKFILE_CHAINUPDATE
	pollq_propagate(q, UK_POLL_CHAINOP_SET, set);
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	return prev;
}

uk_pollevent uk_pollq_assign_n(struct uk_pollq *q, uk_pollevent val, int one)
{
	uk_pollevent prev;
	uk_pollevent set;
#if CONFIG_LIBUKFILE_CHAINUPDATE
	uk_pollevent clr;
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

#if CONFIG_LIBUKFILE_POLLED
	if (UK_POLLQ_IS_POLLED(q))
		return uk_pollq_set_n(q, val, one);
#endif /* CONFIG_LIBUKFILE_POLLED */

#if CONFIG_LIBUKFILE_CHAINUPDATE
	if (q->_tag == uk_thread_current()) /* Chaining update loop, return */
		return 0;
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

	prev = uk_exchange_n(&q->events, val);
	set = val & ~prev;
	if (set)
		pollq_notify_n(q, set, one);
#if CONFIG_LIBUKFILE_CHAINUPDATE
	clr = prev & ~val;
	if (clr)
		pollq_propagate(q, UK_POLL_CHAINOP_CLEAR, clr);
	if (set)
		pollq_propagate(q, UK_POLL_CHAINOP_SET, set);
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	return prev;
}
