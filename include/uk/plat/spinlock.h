/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, Politehnica University of Bucharest. All rights reserved.
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

#ifndef __UKPLAT_SPINLOCK_H__
#define __UKPLAT_SPINLOCK_H__

#include <uk/arch/spinlock.h>
#include <uk/plat/lcpu.h>

#define ukplat_spin_lock_irq(lock) \
	do { \
		ukplat_lcpu_disable_irq(); \
		ukarch_spin_lock(lock); \
	} while (0)

#define ukplat_spin_unlock_irq(lock) \
	do { \
		ukarch_spin_unlock(lock); \
		ukplat_lcpu_enable_irq(); \
	} while (0)

#define ukplat_spin_lock_irqsave(lock, flags) \
	do { \
		flags = ukplat_lcpu_save_irqf(); \
		ukarch_spin_lock(lock); \
	} while (0)

#define ukplat_spin_unlock_irqrestore(lock, flags) \
	do { \
		ukarch_spin_unlock(lock); \
		ukplat_lcpu_restore_irqf(flags); \
	} while (0)

#endif /* __PLAT_SPINLOCK_H__ */
