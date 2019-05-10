/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Ported from Mini-OS */

#ifndef __UK_SCHED_WAIT_TYPES_H__
#define __UK_SCHED_WAIT_TYPES_H__

#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_waitq_entry {
	int waiting;
	struct uk_thread *thread;
	UK_STAILQ_ENTRY(struct uk_waitq_entry) thread_list;
};

/* TODO - lock required? */
UK_STAILQ_HEAD(uk_waitq, struct uk_waitq_entry);

#define __WAIT_QUEUE_INITIALIZER(name) UK_STAILQ_HEAD_INITIALIZER(name)

#define DEFINE_WAIT_QUEUE(name) \
	struct uk_waitq name = __WAIT_QUEUE_INITIALIZER(name)

#define DEFINE_WAIT(name) \
struct uk_waitq_entry name = { \
	.waiting      = 0, \
	.thread       = uk_thread_current(), \
	.thread_list  = { NULL } \
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_WAIT_TYPES_H__ */
