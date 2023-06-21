/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/file/pollqueue.h>

#include <uk/assert.h>

static void pollq_notify_n(struct uk_pollq *q, uk_pollevent set, int n)
{
	uk_rwlock_wlock(&q->waitlock);
	if (q->waitmask & set) {
		/* Walk wait list, wake up & collect */
		uk_pollevent seen = 0;

		for (struct uk_poll_ticket **p = &q->wait; *p; p = &(*p)->next) {
			struct uk_poll_ticket *t = *p;

			if (!n)
				goto done;
			if (t->mask & set) {
				*p = t->next;
				t->next = NULL;
				uk_thread_wake(t->thread);
				n--;
			} else {
				seen |= t->mask;
			}
			if (!*p) {
				/* We just unlinked last node */
				q->waitend = p;
				break;
			}
		}
		/* Reached end of list, can prune waitmask */
		q->waitmask = seen;
	}
done:
	uk_rwlock_wunlock(&q->waitlock);
}

#if CONFIG_LIBUKFILE_CHAINUPDATE
static void pollq_propagate(struct uk_pollq *q,
			    enum uk_poll_chain_op op, uk_pollevent set)
{
	uk_rwlock_wlock(&q->proplock);
	if (q->propmask & set) {
		uk_pollevent seen;

		/* Tag this queue in case of chaining loops */
		UK_ASSERT(!q->_tag);
		q->_tag = uk_thread_current();
		/* Walk chain list & propagate updates */
		seen = 0;
		for (struct uk_poll_chain **p = &q->prop; *p; p = &(*p)->next) {
			struct uk_poll_chain *t = *p;
			uk_pollevent req = set & t->mask;

			if (req) {
				switch (t->type) {
				case UK_POLL_CHAINTYPE_UPDATE:
				{
					uk_pollevent ev = t->set ? t->set : req;

					switch (op) {
					case UK_POLL_CHAINOP_CLEAR:
						uk_pollq_clear(t->queue, ev);
						break;
					case UK_POLL_CHAINOP_SET:
						uk_pollq_set(t->queue, ev);
						break;
					}
				}
					break;
				case UK_POLL_CHAINTYPE_CALLBACK:
					t->callback(req, op, t);
					break;
				}
			}
			seen |= t->mask;
		}
		q->propmask = seen; /* Prune propmask */
		q->_tag = NULL; /* Clear tag */
	}
	uk_rwlock_wunlock(&q->proplock);
}
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

uk_pollevent uk_pollq_clear(struct uk_pollq *q, uk_pollevent clr)
{
	uk_pollevent prev = uk_and(&q->events, ~clr);

#if CONFIG_LIBUKFILE_CHAINUPDATE
	pollq_propagate(q, UK_POLL_CHAINOP_CLEAR, clr);
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	return prev;
}

uk_pollevent uk_pollq_set_n(struct uk_pollq *q, uk_pollevent set, int n)
{
	uk_pollevent prev;

	if (!set)
		return 0;

#if CONFIG_LIBUKFILE_CHAINUPDATE
	if (q->_tag == uk_thread_current()) /* Chaining update loop, return */
		return 0;
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */

	prev = uk_or(&q->events, set);
	pollq_notify_n(q, set, n);
#if CONFIG_LIBUKFILE_CHAINUPDATE
	pollq_propagate(q, UK_POLL_CHAINOP_SET, set);
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	return prev;
}

uk_pollevent uk_pollq_assign_n(struct uk_pollq *q, uk_pollevent val, int n)
{
	uk_pollevent prev = uk_exchange_n(&q->events, val);
	uk_pollevent set = val & ~prev;

	if (set) {
		pollq_notify_n(q, set, n);
#if CONFIG_LIBUKFILE_CHAINUPDATE
		pollq_propagate(q, UK_POLL_CHAINOP_SET, set);
#endif /* CONFIG_LIBUKFILE_CHAINUPDATE */
	}
	return prev;
}
