/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#include <string.h>
#include <uk/swrand.h>
#include <uk/ctors.h>
#include <uk/config.h>
#include <uk/print.h>

__u32 uk_swrandr_gen_seed32(void)
{
	__u32 val;

#ifdef CONFIG_LIBUKSWRAND_INITIALSEED_TIME
	val = (__u32)ukplat_wall_clock();
#endif

#ifdef CONFIG_LIBUKSWRAND_INITIALSEED_RDRAND
	asm volatile ("rdrand %%eax;"
		: "=a" (val));
#endif

#ifdef CONFIG_LIBUKSWRAND_INITIALSEED_USECONSTANT
	val = CONFIG_LIBUKSWRAND_INITIALSEED_CONSTANT;
#endif

	return val;
}

ssize_t uk_swrand_fill_buffer(void *buf, size_t buflen)
{
	size_t step, chunk_size, i;
	__u32 rd;

	step = sizeof(__u32);
	chunk_size = buflen % step;

	for (i = 0; i < buflen - chunk_size; i += step)
		*(__u32 *)((char *) buf + i) = uk_swrand_randr();

	/* fill the remaining bytes of the buffer */
	if (chunk_size > 0) {
		rd = uk_swrand_randr();
		memcpy(buf + i, &rd, chunk_size);
	}

	return buflen;
}

static void _uk_swrand_ctor(void)
{
	unsigned int i;
#ifdef CONFIG_LIBUKSWRAND_CHACHA
	unsigned int seedc = 10;
	__u32 seedv[10];
#else
	unsigned int seedc = 2;
	__u32 seedv[2];
#endif
	uk_pr_info("Initialize random number generator...\n");

	for (i = 0; i < seedc; i++)
		seedv[i] = uk_swrandr_gen_seed32();

	uk_swrand_init_r(&uk_swrand_def, seedc, seedv);
}

UK_CTOR_PRIO(_uk_swrand_ctor, UK_SWRAND_CTOR_PRIO);
