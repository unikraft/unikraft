/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Types used for wait queues in Unikraft */

#ifndef __UK_SCHED_WAIT_TYPES_H__
#define __UK_SCHED_WAIT_TYPES_H__

#include <uk/list.h>
#include <uk/plat/spinlock.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_waitq;

struct uk_waitq_ticket {
	/* Entry to link into wait queue */
	struct uk_list_head link;
	/* Queue we're waiting on, if waiting; must be NULL if not waiting */
	struct uk_waitq *wq;
};

#define UK_WAITQ_TICKET_INITIALIZER { .wq = __NULL }

#define UK_WAITQ_TICKET_INIT_VALUE \
	((struct uk_waitq_ticket)UK_WAITQ_TICKET_INITIALIZER)

struct uk_waitq {
	struct uk_list_head waiters;
	__spinlock lock;
};

#define UK_WAIT_QUEUE_INITIALIZER(name)				\
{								\
	.waiters = UK_LIST_HEAD_INIT((name).waiters),		\
	.lock = UKARCH_SPINLOCK_INITIALIZER()			\
}

#define DEFINE_WAIT_QUEUE(name) \
	struct uk_waitq name = UK_WAIT_QUEUE_INITIALIZER(name)

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_WAIT_TYPES_H__ */
