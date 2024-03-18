/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_FALLOC_STORE_H__
#define __UK_FALLOC_STORE_H__

/* stats object entry IDs */
#define UK_FALLOC_STATS_FREE_MEMORY	0x01
#define UK_FALLOC_STATS_TOTAL_MEMORY	0x02

struct uk_falloc_stats_global {
	__sz free_memory;
	__sz total_memory;
};

extern struct uk_falloc_stats_global uk_falloc_stats_global;

/* Initialize stats for a new uk_falloc instance
 *
 * This function should be called by implementations of the uk_falloc API
 * on every new allocator instance. Calls to this function made before the
 * heap is set up are deferred to uk_init.
 *
 * @param fa uk_falloc instance
 * @return 0 on success or a negative value on error
 */
int uk_falloc_init_stats(struct uk_falloc *fa);

#endif /* __UK_FALLOC_STORE_H__ */
