/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RANDOM_H__
#define __UK_RANDOM_H__

#include <uk/init.h>
#include <uk/arch/random.h>
#include <uk/plat/lcpu.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKRAND_CHACHA

__u32 uk_chacha_randr_u32(void);
__u64 uk_chacha_randr_u64(void);
__ssz uk_chacha_fill_buffer(void *buf, __sz buflen);

#endif /* CONFIG_LIBUKRAND_CHACHA */

#ifdef __cplusplus
}
#endif

#endif /* __UK_RANDOM_H__ */
