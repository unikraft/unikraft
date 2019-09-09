/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UK_MUTEX_H__
#define __UK_MUTEX_H__

#include <uk/config.h>

#if CONFIG_LIBUKLOCK_MUTEX
#include <uk/assert.h>
#include <uk/plat/lcpu.h>
#include <uk/thread.h>
#include <uk/wait.h>
#include <uk/wait_types.h>
#include <uk/plat/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mutex that relies on a scheduler
 * uses wait queues for threads
 */
struct uk_mutex {
	int lock_count;
	struct uk_thread *owner;
	struct uk_waitq wait;
};

#define	UK_MUTEX_INITIALIZER(name)				\
	{ 0, NULL, __WAIT_QUEUE_INITIALIZER((name).wait) }

void uk_mutex_init(struct uk_mutex *m);

static inline void uk_mutex_lock(struct uk_mutex *m)
{
	struct uk_thread *current;
	unsigned long irqf;

	UK_ASSERT(m);

	current = uk_thread_current();

	for (;;) {
		uk_waitq_wait_event(&m->wait,
			m->lock_count == 0 || m->owner == current);
		irqf = ukplat_lcpu_save_irqf();
		if (m->lock_count == 0 || m->owner == current)
			break;
		ukplat_lcpu_restore_irqf(irqf);
	}
	m->lock_count++;
	m->owner = current;
	ukplat_lcpu_restore_irqf(irqf);
}

static inline int uk_mutex_trylock(struct uk_mutex *m)
{
	struct uk_thread *current;
	unsigned long irqf;
	int ret = 0;

	UK_ASSERT(m);

	current = uk_thread_current();

	irqf = ukplat_lcpu_save_irqf();
	if (m->lock_count == 0 || m->owner == current) {
		ret = 1;
		m->lock_count++;
		m->owner = current;
	}
	ukplat_lcpu_restore_irqf(irqf);
	return ret;
}

static inline int uk_mutex_is_locked(struct uk_mutex *m)
{
	return m->lock_count > 0;
}

static inline void uk_mutex_unlock(struct uk_mutex *m)
{
	unsigned long irqf;

	UK_ASSERT(m);

	irqf = ukplat_lcpu_save_irqf();
	UK_ASSERT(m->lock_count > 0);
	if (--m->lock_count == 0) {
		m->owner = NULL;
		uk_waitq_wake_up(&m->wait);
	}
	ukplat_lcpu_restore_irqf(irqf);
}

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LIBUKLOCK_MUTEX */

#endif /* __UK_MUTEX_H__ */
