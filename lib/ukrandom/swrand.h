/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RANDOM_SWRAND_H__
#define __UK_RANDOM_SWRAND_H__

#include <uk/random/driver.h>
#include <uk/spinlock.h>

#include "chacha.h"

/* Invalid pointer to differentiate between not initialized
 * libukrandom and initialized without an underlying driver.
 */
#define UK_SWRAND_DRIVER_NONE	0xb0b0cafe

#define UK_SWRAND_CTX_INIT() {		    \
	.driver = __NULL,		    \
	.lock = UK_SPINLOCK_INITIALIZER(),  \
}

#define UK_SWRAND_CTX(name)		    \
	struct uk_swrand_ctx name = UK_SWRAND_CTX_INIT()

struct uk_swrand_ctx {
	struct uk_random_driver *driver;
	struct chacha_ctx chacha;
	struct uk_spinlock lock;
};

/* Initialize the CSPRNG. The CSPRNG is seeded with randomness
 * provided by the dtb's `/chosen/rng-seed` node.
 */
int uk_swrand_fdt_init(void *fdt, struct uk_random_driver **drv);

/* Initialize the CSPRNG. The CSPRNG is seeded with randomness
 * provided by the kernel's cmdline.
 */
int uk_swrand_cmdline_init(struct uk_random_driver **drv);

/* Initialize the software CSPRNG using a seed provided
 * by the driver of the RNG device.
 */
int uk_swrand_init(struct uk_random_driver **drv);

/* Get a 32-bit random number */
int __check_result uk_swrand_randr(__u32 *val);

#endif /* __UK_RANDOM_SWRAND_H__ */
