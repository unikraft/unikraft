/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2005 John Baldwin <jhb@FreeBSD.org>
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
 */
/* Taken from the Freebsd and modified
 * commit-id: 81c19620beed0e
 */

#ifndef __UK_REFCOUNT_H__
#define __UK_REFCOUNT_H__

#include <uk/arch/types.h>
#include <uk/arch/limits.h>
#include <uk/arch/atomic.h>
#include <uk/arch/lcpu.h>
#include <uk/config.h>

#ifdef CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#endif /* CONFIG_LIBUKDEBUG */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef CONFIG_LIBUKDEBUG
#define __refcnt_assert(x) UK_ASSERT(x)
#else
#define __refcnt_assert(x) \
	do {               \
	} while (0)
#endif /* CONFIG_LIBUKDEBUG */

/**
 * Initialize the atomic reference.
 *
 * @param ref:
 *	A reference to the atomic data structure.
 * @param value:
 *	A value to initialize.
 */
static inline void uk_refcount_init(__atomic *ref, __u32 value)
{
	__refcnt_assert(ref != __NULL);

	ukarch_store_n(&ref->counter, value);
}

/**
 * Increment the reference counter.
 * @param refs
 *	Reference to the atomic counter.
 */
static inline void uk_refcount_acquire(__atomic *ref)
{
	__refcnt_assert((ref != __NULL) && (ref->counter < __U32_MAX));

	ukarch_inc(&ref->counter);
}

/**
 * Decrement the reference counter.
 * @param refs
 *	Reference to the atomic counter.
 * @return
 *	0: there are more active reference
 *	1: this was let reference to the counter.
 */
static inline int uk_refcount_release(__atomic *ref)
{
	__u32 old;

	__refcnt_assert(ref != __NULL);

	/* Compiler Fence */
	barrier();

	old = ukarch_fetch_add(&ref->counter, -1);
	__refcnt_assert(old > 0);
	if (old > 1)
		return 0;

	/*
	 * Last reference.  Signal the user to call the destructor.
	 *
	 * Ensure that the destructor sees all updates.  The fence_rel
	 * at the start of the function synchronized with this fence.
	 */
	barrier();
	return 1;
}

/**
 * Increment the reference counter if it was already in use.
 * @param refs
 *	Reference to the atomic counter.
 * @return
 *	0: Failed to acquire the counter.
 *	1: Success in acquiring the counter.
 */
static inline int uk_refcount_acquire_if_not_zero(__atomic *ref)
{
	__u32 old;

	__refcnt_assert(ref != __NULL && ref->counter < __U32_MAX);

	old = ref->counter;
	for (;;) {
		if (old == 0)
			return 0;
		if (ukarch_compare_exchange_sync(&ref->counter, old, (old + 1))
				== (old + 1))
			return 1;
	}
}

/**
 * refcount_read - get a refcount's value
 * @r: the refcount
 *
 * Return: the refcount's value
 */
static inline __u32 uk_refcount_read(const __atomic *ref)
{
	__refcnt_assert(ref != __NULL);

	return ukarch_load_n(&ref->counter);
}


/**
 * Decrement the reference counter if there are multiple users of the counter.
 * @param refs
 *	Reference to the atomic counter.
 * @return
 *	0: Failed to acquire the counter.
 *	1: Success in acquiring the counter.
 */
static inline int uk_refcount_release_if_not_last(__atomic *ref)
{
	__u32 old;

	__refcnt_assert(ref != __NULL);

	old = ref->counter;
	for (;;) {
		if (old == 1)
			return 0;
		if (ukarch_compare_exchange_sync(&ref->counter, old, (old - 1))
				== (old - 1))
			return 1;
	}
}

#undef __refcnt_assert
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UK_REFCOUNT_H__*/
