/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_FIXMAP_H__
#define __UK_FIXMAP_H__

#include <uk/arch/types.h>
#include <uk/compiler.h>
#include <uk/config.h>
#include <uk/pt.h>

#include <uk/plat/pal/fixmap.h>

#include <uk/pal/fixmap.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

/** Number of bytes a slot maps */
#define UK_FIXMAP_SLOT_SIZE	UK_PAL_PAGE_Lx_SIZE(UK_PAL_FIXMAP_LEVEL)

/**
 * Defines <var> as the window the platform exports as <name>, i.e.
 * UK_PAL_FIXMAP_<name>. If the platform does not export it, the build fails
 * here with "UK_PAL_FIXMAP_<name> undeclared": the platform cannot map
 * <name> through the fixmap.
 *
 * @param var the name of the variable to define
 * @param name the name of the export, e.g. FDT for UK_PAL_FIXMAP_FDT
 */
#define UK_FIXMAP_IMPORT(var, name)					\
	static const unsigned int var __maybe_unused = UK_PAL_FIXMAP_##name

/**
 * Starts using the fixmap. The platform must have linked the page table
 * that covers the slots into the hierarchy, and the hierarchy must be the
 * active one (see uk_pt_init_active(), uk_pt_activate()).
 *
 * @param pt the page table hierarchy
 *
 * @return 0 on success, -ENOENT if the slots are not covered by a page
 *    table of the hierarchy
 */
int uk_fixmap_init(struct uk_pt *pt);

/**
 * Maps a physical memory range into consecutive free slots of a window
 *
 * @param window the window, i.e. its first slot, see UK_FIXMAP_IMPORT()
 * @param paddr the physical address to map, not necessarily aligned
 * @param len the number of bytes to map
 *
 * @return the virtual address of paddr, or a negative error value cast to
 *    __vaddr_t (see PTRISERR()): -EINVAL if len is 0 or no window starts at
 *    the given slot, -EEXIST if part of the range is already mapped in the
 *    window, -ENOSPC if the window does not have enough consecutive free
 *    slots
 */
__vaddr_t uk_fixmap_map(unsigned int window, __paddr_t paddr, __sz len);

/**
 * Unmaps a range mapped with uk_fixmap_map()
 *
 * @param vaddr the virtual address returned by uk_fixmap_map()
 * @param len the length passed to uk_fixmap_map()
 *
 * @return 0 on success, -EINVAL if the range is not within the slots,
 *    -ENOENT if part of the range is not mapped
 */
int uk_fixmap_unmap(__vaddr_t vaddr, __sz len);

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_FIXMAP_H__ */
