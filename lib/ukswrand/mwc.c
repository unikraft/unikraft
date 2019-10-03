/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/ctors.h>

/* https://stackoverflow.com/questions/9492581/c-random-number-generation-pure-c-code-no-libraries-or-functions */
#define PHI 0x9e3779b9
#define UK_SWRAND_CTOR_PRIO    1

struct uk_swrand uk_swrand_def;

/*
 * Declare the constructor function to initialize the swrand
 */
static void _uk_swrand_ctor(void);

void uk_swrand_init_r(struct uk_swrand *r, __u32 seed)
{
	__u32 i;

	UK_ASSERT(r);

	r->Q[0] = seed;
	r->Q[1] = seed + PHI;
	r->Q[2] = seed + PHI + PHI;
	for (i = 3; i < 4096; i++)
		r->Q[i] = r->Q[i - 3] ^ r->Q[i - 2] ^ PHI ^ i;

	r->c = 362436;
	r->i = 4095;
}

__u32 uk_swrand_randr_r(struct uk_swrand *r)
{
	__u64 t, a = 18782LL;
	__u32 x, y = 0xfffffffe;
	__u32 i, c;

	UK_ASSERT(r);

	i = r->i;
	c = r->c;

	i = (i + 1) & 4095;
	t = a * r->Q[i] + c;
	c = (t >> 32);
	x = t + c;
	if (x < c) {
		x++;
		c++;
	}

	r->i = i;
	r->c = c;
	return (r->Q[i] = y - x);
}

ssize_t uk_swrand_fill_buffer(void *buf, size_t buflen)
{
	size_t step, chunk_size, i;
	__u32 rd;

	step = sizeof(__u32);
	chunk_size = buflen % step;

	for (i = 0; i < buflen - chunk_size; i += step)
		*((char *) buf + i) = uk_swrand_randr();

	/* fill the remaining bytes of the buffer */
	if (chunk_size > 0) {
		rd = uk_swrand_randr();
		memcpy(buf + i, &rd, chunk_size);
	}

	return buflen;
}

static void _uk_swrand_ctor(void)
{
	uk_pr_info("Initialize random number generator...\n");
	uk_swrand_init_r(&uk_swrand_def, CONFIG_LIBUKSWRAND_INITIALSEED);
}

UK_CTOR_FUNC(UK_SWRAND_CTOR_PRIO, _uk_swrand_ctor);
