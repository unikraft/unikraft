/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, Michalis Pappas <mpappas@fastmail.fm>
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
 */
#ifndef __UKARCH_TICKETLOCK_H__
#define __UKARCH_TICKETLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/arch/lcpu.h>

#ifdef CONFIG_HAVE_SMP
#include <uk/arch/atomic.h>

/* Unless you know what you are doing, use struct uk_spinlock instead. */
typedef struct __ticketlock __ticketlock;

struct __align(8) __ticketlock {
	__u16	current; /* currently served */
	__u16	next;	 /* next available ticket */
} __packed;

/* Initialize a ticketlock to unlocked state */
#define UKARCH_TICKETLOCK_INITIALIZER() { 0 }


/**
 *  Initialize a ticketlock.
 *  Current and next tickets are set to 0.
 *
 * @param lock
 *     The ticketlock to be initialized.
 */
static inline void ukarch_ticket_init(struct __ticketlock *lock)
{
	lock->next = 0;
	lock->current = 0;
}

/**
 *  Let a thread acquire a ticketlock.
 *  This happens only if the thread has appropriate ticket number.
 *  This function will block until the lock is acquired.
 *
 * @param lock
 *     The ticketlock to be acquired.
 */

static inline void ukarch_ticket_lock(struct __ticketlock *lock)
{
	unsigned int r, r1, r2;

	__asm__ __volatile__(
		/* preload lock */
		"	prfm pstl1keep, [%3]\n"

		/* read current/next */
		"1:	ldaxr	%w0, [%3]\n"
		/* increment next */
		"	add	%w1, %w0, #0x10000\n"
		/* try store-exclusive */
		"	stxr	%w2, %w1, [%3]\n"
		/* retry if excl failed */
		"	cbnz	%w2, 1b\n"
		/* get current */
		"	and	%w2, %w0, #0xffff\n"
		/* is it our ticket? */
		"	cmp	%w2, %w0, lsr #16\n"
		/* if yes, lock acquired */
		"	b.eq	3f\n"
		/* invalidate next wfe */
		"	sevl\n"

		/* wait for unlock event */
		"2:	wfe\n"
		/* load current */
		"	ldaxrh	%w2, [%3]\n"
		/* is it our ticket? */
		"	cmp	%w2, %w0, lsr #16\n"
		/* if not, try again */
		"	b.ne	2b\n"

		/* critical section */
		"3:"
		: "=&r" (r), "=&r" (r1), "=&r" (r2)
		: "r" (lock)
		: "memory");
}

/**
 *  Releases the ticketlock.
 *  This will increment the current value of the lock by 1.
 *
 * @param lock
 *     The ticketlock to be released.
 */
static inline void ukarch_ticket_unlock(struct __ticketlock *lock)
{
	unsigned int r;

	__asm__ __volatile__(
		/* read current */
		"	ldrh	%w0, %1\n"
		/* increment current */
		"	add	%w0, %w0, #1\n"
		/* update lock */
		"	stlrh	%w0, %1\n"
		: "=&r" (r), "=Q" (lock->current)
		:
		: "memory");
}

/**
 *  Attempts to acquire a ticketlock.
 *  If current == next, the lock is acquired.
 *  Otherwise, increment next value by 1.
 *
 * @param lock
 *     The ticketlock to be released.
 * @return
 *     0 if the lock was acquired, 1 otherwise.
 */
static inline int ukarch_ticket_trylock(struct __ticketlock *lock)
{
	unsigned int r, r1;

	__asm__ __volatile__(
		/* preload lock */
		"	prfm pstl1keep, [%2]\n"
		/* read current/next */
		"1:	ldaxr	%w0, [%2]\n"
		/* current == next ? */
		"	eor	%w1, %w0, %w0, ror #16\n"
		/* bail if locked */
		"	cbnz	%w1, 2f\n"
		/* increment next */
		"	add	%w1, %w0, #0x10000\n"
		/* try update next */
		"	stxr	%w0, %w1, [%2]\n"
		/* retry if failed */
		"	cbnz	%w0, 1b\n"
		"2:"
		: "=&r" (r), "=&r" (r1)
		: "r" (lock)
		: "memory");
	return !r;
}

/**
 * Check if a ticketlock is locked.
 *
 * @param lock
 *     The ticketlock being checked.
 * @return
 *     1 if the lock is locked, 0 otherwise.
 */
static inline int ukarch_ticket_is_locked(struct __ticketlock *lock)
{
	return !(lock->next == lock->current);
}

#else /* CONFIG_HAVE_SMP */

typedef struct __ticketlock {
	/* empty */
} __ticketlock;

#define UKARCH_TICKETLOCK_INITIALIZER()	{}
#define ukarch_ticket_init(lock)		(void)(lock)
#define ukarch_ticket_lock(lock)		\
	do { barrier(); (void)(lock); } while (0)
#define ukarch_ticket_unlock(lock)	\
	do { barrier(); (void)(lock); } while (0)
#define ukarch_ticket_trylock(lock)	({ barrier(); (void)(lock); 1; })
#define ukarch_ticket_is_locked(lock)	({ barrier(); (void)(lock); 0; })

#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif

#endif /* __UKARCH_TICKETLOCK_H__ */
