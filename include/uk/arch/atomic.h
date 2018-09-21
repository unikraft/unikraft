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
 * Perform a atomic increment operation.
 */
#define ukarch_inc(src) \
	ukarch_fetch_add(src, 1)

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

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 *
 * This operation is atomic.
 * If you need a memory barrier, use synch_test_and_clear_bit instead.
 */
static inline int ukarch_test_and_clr_bit(unsigned int nr, volatile void *byte)
{
	__u8 *addr = ((__u8 *)byte) + (nr >> 3);
	__u8 bit = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_and(addr, ~bit, __ATOMIC_RELAXED);

	return (orig & bit) != 0;
}

/**
 * Atomically set a bit and return the old value.
 * Similar to test_and_clear_bit.
 */
static inline int ukarch_test_and_set_bit(unsigned int nr, volatile void *byte)
{
	__u8 *addr = ((__u8 *)byte) + (nr >> 3);
	__u8 bit = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_or(addr, bit, __ATOMIC_RELAXED);

	return (orig & bit) != 0;
}

/**
 * Test whether a bit is set.
 */
static inline int ukarch_test_bit(unsigned int nr,
					const volatile unsigned long *byte)
{
	const __u8 *ptr = (const __u8 *)byte;

	return ((1 << (nr & 7)) & (ptr[nr >> 3])) != 0;
}

/**
 * Atomically set a bit in memory (like test_and_set_bit but discards result).
 */
static inline void ukarch_set_bit(unsigned int nr,
					volatile unsigned long *byte)
{
	ukarch_test_and_set_bit(nr, byte);
}

/**
 * Atomically clear a bit in memory (like test_and_clear_bit but discards
 * result).
 */
static inline void ukarch_clr_bit(unsigned int nr,
					volatile unsigned long *byte)
{
	ukarch_test_and_clr_bit(nr, byte);
}

/* As test_and_clear_bit, but using __ATOMIC_SEQ_CST */
static inline int ukarch_test_and_clr_bit_sync(unsigned int nr,
						volatile void *byte)
{
	__u8 *addr = ((__u8 *)byte) + (nr >> 3);
	__u8 bit = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_and(addr, ~bit, __ATOMIC_SEQ_CST);

	return (orig & bit) != 0;
}

/* As test_and_set_bit, but using __ATOMIC_SEQ_CST */
static inline int ukarch_test_and_set_bit_sync(unsigned int nr,
						volatile void *byte)
{
	__u8 *addr = ((__u8 *)byte) + (nr >> 3);
	__u8 bit = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_or(addr, bit, __ATOMIC_SEQ_CST);

	return (orig & bit) != 0;
}

/* As set_bit, but using __ATOMIC_SEQ_CST */
static inline void ukarch_set_bit_sync(unsigned int nr, volatile void *byte)
{
	ukarch_test_and_set_bit_sync(nr, byte);
}

/* As clear_bit, but using __ATOMIC_SEQ_CST */
static inline void ukarch_clr_bit_sync(unsigned int nr, volatile void *byte)
{
	ukarch_test_and_clr_bit_sync(nr, byte);
}

/* As test_bit, but with a following memory barrier. */
static inline int ukarch_test_bit_sync(unsigned int nr, volatile void *byte)
{
	int result;

	result = ukarch_test_bit(nr, byte);
	barrier();
	return result;
}

#ifdef __cplusplus
}
#endif

#endif /* __UKARCH_ATOMIC_H__ */
