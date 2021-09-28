/* SPDX-License-Identifier: BSD-3-Clause */
/*
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

#include <uk/arch/atomic.h>

struct __spinlock {
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
	register int locked = 1;

	__asm__ __volatile__(
		"1:	mov	(%0), %%eax\n"	/* read current value */
		"	test	%%eax, %%eax\n"	/* check if locked */
		"	jz	3f\n"		/* if not locked, try get it */
		"2:	pause\n"		/* is locked, hint spinning */
		"	jmp	1b\n"		/* retry */
		"3:	lock; cmpxchg %1, (%0)\n" /* try to acquire spinlock */
		"	jnz	2b\n"		/* if unsuccessful, retry */
		:
		: "r" (&lock->lock), "r" (locked)
		: "eax");
}

static inline void ukarch_spin_unlock(struct __spinlock *lock)
{
	UK_WRITE_ONCE(lock->lock, 0);
}

static inline int ukarch_spin_trylock(struct __spinlock *lock)
{
	register int r = 0, locked = 1;

	__asm__ __volatile__(
		"	mov	(%1), %%eax\n"	/* read current value */
		"	test	%%eax, %%eax\n"	/* bail out if locked */
		"	jnz	1f\n"
		"	lock; cmpxchg %2, (%1)\n" /* try to acquire spinlock */
		"	cmove	%2, %0\n"	/* store if successful */
		"1:\n"
		: "+&r" (r)
		: "r" (&lock->lock), "r" (locked)
		: "eax");

	return r;
}

static inline int ukarch_spin_is_locked(struct __spinlock *lock)
{
	return UK_READ_ONCE(lock->lock);
}
