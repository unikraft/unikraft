/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RANDOM_CHACHA_H__
#define __UK_RANDOM_CHACHA_H__

#include <uk/essentials.h>

#define CHACHA20_SEED_WORDS	8 /* 256-bit */
#define CHACHA20_IV_WORDS	2 /* 16-bit */

#define CHACHA_COUNTER_MAX  __U64_MAX

struct chacha_ctx {
	int k;
	__u32 input[16], output[16];
};

static inline __u64 chacha_counter(struct chacha_ctx *ctx)
{
	return ((((__u64)ctx->input[13]) << 32) | ctx->input[12]);
}

/**
 * INTERNAL Initialize ChaCha
 */
void chacha_init(struct chacha_ctx *ctx,
		 const __u32 key[CHACHA20_SEED_WORDS],
		 const __u32 iv[CHACHA20_IV_WORDS],
		 __u64 counter);

/**
 * INTERNAL Generate a 32-bit random number
 */
__u32 chacha_rand32(struct chacha_ctx *ctx);

/**
 * INTERNAL Execute ChaCha quarter-round.
 *          Exported for unit-tests.
 */
void chacha_quarterround(__u32 x[16], int a, int b, int c, int d);

/**
 * INTERNAL Generate next block.
 *          Exported for unit-tests.
 */
void chacha_wordtobyte(__u32 output[16], const __u32 input[16]);

#endif /* __UK_RANDOM_CHACHA_H__ */
