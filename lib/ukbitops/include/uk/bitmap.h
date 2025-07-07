/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2013-2017 Mellanox Technologies, Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _LINUX_BITMAP_H_
#define	_LINUX_BITMAP_H_

#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <uk/bitops.h>
#include <uk/bitcount.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	UK_BITMAP_FIRST_WORD_MASK(start)  (~0UL << ((start) % UK_BITS_PER_LONG))
#define	UK_BITMAP_LAST_WORD_MASK(n)       (~0UL >> (UK_BITS_PER_LONG - (n)))

#define	UK_BITS_TO_LONGS(n)               howmany((n), UK_BITS_PER_LONG)

#define	UK_BIT_MASK(nr) \
	(1UL << ((nr) & (UK_BITS_PER_LONG - 1)))
#define UK_BIT_WORD(nr)                   ((nr) / UK_BITS_PER_LONG)
#define	UK_GENMASK(h, l) \
	(((~0UL) >> (UK_BITS_PER_LONG - (h) - 1)) & ((~0UL) << (l)))
#define	UK_GENMASK_ULL(h, l) \
	(((~0ULL) >> (UK_BITS_PER_LONG_LONG - (h) - 1)) & ((~0ULL) << (l)))

static inline unsigned long
uk_find_first_bit(const unsigned long *addr, unsigned long size)
{
	long mask;
	int bit;

	for (bit = 0; size >= UK_BITS_PER_LONG;
		size -= UK_BITS_PER_LONG, bit += UK_BITS_PER_LONG, addr++) {
		if (*addr == 0)
			continue;
		return (bit + uk_lssbl(*addr));
	}
	if (size) {
		mask = (*addr) & UK_BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += uk_lssbl(mask);
		else
			bit += size;
	}
	return (bit);
}

static inline unsigned long
uk_find_first_zero_bit(const unsigned long *addr, unsigned long size)
{
	long mask;
	int bit;

	for (bit = 0; size >= UK_BITS_PER_LONG;
		size -= UK_BITS_PER_LONG, bit += UK_BITS_PER_LONG, addr++) {
		if (~(*addr) == 0)
			continue;
		return (bit + uk_lssbl(~(*addr)));
	}
	if (size) {
		mask = ~(*addr) & UK_BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += uk_lssbl(mask);
		else
			bit += size;
	}
	return (bit);
}

static inline unsigned long
uk_find_last_bit(const unsigned long *addr, unsigned long size)
{
	long mask;
	int offs;
	int bit;
	int pos;

	pos = size / UK_BITS_PER_LONG;
	offs = size % UK_BITS_PER_LONG;
	bit = UK_BITS_PER_LONG * pos;
	addr += pos;
	if (offs) {
		mask = (*addr) & UK_BITMAP_LAST_WORD_MASK(offs);
		if (mask)
			return (bit + uk_mssbl(mask));
	}
	while (pos--) {
		addr--;
		bit -= UK_BITS_PER_LONG;
		if (*addr)
			return (bit + uk_mssbl(*addr));
	}
	return (size);
}

static inline unsigned long
uk_find_next_bit(const unsigned long *addr, unsigned long size,
	unsigned long offset)
{
	long mask;
	int offs;
	int bit;
	int pos;

	if (offset >= size)
		return (size);
	pos = offset / UK_BITS_PER_LONG;
	offs = offset % UK_BITS_PER_LONG;
	bit = UK_BITS_PER_LONG * pos;
	addr += pos;
	if (offs) {
		mask = (*addr) & ~UK_BITMAP_LAST_WORD_MASK(offs);
		if (mask)
			return (bit + uk_lssbl(mask));
		if (size - bit <= UK_BITS_PER_LONG)
			return (size);
		bit += UK_BITS_PER_LONG;
		addr++;
	}
	for (size -= bit; size >= UK_BITS_PER_LONG;
		size -= UK_BITS_PER_LONG, bit += UK_BITS_PER_LONG, addr++) {
		if (*addr == 0)
			continue;
		return (bit + uk_lssbl(*addr));
	}
	if (size) {
		mask = (*addr) & UK_BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += uk_lssbl(mask);
		else
			bit += size;
	}
	return (bit);
}

static inline unsigned long
uk_find_next_zero_bit(const unsigned long *addr, unsigned long size,
	unsigned long offset)
{
	long mask;
	int offs;
	int bit;
	int pos;

	if (offset >= size)
		return (size);
	pos = offset / UK_BITS_PER_LONG;
	offs = offset % UK_BITS_PER_LONG;
	bit = UK_BITS_PER_LONG * pos;
	addr += pos;
	if (offs) {
		mask = ~(*addr) & ~UK_BITMAP_LAST_WORD_MASK(offs);
		if (mask)
			return (bit + uk_lssbl(mask));
		if (size - bit <= UK_BITS_PER_LONG)
			return (size);
		bit += UK_BITS_PER_LONG;
		addr++;
	}
	for (size -= bit; size >= UK_BITS_PER_LONG;
		size -= UK_BITS_PER_LONG, bit += UK_BITS_PER_LONG, addr++) {
		if (~(*addr) == 0)
			continue;
		return (bit + uk_lssbl(~(*addr)));
	}
	if (size) {
		mask = ~(*addr) & UK_BITMAP_LAST_WORD_MASK(size);
		if (mask)
			bit += uk_lssbl(mask);
		else
			bit += size;
	}
	return (bit);
}

/**
 * uk_test_and_clear_bit - Atomically clear a bit and return its old value
 * @param nr Bit to clear
 * @param addr Address to count from
 *
 * Note that nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static inline int
uk_test_and_clear_bit(long nr, volatile unsigned long *addr)
{
	volatile __u8 *ptr = ((__u8 *) addr) + (nr >> 3);
	__u8 mask = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_and(ptr, ~mask, __ATOMIC_SEQ_CST);

	return (orig & mask) != 0;
}

/**
 * __uk_test_and_clear_bit - Clear a bit and return its old value
 * @param nr Bit to clear
 * @param addr Address to count from
 *
 * Note that nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 *
 * This operation is not atomic and can be reordered. If two
 * __uk_test_and_clear_bit are executing in parallel, it could be that
 * only one of them will be successful.
 */
static inline int
__uk_test_and_clear_bit(long nr, volatile unsigned long *addr)
{
	volatile __u8 *ptr = ((__u8 *) addr) + (nr >> 3);
	__u8 mask = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_and(ptr, ~mask, __ATOMIC_RELAXED);

	return (orig & mask) != 0;
}

/**
 * uk_test_and_set_bit - Atomically set a bit and return its old value
 * @param nr Bit to clear
 * @param addr Address to count from
 *
 * Note that nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static inline int
uk_test_and_set_bit(long nr, volatile unsigned long *addr)
{
	volatile __u8 *ptr = ((__u8 *) addr) + (nr >> 3);
	__u8 mask = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_or(ptr, mask, __ATOMIC_SEQ_CST);

	return (orig & mask) != 0;
}

/**
 * __uk_test_and_set_bit - Set a bit and return its old value
 * @param nr Bit to clear
 * @param addr Address to count from
 *
 * Note that nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 *
 * This operation is not atomic and can be reordered. If two
 * __uk_test_and_set_bit are executing in parallel, it could be that
 * only one of them will be successful.
 */
static inline int
__uk_test_and_set_bit(long nr, volatile unsigned long *addr)
{
	volatile __u8 *ptr = ((__u8 *) addr) + (nr >> 3);
	__u8 mask = 1 << (nr & 7);
	__u8 orig;

	orig = __atomic_fetch_or(ptr, mask, __ATOMIC_RELAXED);

	return (orig & mask) != 0;
}

enum {
	REG_OP_ISFREE,
	REG_OP_ALLOC,
	REG_OP_RELEASE,
};

/* uk_set_bit and uk_clear_bit are atomic and protected against
 * reordering (do barriers), while the underscored (__*) versions of
 * them are not (not atomic).
 */
static inline void uk_set_bit(long nr, volatile unsigned long *addr)
{
	uk_test_and_set_bit(nr, addr);
}

static inline void __uk_set_bit(long nr, volatile unsigned long *addr)
{
	__uk_test_and_set_bit(nr, addr);
}

static inline void uk_clear_bit(long nr, volatile unsigned long *addr)
{
	uk_test_and_clear_bit(nr, addr);
}

static inline void __uk_clear_bit(long nr, volatile unsigned long *addr)
{
	__uk_test_and_clear_bit(nr, addr);
}

static inline int uk_test_bit(int nr, const volatile unsigned long *addr)
{
	const volatile __u8 *ptr = (const __u8 *) addr;
	int ret =  ((1 << (nr & 7)) & (ptr[nr >> 3])) != 0;

	return ret;
}

static inline int
__uk_bitopts_reg_op(unsigned long *bitmap, int pos, int order, int reg_op)
{
	int nbits_reg;
	int index;
	int offset;
	int nlongs_reg;
	int nbitsinlong;
	unsigned long mask;
	int i;
	int ret = 0;

	nbits_reg = 1 << order;
	index = pos / UK_BITS_PER_LONG;
	offset = pos - (index * UK_BITS_PER_LONG);
	nlongs_reg = UK_BITS_TO_LONGS(nbits_reg);
	nbitsinlong = MIN(nbits_reg,  UK_BITS_PER_LONG);

	mask = (1UL << (nbitsinlong - 1));
	mask += mask - 1;
	mask <<= offset;

	switch (reg_op) {
	case REG_OP_ISFREE:
		for (i = 0; i < nlongs_reg; i++) {
			if (bitmap[index + i] & mask)
				goto done;
		}
		ret = 1;
		break;

	case REG_OP_ALLOC:
		for (i = 0; i < nlongs_reg; i++)
			bitmap[index + i] |= mask;
		break;

	case REG_OP_RELEASE:
		for (i = 0; i < nlongs_reg; i++)
			bitmap[index + i] &= ~mask;
		break;
	}
done:
	return ret;
}

#define uk_for_each_set_bit(bit, addr, size) \
	for ((bit) = uk_find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = uk_find_next_bit((addr), (size), (bit) + 1))

#define	uk_for_each_clear_bit(bit, addr, size) \
	for ((bit) = uk_find_first_zero_bit((addr), (size));		\
	     (bit) < (size);						\
	     (bit) = uk_find_next_zero_bit((addr), (size), (bit) + 1))

static inline void
uk_bitmap_zero(unsigned long *addr, const unsigned int size)
{
	memset(addr, 0, UK_BITS_TO_LONGS(size) * sizeof(long));
}

static inline void
uk_bitmap_fill(unsigned long *addr, const unsigned int size)
{
	const unsigned int tail = size & (UK_BITS_PER_LONG - 1);

	memset(addr, 0xff, UK_BIT_WORD(size) * sizeof(long));

	if (tail)
		addr[UK_BIT_WORD(size)] = UK_BITMAP_LAST_WORD_MASK(tail);
}

static inline int
uk_bitmap_full(unsigned long *addr, const unsigned int size)
{
	const unsigned int end = UK_BIT_WORD(size);
	const unsigned int tail = size & (UK_BITS_PER_LONG - 1);
	unsigned int i;

	for (i = 0; i != end; i++) {
		if (addr[i] != ~0UL)
			return (0);
	}

	if (tail) {
		const unsigned long mask = UK_BITMAP_LAST_WORD_MASK(tail);

		if ((addr[end] & mask) != mask)
			return (0);
	}
	return (1);
}

static inline int
uk_bitmap_empty(unsigned long *addr, const unsigned int size)
{
	const unsigned int end = UK_BIT_WORD(size);
	const unsigned int tail = size & (UK_BITS_PER_LONG - 1);
	unsigned int i;

	for (i = 0; i != end; i++) {
		if (addr[i] != 0)
			return (0);
	}

	if (tail) {
		const unsigned long mask = UK_BITMAP_LAST_WORD_MASK(tail);

		if ((addr[end] & mask) != 0)
			return (0);
	}
	return (1);
}

static inline void
uk_bitmap_set(unsigned long *map, unsigned int start, int nr)
{
	const unsigned int size = start + nr;
	int bits_to_set = UK_BITS_PER_LONG - (start % UK_BITS_PER_LONG);
	unsigned long mask_to_set = UK_BITMAP_FIRST_WORD_MASK(start);

	map += UK_BIT_WORD(start);

	while (nr - bits_to_set >= 0) {
		*map |= mask_to_set;
		nr -= bits_to_set;
		bits_to_set = UK_BITS_PER_LONG;
		mask_to_set = ~0UL;
		map++;
	}

	if (nr) {
		mask_to_set &= UK_BITMAP_LAST_WORD_MASK(size);
		*map |= mask_to_set;
	}
}

static inline void
uk_bitmap_clear(unsigned long *map, unsigned int start, int nr)
{
	const unsigned int size = start + nr;
	int bits_to_clear = UK_BITS_PER_LONG - (start % UK_BITS_PER_LONG);
	unsigned long mask_to_clear = UK_BITMAP_FIRST_WORD_MASK(start);

	map += UK_BIT_WORD(start);

	while (nr - bits_to_clear >= 0) {
		*map &= ~mask_to_clear;
		nr -= bits_to_clear;
		bits_to_clear = UK_BITS_PER_LONG;
		mask_to_clear = ~0UL;
		map++;
	}

	if (nr) {
		mask_to_clear &= UK_BITMAP_LAST_WORD_MASK(size);
		*map &= ~mask_to_clear;
	}
}

static inline unsigned int
uk_bitmap_find_next_zero_area_off(const unsigned long *map,
	const unsigned int size, unsigned int start,
	unsigned int nr, unsigned int align_mask,
	unsigned int align_offset)
{
	unsigned int index;
	unsigned int end;
	unsigned int i;

retry:
	index = uk_find_next_zero_bit(map, size, start);

	index = (((index + align_offset) + align_mask) & ~align_mask) -
		align_offset;

	end = index + nr;
	if (end > size)
		return (end);

	i = uk_find_next_bit(map, end, index);
	if (i < end) {
		start = i + 1;
		goto retry;
	}
	return (index);
}

static inline unsigned int
uk_bitmap_find_next_zero_area(const unsigned long *map,
	const unsigned int size, unsigned int start,
	unsigned int nr, unsigned int align_mask)
{
	return (uk_bitmap_find_next_zero_area_off(map, size,
		start, nr, align_mask, 0));
}

static inline int
uk_bitmap_find_free_region(unsigned long *bitmap, int bits, int order)
{
	int pos;
	int end;

	for (pos = 0; (end = pos + (1 << order)) <= bits; pos = end) {
		if (!__uk_bitopts_reg_op(bitmap, pos, order, REG_OP_ISFREE))
			continue;
		__uk_bitopts_reg_op(bitmap, pos, order, REG_OP_ALLOC);
		return pos;
	}
	return (-ENOMEM);
}

static inline int
uk_bitmap_allocate_region(unsigned long *bitmap, int pos, int order)
{
	if (!__uk_bitopts_reg_op(bitmap, pos, order, REG_OP_ISFREE))
		return (-EBUSY);
	__uk_bitopts_reg_op(bitmap, pos, order, REG_OP_ALLOC);
	return 0;
}

static inline void
uk_bitmap_release_region(unsigned long *bitmap, int pos, int order)
{
	__uk_bitopts_reg_op(bitmap, pos, order, REG_OP_RELEASE);
}

static inline unsigned int
uk_bitmap_weight(unsigned long *addr, const unsigned int size)
{
	const unsigned int end = UK_BIT_WORD(size);
	const unsigned int tail = size & (UK_BITS_PER_LONG - 1);
	unsigned int retval = 0;
	unsigned int i;

	for (i = 0; i != end; i++)
		retval += uk_bitcountl(addr[i]);

	if (tail) {
		const unsigned long mask = UK_BITMAP_LAST_WORD_MASK(tail);

		retval += uk_bitcountl(addr[end] & mask);
	}
	return (retval);
}

static inline int
uk_bitmap_equal(const unsigned long *pa,
	const unsigned long *pb, unsigned int size)
{
	const unsigned int end = UK_BIT_WORD(size);
	const unsigned int tail = size & (UK_BITS_PER_LONG - 1);
	unsigned int i;

	for (i = 0; i != end; i++) {
		if (pa[i] != pb[i])
			return (0);
	}

	if (tail) {
		const unsigned long mask = UK_BITMAP_LAST_WORD_MASK(tail);

		if ((pa[end] ^ pb[end]) & mask)
			return (0);
	}
	return (1);
}

static inline void
uk_bitmap_complement(unsigned long *dst, const unsigned long *src,
	const unsigned int size)
{
	const unsigned int end = UK_BITS_TO_LONGS(size);
	unsigned int i;

	for (i = 0; i != end; i++)
		dst[i] = ~src[i];
}

static inline void
uk_bitmap_or(unsigned long *dst, const unsigned long *src1,
	const unsigned long *src2, const unsigned int size)
{
	const unsigned int end = UK_BITS_TO_LONGS(size);
	unsigned int i;

	for (i = 0; i != end; i++)
		dst[i] = src1[i] | src2[i];
}

static inline void
uk_bitmap_and(unsigned long *dst, const unsigned long *src1,
	const unsigned long *src2, const unsigned int size)
{
	const unsigned int end = UK_BITS_TO_LONGS(size);
	unsigned int i;

	for (i = 0; i != end; i++)
		dst[i] = src1[i] & src2[i];
}

static inline void
uk_bitmap_xor(unsigned long *dst, const unsigned long *src1,
	const unsigned long *src2, const unsigned int size)
{
	const unsigned int end = UK_BITS_TO_LONGS(size);
	unsigned int i;

	for (i = 0; i != end; i++)
		dst[i] = src1[i] ^ src2[i];
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif					/* _LINUX_BITMAP_H_ */
