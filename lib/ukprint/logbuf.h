/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LOGBUF_H__
#define __UK_LOGBUF_H__

#include <uk/config.h>
#include <uk/spinlock.h>

#include "print.h"

extern struct uk_logbuf uk_logbuf_default;

struct uk_logbuf_msg {
	struct uk_logbuf_msg *next;
	struct uk_print_msg msg;
};

struct uk_logbuf {
	__u8 buf[CONFIG_LIBUKPRINT_LOGBUF_SIZE];
	struct uk_logbuf_msg *head;
	struct uk_logbuf_msg *tail;
	uk_spinlock lock;
};

#define UK_LOGBUF(lb)				    \
	struct uk_logbuf lb = UK_LOGBUF_INIT()

#define UK_LOGBUF_INIT()				    \
	{						    \
		.head = __NULL,				    \
		.tail = __NULL,				    \
		.lock = UK_SPINLOCK_INITIALIZER(),	    \
	}

/* struct uk_logbuf_msg *lm */
#define uk_logbuf_foreach(lb, lm)			\
	for ((lm) = (lb)->head; (lm) != __NULL;		\
	     (lm) = ((lm) == (lm)->next) ? __NULL : (lm)->next)

int uk_logbuf_write(struct uk_logbuf *lb, struct uk_print_msg *msg);
__isr int uk_logbuf_write_isr(struct uk_logbuf *lb, struct uk_print_msg *msg);

#endif /* __UK_LOGBUF_H__ */
