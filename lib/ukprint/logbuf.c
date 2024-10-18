/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <string.h>

#include <uk/assert.h>
#include <uk/console.h>
#include <uk/spinlock.h>

#include "logbuf.h"
#include "print.h"
#include "snprintf.h"

#define LOGMSG_ALIGN		16
#define LOGMSG_SIZE(_msg_len)	(sizeof(struct uk_logbuf_msg) + (_msg_len))

UK_LOGBUF(uk_logbuf_default);

/* Returns the next free aligned position in the buffer, or wraps
 * around.
 */
static __uptr logbuf_next(struct uk_logbuf *lb)
{
	__uptr next;

	UK_ASSERT(lb);

	if (!lb->head)
		return (__uptr)lb->buf;

	next = ALIGN_UP((__uptr)lb->tail + LOGMSG_SIZE(lb->tail->msg.len),
			LOGMSG_ALIGN);

	if (next >= (__uptr)lb->buf + sizeof(lb->buf))
		next = (__uptr)lb->buf;

	return next;
}

static void update_head(struct uk_logbuf *lb,
			struct uk_logbuf_msg *logmsg, __sz msg_len)
{
	struct uk_logbuf_msg *lm;

	UK_ASSERT(lb);
	UK_ASSERT(logmsg);

	/* If the new message overlaps with the current head
	 * shift head to the first non-overlapping message.
	 */
	if (RANGE_OVERLAP((__uptr)lb->head, LOGMSG_SIZE(lb->head->msg.len),
			  (__uptr)logmsg, LOGMSG_SIZE(msg_len))) {
		uk_logbuf_foreach(lb, lm) {
			/* Use RANGE_OVERLAP instead of numeric comparison as
			 * we're operating on a circular buffer and hence the
			 * next element may be at a lower address.
			 */
			if (!RANGE_OVERLAP(lm, LOGMSG_SIZE(lm->msg.len),
					   logmsg, LOGMSG_SIZE(msg_len))) {
				lb->head = lm;
				return;
			}
		}
		/* This logmsg will be the only element in the buffer */
		lb->head = (struct uk_logbuf_msg *)logmsg;
	}
}

int uk_logbuf_write(struct uk_logbuf *lb, struct uk_print_msg *msg)
{
	struct uk_logbuf_msg *logmsg;
	va_list cp;

	UK_ASSERT(lb);
	UK_ASSERT(msg);

	uk_spin_lock(&lb->lock);

	va_copy(cp, msg->ap);

	/* pass zero size to obtain formatted string's length */
	msg->len = uk_vsnprintf(msg->msg, 0, msg->fmt, cp);
	msg->len++; /* NUL byte */

	va_end(cp);

	if (LOGMSG_SIZE(msg->len) > sizeof(lb->buf)) {
		uk_spin_unlock(&lb->lock);
		return -EMSGSIZE;
	}

	/* Get next position in the logbuf */
	logmsg = (struct uk_logbuf_msg *)logbuf_next(lb);

	/* Now take the message length into account.
	 * If we exceed the buffer, wrap around.
	 */
	if ((__uptr)logmsg + LOGMSG_SIZE(msg->len) > (__uptr)lb->buf + sizeof(lb->buf))
		logmsg = (struct uk_logbuf_msg *)lb->buf;

	/* Update logbuf_head before we overwrite any message objects.
	 * The first message needs to be checked / updated upon every
	 * new write after the first wraparound.
	 */
	if (lb->head)
		update_head(lb, logmsg, msg->len);

	/* Update previous tail before we do the write.
	 * As we may be overwriting that member, performing
	 * the update now ensures that we don't corrupt our
	 * own memory.
	 */
	if (lb->tail)
		lb->tail->next = logmsg;

	memcpy(&logmsg->msg, msg, sizeof(*msg));

	/* Format the message now. Once the caller returns,
	 * fmt and va_list will no longer be available.
	 */
	va_copy(cp, msg->ap);
	uk_vsnprintf(logmsg->msg.msg, msg->len, msg->fmt, cp);
	va_end(cp);

	if (!lb->head) {
		lb->head = logmsg;
		lb->tail = logmsg;
	}
	/* We are now the new last message. Set
	 * next to self and update tail.
	 */
	logmsg->next = logmsg;
	lb->tail = logmsg;

	uk_spin_unlock(&lb->lock);

	return 0;
}

void uk_print_dmesg(void)
{
#if CONFIG_LIBUKCONSOLE
	struct uk_logbuf_msg *lm;
	struct uk_logbuf *lb;

	lb = &uk_logbuf_default;

	uk_spin_lock(&lb->lock);

	if (!lb->head) {
		uk_spin_unlock(&lb->lock);
		return;
	}

	uk_logbuf_foreach(lb, lm)
		uk_console_out(lm->msg.msg, lm->msg.len);

	uk_spin_unlock(&lb->lock);
#endif /* CONFIG_LIBUKCONSOLE */
}
