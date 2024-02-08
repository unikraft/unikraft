/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Michalis Pappas <mpappas@fastmail.fm>.
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
#ifndef __UKARCH_RANDOM_H__
#error Do not include this header directly
#endif

#ifdef CONFIG_ARM64_FEAT_RNG

#include <errno.h>
#include <uk/arch/lcpu.h>

static inline int ukarch_random_init(void)
{
	__u64 reg;

	__asm__ __volatile__("mrs %x0, ID_AA64ISAR0_EL1\n" : "=r"(reg));

	return !((reg >> ID_AA64ISAR0_EL1_RNDR_SHIFT) &
		 ID_AA64ISAR0_EL1_RNDR_MASK);
}

static inline int ukarch_random_u64(__u64 *val)
{
	__u64 res;

	__asm__ __volatile__("	mrs	%x0, RNDR\n" /* Get rand */
			     "	mrs	%x1, NZCV\n" /* Get result */
			     : "=r"(*val), "=r"(res));

	if (unlikely(res != 0))
		return -EIO;

	return res;
}

static inline int ukarch_random_u32(__u32 *val)
{
	__u32 res;
	__u64 val64;

	res = ukarch_random_u64(&val64);
	*val = (__u32)val64;

	return res;
}

static inline int ukarch_random_seed_u64(__u64 *val)
{
	__u64 res;

	__asm__ __volatile__("	mrs	%x0, RNDRRS\n" /* Get rand */
			     "	mrs	%x1, NZCV\n"   /* Get result */
			     : "=r"(*val), "=r"(res));

	if (unlikely(res != 0))
		return -EIO;

	return res;
}

static inline int ukarch_random_seed_u32(__u32 *val)
{
	__u32 res;
	__u64 val64;

	res = ukarch_random_seed_u64(&val64);
	*val = (__u32)val64;

	return res;
}

#endif /* CONFIG_ARM64_FEAT_RNG */
