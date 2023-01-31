/* SPDX-License-Identifier: BSD-3-Clause */
/*
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

#ifndef __UK_SPINLOCK_H__
#define __UK_SPINLOCK_H__

#include <uk/plat/lcpu.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/* See uk/arch/spinlock.h for the interface documentation */

#ifndef CONFIG_LIBUKLOCK_TICKETLOCK

#ifndef uk_spinlock
#include <uk/arch/spinlock.h>

#define uk_spinlock __spinlock

#define UK_SPINLOCK_INITIALIZER()  UKARCH_SPINLOCK_INITIALIZER()
#define uk_spin_init(lock)         ukarch_spin_init(lock)
#define uk_spin_lock(lock)         ukarch_spin_lock(lock)
#define uk_spin_unlock(lock)       ukarch_spin_unlock(lock)
#define uk_spin_trylock(lock)      ukarch_spin_trylock(lock)
#define uk_spin_is_locked(lock)    ukarch_spin_is_locked(lock)
#endif /* uk_spinlock */

#else	/* CONFIG_LIBUKLOCK_TICKETLOCK */

#ifndef uk_spinlock

#ifdef CONFIG_ARCH_ARM_64
#include <uk/arch/arm64/ticketlock.h>
#endif

#define uk_spinlock __ticketlock

#define UK_SPINLOCK_INITIALIZER()  UKARCH_TICKETLOCK_INITIALIZER()
#define uk_spin_init(lock)         ukarch_ticket_init(lock)
#define uk_spin_lock(lock)         ukarch_ticket_lock(lock)
#define uk_spin_unlock(lock)       ukarch_ticket_unlock(lock)
#define uk_spin_trylock(lock)      ukarch_ticket_trylock(lock)
#define uk_spin_is_locked(lock)    ukarch_ticket_is_locked(lock)
#endif /* uk_spinlock */

#endif	/* CONFIG_LIBUKLOCK_TICKETLOCK */

#define uk_spin_lock_irq(lock)						\
	do {								\
		ukplat_lcpu_disable_irq();				\
		uk_spin_lock(lock);					\
	} while (0)

#define uk_spin_unlock_irq(lock)					\
	do {								\
		uk_spin_unlock(lock);					\
		ukplat_lcpu_enable_irq();				\
	} while (0)

#define uk_spin_trylock_irq(lock)					\
	do {								\
		ukplat_lcpu_disable_irq();				\
		if (unlikely(uk_spin_trylock(lock) == 0))		\
			ukplat_lcpu_enable_irq();			\
	} while (0)

#define uk_spin_lock_irqsave(lock, flags)				\
	do {								\
		flags = ukplat_lcpu_save_irqf();			\
		uk_spin_lock(lock);					\
	} while (0)

#define uk_spin_unlock_irqrestore(lock, flags)				\
	do {								\
		uk_spin_unlock(lock);					\
		ukplat_lcpu_restore_irqf(flags);			\
	} while (0)

#define uk_spin_trylock_irqsave(lock, flags)				\
	do {								\
		flags = ukplat_lcpu_save_irqf();			\
		if (unlikely(uk_spin_trylock(lock) == 0))		\
			ukplat_lcpu_restore_irqf(flags);		\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __UK_SPINLOCK_H__ */
