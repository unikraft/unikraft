/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ARCH_MEMTAG_H__
#define __UK_ARCH_MEMTAG_H__

#include <uk/asm/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MTE_TAG_GRANULE		16
#define MTE_TAG_MASK		(0xFULL << 56)
#define UK_ARCH_MEMTAG_GRANULE	MTE_TAG_GRANULE

#define uk_arch_memtag_async_fault()	SYSREG_READ(TFSR_EL1)

int uk_arch_memtag_init(void);

void *uk_arch_memtag_tag_region(void *ptr, __sz size);

#ifdef __cplusplus
}
#endif

#endif /* __UK_ARCH_MEMTAG_H__ */
