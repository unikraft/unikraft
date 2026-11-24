/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_PAGING_H__
#define __UK_PLAT_NATIVE_ARCH_PAGING_H__

#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

/**
 * Initialize this CPU's paging capabilities and architectural prerequisites.
 *
 * Does not activate a page table or publish shared paging geometry.
 * If max_physaddr is non-NULL, writes this CPU's supported physical limit.
 * May be called before TLS and the runtime address space are available.
 */
int uk_plat_native_paging_init_cpu(__paddr_t *max_physaddr);

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_NATIVE_ARCH_PAGING_H__ */
