/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKALLOCSTACK_H__
#define __UKALLOCSTACK_H__

#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_alloc *uk_allocstack_init(struct uk_alloc *a
#if CONFIG_LIBUKVMEM
				     , struct uk_vas __maybe_unused *vas,
				     __sz initial_size
#endif /* CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
				     );

#ifdef __cplusplus
}
#endif

#endif /* __UKALLOCSTACK_H__ */
