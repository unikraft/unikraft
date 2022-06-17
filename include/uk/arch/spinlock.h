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

#ifndef __UKARCH_SPINLOCK_H__
#define __UKARCH_SPINLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/config.h>
#include <uk/arch/lcpu.h>

#ifdef CONFIG_HAVE_SMP
#include <uk/asm/spinlock.h>

/* Unless you know what you are doing, use struct uk_spinlock instead. */
typedef struct __spinlock __spinlock;

/**
 * UKARCH_SPINLOCK_INITIALIZER() macro
 *
 * Statically initialize a spinlock to unlocked state.
 */
#ifndef UKARCH_SPINLOCK_INITIALIZER
#error The spinlock implementation must define UKARCH_SPINLOCK_INITIALIZER
#endif /* UKARCH_SPINLOCK_INITIALIZER */

/**
 * Initialize a spinlock to unlocked state.
 *
 * @param [in,out] lock Pointer to spinlock.
 */
void ukarch_spin_init(__spinlock *lock);

/**
 * Acquire spinlock. It is guaranteed that the spinlock will be held
 * exclusively.
 *
 * @param [in,out] lock Pointer to spinlock.
 */
void ukarch_spin_lock(__spinlock *lock);

/**
 * Release previously acquired spinlock.
 *
 * @param [in,out] lock Pointer to spinlock.
 */
void ukarch_spin_unlock(__spinlock *lock);

/**
 * Try to acquire spinlock. If the lock is already acquired (busy), this
 * function returns instead of spinning.
 *
 * @param [in,out] lock Pointer to spinlock.
 *
 * @return A non-zero value if spinlock was acquired, 0 otherwise.
 */
int ukarch_spin_trylock(__spinlock *lock);

/**
 * Read spinlock state. No lock/unlock operations are performed on the lock.
 *
 * @param [in,out] lock Pointer to spinlock.
 *
 * @return A non-zero value if spinlock is acquired, 0 otherwise.
 */
int ukarch_spin_is_locked(__spinlock *lock);

#else /* CONFIG_HAVE_SMP */
/* N.B.: For single-core systems we remove spinlocks by mapping functions to
 * void operations. No state is saved for the spinlock but we assume that the
 * CPU is always able to get the lock. Trying to acquire a spinlock thus always
 * succeeds and asserting that a lock is held returns false.
 */
typedef struct __spinlock {
	/* empty */
} __spinlock;

#define UKARCH_SPINLOCK_INITIALIZER()	{}
#define ukarch_spin_init(lock)		(void)(lock)
#define ukarch_spin_lock(lock)		\
	do { barrier(); (void)(lock); } while (0)
#define ukarch_spin_unlock(lock)	\
	do { barrier(); (void)(lock); } while (0)
#define ukarch_spin_trylock(lock)	({ barrier(); (void)(lock); 1; })
#define ukarch_spin_is_locked(lock)	({ barrier(); (void)(lock); 0; })

#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif

#endif /* __UKARCH_SPINLOCK_H__ */
