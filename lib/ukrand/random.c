/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/random.h>
#include <uk/assert.h>
#include <string.h>

#if CONFIG_LIBUKRAND_HW
#define UK_RANDOM_RNG(valp, bits)			\
	({						\
		ukarch_random_u##bits((valp));		\
	})
#define UK_RANDOM_RNG_SEED(valp, bits)			\
	({						\
		ukarch_random_seed_u##bits((valp));	\
	})
#elif CONFIG_LIBUKRAND_CHACHA
#define UK_RANDOM_RNG(valp, bits)			\
	({						\
		*(valp) = uk_chacha_randr_u##bits();	\
		0;					\
	})
#define UK_RANDOM_RNG_SEED(valp, bits) UK_RANDOM_RNG(valp, bits)
#else /* !CONFIG_LIBUKRAND_HW && !CONFIG_LIBUKRAND_CHACHA */
#error "Invalid RNG source"
#endif /* !CONFIG_LIBUKRAND_HW && !CONFIG_LIBUKRAND_CHACHA */

static int uk_hwrand_init(struct uk_init_ctx *ictx __unused)
{
	return ukarch_random_init();
}
uk_early_initcall(uk_hwrand_init, 0x0);

int uk_random_u32(__u32 *val)
{
	UK_ASSERT(val);
	return UK_RANDOM_RNG(val, 32);
}

int uk_random_u64(__u64 *val)
{
	UK_ASSERT(val);
	return UK_RANDOM_RNG(val, 64);
}

int uk_random_seed_u32(__u32 *val)
{
	UK_ASSERT(val);
	return UK_RANDOM_RNG_SEED(val, 32);
}

int uk_random_seed_u64(__u64 *val)
{
	UK_ASSERT(val);
	return UK_RANDOM_RNG_SEED(val, 64);
}

__ssz uk_random_fill_buffer(void *buf, __sz buflen)
{
	UK_ASSERT(buf);
	UK_ASSERT(buflen);

	__sz step, chunk_size, i;
	__u64 rd;
	int ret;

	step = sizeof(__u64);
	chunk_size = buflen % step;

	for (i = 0; i < buflen - chunk_size; i += step) {
		ret = uk_random_u64((__u64 *)((char *)buf + i));
		if (unlikely(ret))
			return ret;
	}

	/* fill the remaining bytes of the buffer */
	if (chunk_size > 0) {
		ret = uk_random_u64(&rd);
		if (unlikely(ret))
			return ret;
		memcpy((char *)buf + i, &rd, chunk_size);
	}

	/* Number of bytes copied to buf.
	 * This is always buflen since we rely on HW randomness
	 */
	return buflen;
}
