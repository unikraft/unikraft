/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2019 OpenSynergy GmbH. All rights reserved.
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

#ifndef __UKARCH_SPINLOCK_H__
#define __UKARCH_SPINLOCK_H__

#include <uk/config.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_UKPLAT_LCPU_MULTICORE
/** spinlock struct */
typedef struct {
	volatile uint32_t lock;
} __spinlock_t;

/** section attribute cannot be used with pointers */
typedef __spinlock_t *spinlock_ptr_t;

/* spinlock type for declaration */
#define spinlock_t __attribute__((section(".slocks"))) __spinlock_t

/**
 * Initialize a spinlock object (put in unlocked state)
 * @param [in/out] lock Pointer to spinlock object
 */
void ukarch_spin_lock_init(spinlock_ptr_t spinlock);

/**
 * Acquire lock. It is guaranteed that the spinlock object will be locked
 * in a atomic way, so no other function can have the same object in a unlocked
 * state.
 * @param [in/out] lock Pointer to spinlock object
 */
void ukarch_spin_lock(spinlock_ptr_t lock);

/**
 * Release lock previously acquired by spin_lock.
 * Use store-release to unconditionally clear the spinlock variable.
 * Store operation generates an event to all cores waiting in WFE
 * when address is monitored by the global monitor.
 * @param [in/out] lock Pointer to spinlock object
 */
void ukarch_spin_unlock(spinlock_ptr_t lock);

/* Defines a pre-initialized spin_lock in unlocked state */
#define DEFINE_SPINLOCK(lock)            spinlock_t lock = { 0 }

int ukarch_spin_is_locked(spinlock_ptr_t lock)
	__attribute__((error("function not implemented")));
int ukarch_spin_trylock(spinlock_ptr_t lock)
	__attribute__((error("function not implemented")));

#else
typedef struct {} spinlock_t;

#define ukarch_spin_lock_init(lock)      (void)(lock)
#define ukarch_spin_is_locked(lock)      (void)(lock)
#define ukarch_spin_lock(lock)           (void)(lock)
#define ukarch_spin_trylock(lock)        (void)(lock)
#define ukarch_spin_unlock(lock)         (void)(lock)

/* Defines a preinitialized spin_lock in unlocked state */
#define DEFINE_SPINLOCK(lock)            spinlock_t lock = {}

#endif

#ifdef __cplusplus
}
#endif

#endif /* __UKARCH_SPINLOCK_H__ */
