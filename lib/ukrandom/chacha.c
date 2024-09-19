/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Vlad-Andrei Badoiu <vlad_andrei.badoiu@stud.acs.upb.ro>
 *
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
 */

#include <string.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/ctors.h>
#include <uk/random.h>
#include <uk/random/driver.h>
#include <uk/plat/lcpu.h>

/*
 * ChaCha20 requires eight 32-bit integers for the key and two 32-bit integers
 * for the nonce, hence the seed length is 10.
 * RFC7539 specifies three 32-bit integers for the nonce, but the reference
 * implementation uses only two:
 * http://cr.yp.to/streamciphers/timings/estreambench/submissions/salsa20/chacha8/ref/chacha.c
 *
 * TODO: bring the nonce-size in-line with the RFC.
 */
#define CHACHA_SEED_LENGTH		10
#define CHACHA_SEED_INSECURE		0xdeadb0b0

struct uk_swrand {
	int k;
	__u32 input[16], output[16];
};

struct uk_swrand uk_swrand_def;

/* This value isn't important, as long as it's sufficiently asymmetric */
static const char sigma[16] = "expand 32-byte k";

static inline __u32 uk_rotl32(__u32 v, int c)
{
	return (v << c) | (v >> (32 - c));
}

static inline void uk_quarterround(__u32 x[16], int a, int b, int c, int d)
{
	x[a] = x[a] + x[b];
	x[d] = uk_rotl32(x[d] ^ x[a], 16);

	x[c] = x[c] + x[d];
	x[b] = uk_rotl32(x[b] ^ x[c], 12);

	x[a] = x[a] + x[b];
	x[d] = uk_rotl32(x[d] ^ x[a], 8);

	x[c] = x[c] + x[d];
	x[b] = uk_rotl32(x[b] ^ x[c], 7);
}

static inline void
uk_salsa20_wordtobyte(__u32 output[16], const __u32 input[16])
{
	__u32 i;

	for (i = 0; i < 16; i++)
		output[i] = input[i];

	for (i = 8; i > 0; i -= 2) {
		uk_quarterround(output, 0, 4, 8, 12);
		uk_quarterround(output, 1, 5, 9, 13);
		uk_quarterround(output, 2, 6, 10, 14);
		uk_quarterround(output, 3, 7, 11, 15);
		uk_quarterround(output, 0, 5, 10, 15);
		uk_quarterround(output, 1, 6, 11, 12);
		uk_quarterround(output, 2, 7, 8, 13);
		uk_quarterround(output, 3, 4, 9, 14);
	}

	for (i = 0; i < 16; i++)
		output[i] += input[i];
}

static inline void uk_key_setup(struct uk_swrand *r, __u32 k[8])
{
	int i;

	for (i = 0; i < 8; i++)
		r->input[i + 4] = k[i];

	for (i = 0; i < 4; i++)
		r->input[i] = ((__u32 *)sigma)[i];
}

static inline void uk_iv_setup(struct uk_swrand *r, __u32 iv[2])
{
	r->input[12] = 0;
	r->input[13] = 0;
	r->input[14] = iv[0];
	r->input[15] = iv[1];
}

static inline __u32 infvec_val(unsigned int c, const __u32 v[],
			       unsigned int pos)
{
	if (c == 0)
		return 0x0;
	return v[pos % c];
}

static void chacha_init(struct uk_swrand *r, unsigned int seedc,
			const __u32 seedv[])
{
	UK_ASSERT(r);
	/* Initialize chacha */
	__u32 k[8], iv[2], i;

	for (i = 0; i < 8; i++)
		k[i] = infvec_val(CHACHA_SEED_LENGTH, seedv, i);

	iv[0] = infvec_val(seedc, seedv, i);
	iv[1] = infvec_val(seedc, seedv, i + 1);

	uk_key_setup(r, k);
	uk_iv_setup(r, iv);

	r->k = 16;
}

int uk_swrand_init(struct uk_random_driver **drv)
{
	unsigned int seedc = CHACHA_SEED_LENGTH;
	__u32 seedv[CHACHA_SEED_LENGTH];
	unsigned int i;
	int ret;

	UK_ASSERT(drv && *drv);

	uk_pr_info("Initialize random number generator...\n");
#if CONFIG_LIBUKRANDOM_SEED_INSECURE
	uk_pr_err("*******************************************\n");
	uk_pr_err("* This configuration uses an insecure RNG *\n");
	uk_pr_err("*         DO NOT USE IN PRODUCTION        *\n");
	uk_pr_err("*******************************************\n");
#endif /* CONFIG_LIBUKRANDOM_SEED_INSECURE */

#if CONFIG_LIBUKRANDOM_SEED_INSECURE
	for (i = 0; i < seedc; i++)
		seedv[i] = CHACHA_SEED_INSECURE;
#else /* !CONFIG_LIBUKRANDOM_SEED_INSECURE */
	ret = (*drv)->ops->seed_bytes_fb((__u8 *)seedv, seedc);
	if (unlikely(ret)) {
		uk_pr_err("Could not generate random seed\n");
		return ret;
	}
#endif /* !CONFIG_LIBUKRANDOM_SEED_INSECURE */

	uk_pr_info("Entropy source: %s\n", (*drv)->name);

	chacha_init(&uk_swrand_def, seedc, seedv);

	return 0;
}

__u32 uk_swrand_randr_r(struct uk_swrand *r)
{
	__u32 res;

	for (;;) {
		uk_salsa20_wordtobyte(r->output, r->input);
		r->input[12] = r->input[12] + 1;
		if (r->input[12] == 0)
			r->input[13]++;

		if (r->k < 16) {
			res = r->output[r->k];
			r->k += 1;
			return res;
		}

		r->k = 0;
	}
}

/* Uses the pre-initialized default generator  */
/* TODO: Revisit with multi-CPU support */
__u32 uk_swrand_randr(void)
{
	unsigned long iflags;
	__u32 ret;

	UK_ASSERT(!ukplat_lcpu_irqs_disabled());

	iflags = ukplat_lcpu_save_irqf();
	ret = uk_swrand_randr_r(&uk_swrand_def);
	ukplat_lcpu_restore_irqf(iflags);

	return ret;
}
