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

#include <errno.h>
#include <uk/arch/lcpu.h>

#define RDRAND_RETRY_LIMIT	10

static inline int ukarch_random_init(void)
{
	__u32 eax, ebx, ecx, edx;

	ukarch_x86_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (unlikely(!(ecx & X86_CPUID1_ECX_RDRAND)))
		return -ENOTSUP;

	ukarch_x86_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
	if (unlikely(!(ebx & X86_CPUID7_EBX_RDSEED)))
		return -ENOTSUP;

	return 0;
}

static inline int ukarch_random_u64(__u64 *val)
{
	unsigned int i;
	unsigned char res = 0;

	for (i = 0; i < RDRAND_RETRY_LIMIT; i++) {
		__asm__ __volatile__(
			"	rdrand	%0\n" /* get rand */
			"	setc	%1\n" /* get result */
			: "=r" (*val), "=qm" (res)
		);
		if (likely(res))
			return 0;
	}
	return -EIO;
}

static inline int ukarch_random_u32(__u32 *val)
{
	unsigned int i;
	unsigned char res = 0;

	for (i = 0; i < RDRAND_RETRY_LIMIT; i++) {
		__asm__ __volatile__(
			"	rdrand	%0\n" /* get rand */
			"	setc	%1\n" /* get result */
			: "=r" (*val), "=qm" (res)
		);
		if (likely(res))
			return 0;
	}
	return -EIO;
}

static inline int ukarch_random_seed_u64(__u64 *val)
{
	unsigned char res;

	__asm__ __volatile__(
		"	rdseed	%0\n" /* get seed */
		"	setc	%1\n" /* get result */
		: "=r" (*val), "=qm" (res)
	);

	if (likely(res))
		return 0;

	return -EIO;
}

static inline int ukarch_random_seed_u32(__u32 *val)
{
	unsigned char res;

	__asm__ __volatile__(
		"	rdseed	%0\n" /* get seed */
		"	setc	%1\n" /* get result */
		: "=r" (*val), "=qm" (res)
	);

	if (likely(res))
		return 0;

	return -EIO;
}
