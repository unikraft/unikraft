/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Bit operations related to finding the first set bit in a word. */

#ifndef __UK_BITSCAN_H__
#define __UK_BITSCAN_H__

/**
 * Find First Set bit of x.
 *
 * @return
 *   x != 0: 1 plus the index of the least significant 1-bit of x
 *   x == 0: 0
 */
static inline
unsigned int uk_ffs(int x)
{
	return __builtin_ffs(x);
}

/**
 * Find First Set bit of long x.
 *
 * @return
 *   x != 0: 1 plus the index of the least significant 1-bit of x
 *   x == 0: 0
 */
static inline
unsigned int uk_ffsl(long x)
{
	return __builtin_ffsl(x);
}

/**
 * Count Leading Zeros in x != 0; if x is 0, the result is undefined.
 *
 * @return
 *   Number of leading 0-bits in x, starting at the most significant bit.
 */
static inline
unsigned int uk_clz(unsigned int x)
{
	return __builtin_clz(x);
}

/**
 * Count Leading Zeros in long x != 0; if x is 0, the result is undefined.
 *
 * @return
 *   Number of leading 0-bits in x, starting at the most significant bit.
 */
static inline
unsigned int uk_clzl(unsigned long x)
{
	return __builtin_clzl(x);
}

/**
 * Count Trailing Zeros in x != 0; if x is 0, the result is undefined.
 *
 * Doubles as the 0-based index of the least significant set bit of x.
 *
 * @return
 *   Number of trailing 0-bits in x, starting at the least significant bit.
 */
static inline
unsigned int uk_ctz(unsigned int x)
{
	return __builtin_ctz(x);
}

/**
 * Count Trailing Zeros in long x != 0; if x is 0, the result is undefined.
 *
 *  * Doubles as the 0-based index of the least significant set bit of x.
 *
 * @return
 *   Number of trailing 0-bits in x, starting at the least significant bit.
 */
static inline
unsigned int uk_ctzl(unsigned long x)
{
	return __builtin_ctzl(x);
}

/**
 * Find most significant set bit of x != 0; if x is 0, the result is undefined.
 *
 * @return
 *  Index of most significant set bit of x; 0 is least significant bit.
 */
static inline
unsigned int uk_mssb(unsigned int x)
{
	return (sizeof(x) * 8 - 1) - uk_clz(x);
}

/**
 * Find most significant set bit of x != 0; if x is 0, the result is undefined.
 *
 * @return
 *  Index of most significant set bit of x; 0 is least significant bit.
 */
static inline
unsigned int uk_mssbl(unsigned long x)
{
	return (sizeof(x) * 8 - 1) - uk_clzl(x);
}

/**
 * Find least significant set bit of x != 0; if x is 0, the result is undefined.
 *
 * @return
 *  Index of least significant set bit of x; 0 is least significant bit.
 */
static inline
unsigned int uk_lssb(unsigned int x)
{
	return uk_ctz(x);
}

/**
 * Find least significant set bit of x != 0; if x is 0, the result is undefined.
 *
 * @return
 *  Index of least significant set bit of x; 0 is least significant bit.
 */
static inline
unsigned int uk_lssbl(unsigned long x)
{
	return uk_ctzl(x);
}

#endif /* __UK_BITSCAN_H__ */
