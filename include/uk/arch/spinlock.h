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
/* Taken from Mini-OS */

#ifndef __UKARCH_SPINLOCK_H__
#define __UKARCH_SPINLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {} spinlock_t;

#ifdef CONFIG_SMP
#error "Define your spinlock operations!"
#else

#define ukarch_spin_lock_init(lock)      do {} while (0)
#define ukarch_spin_is_locked(lock)      do {} while (0)
#define ukarch_spin_lock(lock)           do {} while (0)
#define ukarch_spin_trylock(lock)        do {} while (0)
#define ukarch_spin_unlock(lock)         do {} while (0)

/* Defines a preinitialized spin_lock in unlocked state */
#define DEFINE_SPINLOCK(lock)            do {} while (0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* __UKARCH_SPINLOCK_H__ */
