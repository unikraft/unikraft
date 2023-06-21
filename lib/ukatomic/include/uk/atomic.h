/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Port from Mini-OS: include/arm/os.h
 */
/*
 * Copyright (c) 2009 Citrix Systems, Inc. All rights reserved.
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

#ifndef __UK_ATOMIC_H__
#define __UK_ATOMIC_H__

#include <uk/arch/lcpu.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Perform an atomic load operation.
 */
#define uk_load_n(src) \
	__atomic_load_n(src, __ATOMIC_SEQ_CST)

/**
 * Perform an atomic store operation.
 */
#define uk_store_n(src, value) \
	__atomic_store_n(src, value, __ATOMIC_SEQ_CST)

/**
 * Perform an atomic fetch and add/sub operation.
 */
#define uk_fetch_add(src, value) \
	__atomic_fetch_add(src, value, __ATOMIC_SEQ_CST)
#define uk_fetch_sub(src, value) \
	__atomic_fetch_sub(src, value, __ATOMIC_SEQ_CST)

/**
 * Perform an atomic add/sub and fetch operation.
 */
#define uk_add_fetch(src, value) \
	__atomic_add_fetch(src, value, __ATOMIC_SEQ_CST)
#define uk_sub_fetch(src, value) \
	__atomic_sub_fetch(src, value, __ATOMIC_SEQ_CST)

/**
 * Perform an atomic increment/decrement operation and return the
 * previous value.
 */
#define uk_inc(src) \
	uk_fetch_add(src, 1)
#define uk_dec(src) \
	uk_fetch_sub(src, 1)

/**
 * Perform an atomic OR operation and return the previous value.
 */
#define uk_or(src, val) \
	__atomic_fetch_or(src, val, __ATOMIC_SEQ_CST)

/**
 * Perform an atomic AND operation and return the previous value.
 */
#define uk_and(src, val) \
	__atomic_fetch_and(src, val, __ATOMIC_SEQ_CST)

/**
 * Writes *src into *dst, and returns the previous contents of *dst.
 */
#define uk_exchange(dst, src) \
	__atomic_exchange(dst, src, __ATOMIC_SEQ_CST)

/**
 * Writes v into *dst, and returns the previous contents of *dst.
 */
#define uk_exchange_n(dst, v) \
	__atomic_exchange_n(dst, v, __ATOMIC_SEQ_CST)

#define uk_compare_exchange_n(dst, exp, des)  \
	__atomic_compare_exchange_n(dst, exp, des, 0, \
	                            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define uk_compare_exchange_sync(ptr, old, new)                            \
	({                                                                     \
		__typeof__(*ptr) stored = old;                                 \
		__atomic_compare_exchange_n(                                   \
		    ptr, &stored, new, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)  \
		    ? new                                                      \
		    : old;                                                     \
	})

#define	UK_ACCESS_ONCE(x)			(*(volatile __typeof(x) *)&(x))

#define	UK_WRITE_ONCE(x, v) do {	\
	barrier();			\
	UK_ACCESS_ONCE(x) = (v);	\
	barrier();			\
} while (0)

#define	UK_READ_ONCE(x) ({		\
	__typeof(x) __var = ({		\
		barrier();		\
		UK_ACCESS_ONCE(x);	\
	});				\
	barrier();			\
	__var;				\
})

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UK_ATOMIC_H__ */