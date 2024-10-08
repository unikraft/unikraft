/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_MEMTAG_H__
#define __UK_MEMTAG_H__

#include <uk/asm/memtag.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_MEMTAG_GRANULE	UK_ARCH_MEMTAG_GRANULE /* bytes */
#define UK_MEMTAG_ALIGN(sz)	ALIGN_UP(sz, UK_MEMTAG_GRANULE)

#define uk_memtag_init		uk_arch_memtag_init
#define uk_memtag_tag_region	uk_arch_memtag_tag_region

/**
 * Check if an asynchronous tag check fault has ocurred
 *
 * @return non-zero if a tag fault has ocurred
 */
#if (CONFIG_LIBUKMEMTAG_TCF_ASYNC || CONFIG_LIBUKMEMTAG_TCF_ASYMMETRIC)
#define uk_memtag_async_fault()		uk_arch_memtag_async_fault()
#else /* !(CONFIG_LIBUKMEMTAG_TCF_ASYNC || CONFIG_LIBUKMEMTAG_TCF_ASYMMETRIC) */
#define uk_memtag_async_fault()		(0)
#endif /* !(CONFIG_LIBUKMEMTAG_TCF_ASYNC || CONFIG_LIBUKMEMTAG_TCF_ASYMMETRIC) */

/**
 * Initialize memory tagging
 *
 * @return zero on success, negative value on error
 */
int uk_memtag_init(void);

/**
 * Tags a contiguous region
 *
 * @param ptr  pointer to region
 * @param size region size. Must be aligned to MEMTAG_GRANULE
 *
 * @return tagged pointer
 */
void *uk_memtag_tag_region(void *ptr, __sz size);

#ifdef __cplusplus
}
#endif

#endif /* __UK_MEMTAG_H__ */
