/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2007-2009 Kip Macy <kmacy@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 *
 */

#ifndef __UK_RING_H__
#define __UK_RING_H__

#include <errno.h>
#include <uk/mutex.h>
#include <uk/print.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/plat/lcpu.h>
#include <uk/arch/atomic.h>
#include <uk/essentials.h>
#include <uk/preempt.h>

#define critical_enter()  uk_preempt_disable()
#define critical_exit()   uk_preempt_enable()

#define __UK_RING_NAME0(x, ...) x
#define __UK_RING_NAME1(x, a1) UK_CONCAT(__UK_RING_NAME0(x), _ ## a1)
#define __UK_RING_NAME2(x, a1, a2) UK_CONCAT(__UK_RING_NAME1(x, a1), _ ## a2)
#define   UK_RING_NAME(x, ...) \
	UK_CONCAT(__UK_RING_NAME, UK_NARGS(__VA_ARGS__))(x, __VA_ARGS__)

struct uk_ring {
	volatile uint32_t br_prod_head;
	volatile uint32_t br_prod_tail;
	int               br_prod_size;
	int               br_prod_mask;
	uint64_t          br_drops;
	volatile uint32_t br_cons_head __aligned(CACHE_LINE_SIZE);
	volatile uint32_t br_cons_tail;
	int               br_cons_size;
	int               br_cons_mask;
#ifdef DEBUG_BUFRING
	struct uk_mutex  *br_lock;
#endif
	void             *br_ring[0] __aligned(CACHE_LINE_SIZE);
};

/*
 * multi-producer safe lock-free ring buffer enqueue
 *
 */
static __inline int
uk_ring_enqueue(struct uk_ring *br, void *buf)
{
	uint32_t prod_head, prod_next, cons_tail;
#ifdef DEBUG_BUFRING
	int i;

	/*
	 * Note: It is possible to encounter an mbuf that was removed
	 * via drbr_peek(), and then re-added via drbr_putback() and
	 * trigger a spurious panic.
	 */
	for (i = br->br_cons_head; i != br->br_prod_head;
			 i = ((i + 1) & br->br_cons_mask))
		if (br->br_ring[i] == buf)
			UK_CRASH("buf=%p already enqueue at %d prod=%d cons=%d",
					buf, i, br->br_prod_tail, br->br_cons_tail);
#endif
	critical_enter();
	do {
		prod_head = br->br_prod_head;
		prod_next = (prod_head + 1) & br->br_prod_mask;
		cons_tail = br->br_cons_tail;

		if (prod_next == cons_tail) {
			rmb();
			if (prod_head == br->br_prod_head && cons_tail == br->br_cons_tail) {
				br->br_drops++;
				critical_exit();
				return -ENOBUFS;
			}
			continue;
		}
	} while (!ukarch_compare_exchange_sync((uint32_t *) &br->br_prod_head,
			prod_head, prod_next));

#ifdef DEBUG_BUFRING
	if (br->br_ring[prod_head] != NULL)
		UK_CRASH("dangling value in enqueue");
#endif
	br->br_ring[prod_head] = buf;

	/*
	 * If there are other enqueues in progress
	 * that preceded us, we need to wait for them
	 * to complete 
	 */
	while (br->br_prod_tail != prod_head)
		ukarch_spinwait();
	ukarch_store_n(&br->br_prod_tail, prod_next);
	critical_exit();
	return 0;
}

/*
 * multi-consumer safe dequeue 
 *
 */
static __inline void *
uk_ring_dequeue_mc(struct uk_ring *br)
{
	uint32_t cons_head, cons_next;
	void *buf;

	critical_enter();
	do {
		cons_head = br->br_cons_head;
		cons_next = (cons_head + 1) & br->br_cons_mask;

		if (cons_head == br->br_prod_tail) {
			critical_exit();
			return NULL;
		}
	} while (!ukarch_compare_exchange_sync((uint32_t *) &br->br_cons_head,
			cons_head, cons_next));

	buf = br->br_ring[cons_head];
#ifdef DEBUG_BUFRING
	br->br_ring[cons_head] = NULL;
#endif
	/*
	 * If there are other dequeues in progress
	 * that preceded us, we need to wait for them
	 * to complete
	 */
	while (br->br_cons_tail != cons_head)
		ukarch_spinwait();

	ukarch_store_n(&br->br_cons_tail, cons_next);
	critical_exit();

	return buf;
}

/*
 * single-consumer dequeue 
 * use where dequeue is protected by a lock
 * e.g. a network driver's tx queue lock
 */
static __inline void *
uk_ring_dequeue_sc(struct uk_ring *br)
{
	uint32_t cons_head, cons_next;
#ifdef PREFETCH_DEFINED
	uint32_t cons_next_next;
#endif
	uint32_t prod_tail;
	void *buf;

	/*
	 * This is a workaround to allow using uk_ring on ARM and ARM64.
	 * ARM64TODO: Fix uk_ring in a generic way.
	 * REMARKS: It is suspected that br_cons_head does not require
	 *   load_acq operation, but this change was extensively tested
	 *   and confirmed it's working. To be reviewed once again in
	 *   FreeBSD-12.
	 *
	 * Preventing following situation:
	 * Core(0) - uk_ring_enqueue()                                       Core(1) - uk_ring_dequeue_sc()
	 * -----------------------------------------                                       ----------------------------------------------
	 *
	 *                                                                                cons_head = br->br_cons_head;
	 * atomic_cmpset_acq_32(&br->br_prod_head, ...));
	 *                                                                                buf = br->br_ring[cons_head];     <see <1>>
	 * br->br_ring[prod_head] = buf;
	 * atomic_store_rel_32(&br->br_prod_tail, ...);
	 *                                                                                prod_tail = br->br_prod_tail;
	 *                                                                                if (cons_head == prod_tail) 
	 *                                                                                        return (NULL);
	 *                                                                                <condition is false and code uses invalid(old) buf>`
	 *
	 * <1> Load (on core 1) from br->br_ring[cons_head] can be reordered (speculative readed) by CPU.
	 */
#if defined(CONFIG_ARCH_ARM_32) || defined(CONFIG_ARCH_ARM_64)
	cons_head = ukarch_load_n(&br->br_cons_head);
#else
	cons_head = br->br_cons_head;
#endif
	prod_tail = ukarch_load_n(&br->br_prod_tail);

	cons_next = (cons_head + 1) & br->br_cons_mask;
#ifdef PREFETCH_DEFINED
	cons_next_next = (cons_head + 2) & br->br_cons_mask;
#endif

	if (cons_head == prod_tail)
		return NULL;

#ifdef PREFETCH_DEFINED
	if (cons_next != prod_tail) {
		prefetch(br->br_ring[cons_next]);
		if (cons_next_next != prod_tail)
			prefetch(br->br_ring[cons_next_next]);
	}
#endif
	br->br_cons_head = cons_next;
	buf = br->br_ring[cons_head];

#ifdef DEBUG_BUFRING
	br->br_ring[cons_head] = NULL;
	if (!uk_mutex_is_locked(br->br_lock))
		UK_CRASH("lock not held on single consumer dequeue: %d", br->br_lock->lock_count);
	if (br->br_cons_tail != cons_head)
		UK_CRASH("inconsistent list cons_tail=%d cons_head=%d",
				br->br_cons_tail, cons_head);
#endif
	br->br_cons_tail = cons_next;
	return buf;
}

/*
 * single-consumer advance after a peek
 * use where it is protected by a lock
 * e.g. a network driver's tx queue lock
 */
static __inline void
uk_ring_advance_sc(struct uk_ring *br)
{
	uint32_t cons_head, cons_next;
	uint32_t prod_tail;

	cons_head = br->br_cons_head;
	prod_tail = br->br_prod_tail;

	cons_next = (cons_head + 1) & br->br_cons_mask;
	if (cons_head == prod_tail)
		return;
	br->br_cons_head = cons_next;
#ifdef DEBUG_BUFRING
	br->br_ring[cons_head] = NULL;
#endif
	br->br_cons_tail = cons_next;
}

/*
 * Used to return a buffer (most likely already there)
 * to the top of the ring. The caller should *not*
 * have used any dequeue to pull it out of the ring
 * but instead should have used the peek() function.
 * This is normally used where the transmit queue
 * of a driver is full, and an mbuf must be returned.
 * Most likely whats in the ring-buffer is what
 * is being put back (since it was not removed), but
 * sometimes the lower transmit function may have
 * done a pullup or other function that will have
 * changed it. As an optimization we always put it
 * back (since jhb says the store is probably cheaper),
 * if we have to do a multi-queue version we will need
 * the compare and an atomic.
 */
static __inline void
uk_ring_putback_sc(struct uk_ring *br, void *new)
{
	/* Buffer ring has none in putback */
	UK_ASSERT(br->br_cons_head != br->br_prod_tail);
	br->br_ring[br->br_cons_head] = new;
}

/*
 * return a pointer to the first entry in the ring
 * without modifying it, or NULL if the ring is empty
 * race-prone if not protected by a lock
 */
static __inline void *
uk_ring_peek(struct uk_ring *br)
{
#ifdef DEBUG_BUFRING
	if (!uk_mutex_is_locked(br->br_lock))
		UK_CRASH("lock not held on single consumer dequeue");
#endif
	/*
	 * I believe it is safe to not have a memory barrier
	 * here because we control cons and tail is worst case
	 * a lagging indicator so we worst case we might
	 * return NULL immediately after a buffer has been enqueued
	 */
	if (br->br_cons_head == br->br_prod_tail)
		return NULL;

	return br->br_ring[br->br_cons_head];
}

static __inline void *
uk_ring_peek_clear_sc(struct uk_ring *br)
{
#ifdef DEBUG_BUFRING
	void *ret;

	if (!uk_mutex_is_locked(br->br_lock))
		UK_CRASH("lock not held on single consumer dequeue");
#endif

	if (br->br_cons_head == br->br_prod_tail)
		return NULL;

#if defined(CONFIG_ARCH_ARM_32) || defined(CONFIG_ARCH_ARM_64)
	/*
	 * The barrier is required there on ARM and ARM64 to ensure, that
	 * br->br_ring[br->br_cons_head] will not be fetched before the above
	 * condition is checked.
	 * Without the barrier, it is possible, that buffer will be fetched
	 * before the enqueue will put mbuf into br, then, in the meantime, the
	 * enqueue will update the array and the br_prod_tail, and the
	 * conditional check will be true, so we will return previously fetched
	 * (and invalid) buffer.
	 */
	#error "unsupported: atomic_thread_fence_acq()"
	/* TODO atomic_thread_fence_acq(); */
#endif

#ifdef DEBUG_BUFRING
	/*
	 * Single consumer, i.e. cons_head will not move while we are
	 * running, so atomic_swap_ptr() is not necessary here.
	 */
	ret = br->br_ring[br->br_cons_head];
	br->br_ring[br->br_cons_head] = NULL;
	return ret;
#else
	return br->br_ring[br->br_cons_head];
#endif
}

static __inline int
uk_ring_full(struct uk_ring *br)
{
	return ((br->br_prod_head + 1) & br->br_prod_mask) == br->br_cons_tail;
}

static __inline int
uk_ring_empty(struct uk_ring *br)
{
	return br->br_cons_head == br->br_prod_tail;
}

static __inline int
uk_ring_count(struct uk_ring *br)
{
	return (br->br_prod_size + br->br_prod_tail - br->br_cons_tail)
			& br->br_prod_mask;
}

static struct uk_ring *
uk_ring_alloc(unsigned int count, size_t elemsize, struct uk_alloc *a
#ifdef DEBUG_BUFRING
		, struct uk_mutex *lock
#endif
)
{
	struct uk_ring *br;

	/* buf ring must be size power of 2 */
	UK_ASSERT(POWER_OF_2(count));

	/* Limit the size of each element to the maximum pointer size as the buffer
	 * should not handle large elements. */
	UK_ASSERT(elemsize <= sizeof(void *));

	br = uk_malloc(a, sizeof(struct uk_ring) + count * elemsize);
	if (br == NULL)
		return NULL;
#ifdef DEBUG_BUFRING
	br->br_lock = lock;
#endif
	br->br_prod_size = br->br_cons_size = count;
	br->br_prod_mask = br->br_cons_mask = count - 1;
	br->br_prod_head = br->br_cons_head = 0;
	br->br_prod_tail = br->br_cons_tail = 0;

	return br;
}

static void
uk_ring_free(struct uk_ring *br, struct uk_alloc *a)
{
	uk_free(a, br);
}

#endif

