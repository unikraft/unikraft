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

#include <uk/bitops.h>

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
		retval += uk_hweight_long(addr[i]);

	if (tail) {
		const unsigned long mask = UK_BITMAP_LAST_WORD_MASK(tail);

		retval += uk_hweight_long(addr[end] & mask);
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

#endif					/* _LINUX_BITMAP_H_ */
