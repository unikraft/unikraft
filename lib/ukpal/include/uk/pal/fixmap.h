/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_FIXMAP_H__
#define __UK_PAL_FIXMAP_H__

#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_HAVE_FIXMAP

/**
 * UK_PAL_FIXMAP_LEVEL definition
 *
 * Page table level of the fixmap slots. Each slot maps a single page of
 * UK_PAL_PAGE_Lx_SIZE(UK_PAL_FIXMAP_LEVEL) bytes.
 */
#ifndef UK_PAL_FIXMAP_LEVEL
#error "UK_PAL_FIXMAP_LEVEL not defined"
#endif

/**
 * UK_PAL_FIXMAP_BASE definition
 *
 * Virtual address of slot 0. Slot n is at
 * UK_PAL_FIXMAP_BASE + n * UK_PAL_PAGE_Lx_SIZE(UK_PAL_FIXMAP_LEVEL).
 */
#ifndef UK_PAL_FIXMAP_BASE
#error "UK_PAL_FIXMAP_BASE not defined"
#endif

/**
 * UK_PAL_FIXMAP_NR_SLOTS definition
 *
 * Number of slots
 */
#ifndef UK_PAL_FIXMAP_NR_SLOTS
#error "UK_PAL_FIXMAP_NR_SLOTS not defined"
#endif

#if !__ASSEMBLY__

/**
 * Returns the page attributes a slot is mapped with
 *
 * @param slot the slot [0..UK_PAL_FIXMAP_NR_SLOTS - 1]
 *
 * @return the attributes (UK_PAL_PAGE_ATTR_* and memory type)
 */
unsigned long uk_pal_fixmap_slot_attr(unsigned int slot);

/**
 * Returns the number of slots of the window that starts at a slot. A window
 * is a group of consecutive slots that serves the same purpose, e.g. the
 * instances of a device.
 *
 * @param slot the first slot of the window
 *
 * @return the number of slots in the window, 0 if no window starts at slot
 */
unsigned int uk_pal_fixmap_window_slots(unsigned int slot);

/*
 * A platform exports a window to other components by defining
 * UK_PAL_FIXMAP_<NAME> to the first slot of the window.
 */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_HAVE_FIXMAP */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PAL_FIXMAP_H__ */
