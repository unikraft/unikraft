/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Hyperlight Paging Configuration
 *
 * Hyperlight provides identity-mapped memory (vaddr == paddr).
 * We override the direct-map region to be identity-mapped as well,
 * since we cannot set up the high-address direct map at 0xffffff8000000000.
 */

#ifndef __HYPERLIGHT_PAGING_H__
#define __HYPERLIGHT_PAGING_H__

/*
 * For Hyperlight, we use identity mapping (vaddr == paddr) as our "direct map"
 * This means DIRECTMAP_AREA_START = 0, so:
 *   x86_directmap_paddr_to_vaddr(paddr) = paddr + 0 = paddr
 *   x86_directmap_vaddr_to_paddr(vaddr) = vaddr - 0 = vaddr
 */
#define HYPERLIGHT_DIRECTMAP_IDENTITY   1

#endif /* __HYPERLIGHT_PAGING_H__ */
