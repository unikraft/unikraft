/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <string.h>

#include <uk/assert.h>
#include <uk/libparam.h>
#include <uk/print.h>
#include <uk/random.h>
#include <uk/random/driver.h>
#include <uk/spinlock.h>

#if CONFIG_LIBUKRANDOM_DTB_SEED
#include <uk/ofw/fdt.h>
#endif /* CONFIG_LIBUKRANDOM_DTB_SEED */

#include "chacha.h"
#include "swrand.h"

/* Unlike encryption, the nonce value is not important on RNG
 * as the 64-bit counter can generate up to 2**64 blocks, i.e.
 * 1ZiB of random data under the same (key, nonce) pair.
 */
static __u32 iv[CHACHA20_IV_WORDS] = {0};

static UK_SWRAND_CTX(ctx);

#if CONFIG_LIBUKRANDOM_CMDLINE_SEED
#define CHACHA_SEED_NOINIT						\
	{0xdeadb0b0, 0xdeadb0b0, 0xdeadb0b0, 0xdeadb0b0,		\
	 0xdeadb0b0, 0xdeadb0b0, 0xdeadb0b0, 0xdeadb0b0}

static __u32 seedv_cmdl[CHACHA20_SEED_WORDS] = CHACHA_SEED_NOINIT;

UK_LIBPARAM_PARAM_ARR_ALIAS(seed, seedv_cmdl, __u32, CHACHA20_SEED_WORDS,
			    "ChaCha20 256-bit key");
#endif /* CONFIG_LIBUKRANDOM_CMDLINE_SEED */

#if CONFIG_LIBUKRANDOM_DTB_SEED
int uk_swrand_fdt_init(void *fdt, struct uk_random_driver **drv)
{
	__u32 *seedv;
	__sz seedc;
	int rc;

	rc = fdt_chosen_rng_seed(fdt, &seedv, &seedc);
	if (unlikely(rc)) {
		uk_pr_debug("Seed not set in the dtb\n");
		return -ENOTSUP;
	}

	if (unlikely(seedc < CHACHA20_SEED_WORDS)) {
		uk_pr_err("The dtb does not provide enough randomness\n");
		return -ENOTSUP;
	}

	uk_pr_warn("Passing the seed via the dtb is potentially insecure\n");
	uk_pr_info("Initializing the random number generator...\n");

	/* prevent drivers from registering */
	*drv = (void *)UK_SWRAND_DRIVER_NONE;

	ctx.driver = *drv;

	chacha_init(&ctx.chacha, seedv, iv, 0);

	return rc;
}
#endif /* CONFIG_LIBUKRANDOM_DTB_SEED */

#if CONFIG_LIBUKRANDOM_CMDLINE_SEED
int uk_swrand_cmdline_init(struct uk_random_driver **drv)
{
	__u32 seedv[CHACHA20_SEED_WORDS] = CHACHA_SEED_NOINIT;

	/* FIXME This could theoretically (but extremely rarely)
	 *       cause a false positive if the loader passes
	 *       CHACHA_SEED_NOINIT, yet libukparam does not provide
	 *       a way to tell whether a param has been set other
	 *       than checking against its value.
	 */
	if (!memcmp(seedv, seedv_cmdl, sizeof(seedv))) {
		uk_pr_debug("Seed not set in the cmdline\n");
		return -ENOTSUP;
	}

	uk_pr_warn("Passing the seed via the cmdline is potentially insecure\n");
	uk_pr_info("Initializing the random number generator...\n");

	/* prevent drivers from registering */
	*drv = (void *)UK_SWRAND_DRIVER_NONE;

	ctx.driver = *drv;

	chacha_init(&ctx.chacha, seedv, iv, 0);

	return 0;
}
#endif /* CONFIG_LIBUKRANDOM_CMDLINE_SEED */

int uk_swrand_init(struct uk_random_driver **drv)
{
	__u32 seedv[CHACHA20_SEED_WORDS];
	unsigned int i __maybe_unused;
	int ret;

	uk_pr_info("Initializing the random number generator...\n");

	if (!*drv) {
		uk_pr_err("Could not initialize: No entropy source available\n");
		return -ENODEV;
	}

	ret = (*drv)->ops->seed_bytes_fb((__u8 *)seedv, sizeof(seedv));
	if (unlikely(ret)) {
		uk_pr_err("Could not initialize: Failed to collect entropy (%d)\n",
			  ret);
		return ret;
	}

	ctx.driver = *drv;

	chacha_init(&ctx.chacha, seedv, iv, 0);

	return 0;
}

int __check_result uk_swrand_randr(__u32 *val)
{
	if (unlikely(!ctx.driver))
		return -ENODEV;

	uk_spin_lock(&ctx.lock);

	*val = chacha_rand32(&ctx.chacha);

	uk_spin_unlock(&ctx.lock);

	return 0;
}
