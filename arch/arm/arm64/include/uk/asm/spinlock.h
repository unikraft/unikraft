/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2021 OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2021 Karlsruhe Institute of Technology (KIT).
 *               All rights reserved.
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
#ifndef __UKARCH_SPINLOCK_H__
#error Do not include this header directly
#endif

#include <uk/essentials.h>
#include <uk/arch/atomic.h>

struct __align(8) __spinlock {
	volatile int lock;
};

/* Initialize a spinlock to unlocked state */
#define UKARCH_SPINLOCK_INITIALIZER() { 0 }

static inline void ukarch_spin_init(struct __spinlock *lock)
{
	lock->lock = 0;
}

static inline void ukarch_spin_lock(struct __spinlock *lock)
{
	register int r, locked = 1;

	__asm__ __volatile__(
		"	sevl\n"			/* set event locally */
		"1:	wfe\n"			/* wait for event */
		"2:	ldaxr	%w0, [%1]\n"	/* exclusive load lock value */
		"	cbnz	%w0, 1b\n"	/* check if already locked */
		"	stxr	%w0, %w2, [%1]\n"/* try to lock it */
		"	cbnz	%w0, 2b\n"	/* jump to l2 if we failed */
		: "=&r" (r)
		: "r" (&lock->lock), "r" (locked));
}

static inline void ukarch_spin_unlock(struct __spinlock *lock)
{
	__asm__ __volatile__(
		"stlr	wzr, [%0]\n"		/* unlock lock */
		"sev\n"				/* wake up any waiters */
		:
		: "r" (&lock->lock));
}

static inline int ukarch_spin_trylock(struct __spinlock *lock)
{
	register int r, locked = 1;

	__asm__ __volatile__(
		"	ldaxr	%w0, [%1]\n"	/* exclusive load lock value */
		"	cbnz	%w0, 1f\n"	/* bail out if locked */
		"	stxr	%w0, %w2, [%1]\n"/* try to lock it */
		"1:\n"
		: "=&r" (r)
		: "r" (&lock->lock), "r" (locked));

	return !r;
}

static inline int ukarch_spin_is_locked(struct __spinlock *lock)
{
	return UK_READ_ONCE(lock->lock);
}
