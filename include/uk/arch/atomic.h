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

#ifndef __UKARCH_ATOMIC_H__
#define __UKARCH_ATOMIC_H__

#include <uk/arch/lcpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/asm/atomic.h>

/**
 * Perform a atomic load operation.
 */
#define ukarch_load_n(src) \
	__atomic_load_n(src, __ATOMIC_SEQ_CST)

/**
 * Perform a atomic store operation.
 */
#define ukarch_store_n(src, value) \
	__atomic_store_n(src, value, __ATOMIC_SEQ_CST)

/**
 * Perform a atomic fetch and add operation.
 */
#define ukarch_fetch_add(src, value) \
	__atomic_fetch_add(src, value, __ATOMIC_SEQ_CST)

/**
 * Perform a atomic increment/decrement operation and return the
 * previous value.
 */
#define ukarch_inc(src) \
	ukarch_fetch_add(src, 1)
#define ukarch_dec(src) \
	__atomic_fetch_sub(src, 1, __ATOMIC_SEQ_CST)
/**
 * Writes *src into *dst, and returns the previous contents of *dst.
 */
#define ukarch_exchange(dst, src) \
	__atomic_exchange(dst, src, __ATOMIC_SEQ_CST)

/**
 * Writes v into *dst, and returns the previous contents of *dst.
 */
#define ukarch_exchange_n(dst, v) \
	__atomic_exchange_n(dst, v, __ATOMIC_SEQ_CST)

#define ukarch_compare_exchange_sync(ptr, old, new)                            \
	({                                                                     \
		__typeof__(*ptr) stored = old;                                 \
		__atomic_compare_exchange_n(                                   \
		    ptr, &stored, new, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)  \
		    ? new                                                      \
		    : old;                                                     \
	})

#ifdef __cplusplus
}
#endif

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

#endif /* __UKARCH_ATOMIC_H__ */
