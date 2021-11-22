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

static inline void ukarch_ticket_init(struct __ticketlock *lock)
{
	lock->next = 0;
	lock->current = 0;
}

static inline void ukarch_ticket_lock(struct __ticketlock *lock)
{
	unsigned int r, r1, r2;

	__asm__ __volatile__(
		"	prfm pstl1keep, [%3]\n"      /* preload lock */
		"1:	ldaxr	%w0, [%3]\n"         /* read current/next */
		"	add	%w1, %w0, #0x10000\n"/* increment next */
		"	stxr	%w2, %w1, [%3]\n"    /* try store-exclusive */
		"	cbnz	%w2, 1b\n"           /* retry if excl failed */
		"	and	%w2, %w0, #0xffff\n" /* get current */
		"	cmp	%w2, %w0, lsr #16\n" /* is it our ticket? */
		"	b.eq	3f\n"                /* if yes, lock acquired */
		"	sevl\n"                      /* invalidate next wfe */
		"2:	wfe\n"                       /* wait for unlock event */
		"	ldaxrh	%w2, [%3]\n"         /* load current */
		"	cmp	%w2, %w0, lsr #16\n" /* is it our ticket? */
		"	b.ne	2b\n"                /* if not, try again */
		"3:"                                 /* critical section */
		: "=&r" (r), "=&r" (r1), "=&r" (r2)
		: "r" (lock)
		: "memory");
}

static inline void ukarch_ticket_unlock(struct __ticketlock *lock)
{
	unsigned int r;

	__asm__ __volatile__(
		"	ldrh	%w0, %1\n"     /* read current */
		"	add	%w0, %w0, #1\n"/* increment current */
		"	stlrh	%w0, %1\n"     /* update lock */
		: "=&r" (r), "=Q" (lock->current)
		:
		: "memory");
}

static inline int ukarch_ticket_trylock(struct __ticketlock *lock)
{
	unsigned int r, r1;

	__asm__ __volatile__(
		"	prfm pstl1keep, [%2]\n"          /* preload lock */
		"1:	ldaxr	%w0, [%2]\n"             /* read current/next */
		"	eor	%w1, %w0, %w0, ror #16\n"/* current == next ? */
		"	cbnz	%w1, 2f\n"               /* bail if locked */
		"	add	%w1, %w0, #0x10000\n"    /* increment next */
		"	stxr	%w0, %w1, [%2]\n"        /* try update next */
		"	cbnz	%w0, 1b\n"               /* retry if failed */
		"2:"
		: "=&r" (r), "=&r" (r1)
		: "r" (lock)
		: "memory");
	return !r;
}

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
