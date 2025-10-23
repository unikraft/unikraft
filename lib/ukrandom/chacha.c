/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/essentials.h>

#include "chacha.h"

/* This implements the original version of ChaCha20 as presented
 * in http://cr.yp.to/chacha/chacha-20080128.pdf. For the differences
 * with the IRTF version see RFC-8439 Sect. 2.3.
 */

/* Block constant: "expand 32-byte k" */
static const __u32 sigma[4] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};

static inline __u32 rotl32(__u32 v, int c)
{
	return (v << c) | (v >> (32 - c));
}

void chacha_quarterround(__u32 x[16], int a, int b, int c, int d)
{
	x[a] = x[a] + x[b];
	x[d] = rotl32(x[d] ^ x[a], 16);

	x[c] = x[c] + x[d];
	x[b] = rotl32(x[b] ^ x[c], 12);

	x[a] = x[a] + x[b];
	x[d] = rotl32(x[d] ^ x[a], 8);

	x[c] = x[c] + x[d];
	x[b] = rotl32(x[b] ^ x[c], 7);
}

void chacha_wordtobyte(__u32 output[16], const __u32 input[16])
{
	__u32 i;

	for (i = 0; i < 16; i++)
		output[i] = input[i];

	for (i = 20; i > 0; i -= 2) {
		chacha_quarterround(output, 0, 4, 8, 12);
		chacha_quarterround(output, 1, 5, 9, 13);
		chacha_quarterround(output, 2, 6, 10, 14);
		chacha_quarterround(output, 3, 7, 11, 15);
		chacha_quarterround(output, 0, 5, 10, 15);
		chacha_quarterround(output, 1, 6, 11, 12);
		chacha_quarterround(output, 2, 7, 8, 13);
		chacha_quarterround(output, 3, 4, 9, 14);
	}

	for (i = 0; i < 16; i++)
		output[i] += input[i];
}

void chacha_init(struct chacha_ctx *ctx, const __u32 key[8],
		 const __u32 iv[2], __u64 counter)
{
	int i;

	for (i = 0; i < 4; i++)
		ctx->input[i] = sigma[i];

	for (i = 0; i < 8; i++)
		ctx->input[i + 4] = key[i];

	ctx->input[12] = (__u32)counter;
	ctx->input[13] = (__u32)(counter >> 32);

	ctx->input[14] = iv[0];
	ctx->input[15] = iv[1];

	ctx->k = 16;
}

__u32 chacha_rand32(struct chacha_ctx *ctx)
{
	__u32 res;

	if (ctx->k < 16) {
		res = ctx->output[ctx->k];
		ctx->k++;
		return res;
	}

	chacha_wordtobyte(ctx->output, ctx->input);
	ctx->input[12]++;
	if (ctx->input[12] == 0)
		ctx->input[13]++;

	ctx->k = 1;
	return ctx->output[0];
}
