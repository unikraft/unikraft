/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef _PT_H_
#define _PT_H_

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/plat/pal/page.h>
#include <uk/plat/pal/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Takes over the page table hierarchy that the platform booted with
 *
 * @param pt_pbase the physical address of the top-level page table
 * @param frame_off the offset at which a page table frame of the hierarchy
 *    can be accessed, that is vaddr = paddr + frame_off
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int xenplat_pt_init(__paddr_t pt_pbase, __vaddr_t frame_off);

/**
 * Puts a page table that the platform reserved into the hierarchy. All of
 * its entries are invalidated first.
 *
 * @param vaddr a virtual address the page table is to translate
 * @param level the level of the page table that holds the entry, so one
 *    above the level of the page table being linked
 * @param tbl the page table, page aligned
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int xenplat_pt_link(__vaddr_t vaddr, unsigned int level, void *tbl);

/**
 * Maps a range of pages. The page tables on the way must already be in the
 * hierarchy, see xenplat_pt_link(). An existing mapping in the range is
 * replaced in place, without the translation ever becoming invalid.
 *
 * @param vaddr the virtual address to map at
 * @param paddr the physical address to map
 * @param pages the number of pages to map
 * @param level the level that defines the page size [0..UK_PAL_PT_LEVELS - 2]
 * @param attr the attributes of the new mappings
 *
 * @return 0 on success, -ENOENT if a page table is missing, a non-zero
 *    error value otherwise
 */
int xenplat_pt_map(__vaddr_t vaddr, __paddr_t paddr, unsigned long pages,
		   unsigned int level, unsigned long attr);

/**
 * Changes the attributes of a range of pages
 *
 * @param vaddr the virtual address of the first page to change
 * @param pages the number of pages to change
 * @param level the level that defines the page size [0..UK_PAL_PT_LEVELS - 2]
 * @param attr the new attributes
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int xenplat_pt_set_attr(__vaddr_t vaddr, unsigned long pages,
			unsigned int level, unsigned long attr);

#ifdef __cplusplus
}
#endif
#endif /* _PT_H_ */
