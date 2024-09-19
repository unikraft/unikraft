/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RANDOM_SWRAND_H__
#define __UK_RANDOM_SWRAND_H__

#include <uk/random/driver.h>

/* Initialize the software CSPRNG */
int uk_swrand_init(struct uk_random_driver **drv);

/* Get a 32-bit random number */
__u32 uk_swrand_randr(void);

#endif /* __UK_RANDOM_SWRAND_H__ */
