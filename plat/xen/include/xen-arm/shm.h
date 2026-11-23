/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_ARM_SHM_H__
#define __UK_PLAT_XEN_ARM_SHM_H__

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/arch/types.h>

#if CONFIG_XEN_STATIC_SHM

/**
 * Maximum length of the "xen,shm-id" string property from the device tree,
 * including trailing '\0'.
 * (From Xen's docs/misc/arm/device-tree/booting.txt)
 */
#define UK_PLAT_XEN_SHM_ID_MAXLEN 16

/**
 * Initialize the shared memory regions during boot.
 * Read shared memory regions defined in the device-tree, and map the
 * corresponding physical memory ranges. Memory is mapped without any
 * access permission, which must be set with `uk_plat_xen_shm_configure()`
 */
void uk_plat_xen_shm_init(void);

/**
 * Configure a shared memory region for usage
 *
 * @param [in] id
 *   The identifier ('xen-id') of the shared memory region
 * @param [in] writeable
 *   If true, configure the memory read+write, otherwise read-only
 * @return
 *   0 on success
 *   -EINVAL if id is NULL
 *   -ENOENT if no region with that id is found
 *   -EIO if setting page attributes fails
 *   -EPERM if shared memory is sealed, and cannot be configured anymore
 */
int uk_plat_xen_shm_configure(const char *id, __bool writeable);

/**
 * Get a pointer to a static shared memory region
 *
 * @param [in] id
 *   The identifier ('xen-id') of the shared memory region
 * @param [out] mem
 *   If non-null, points to the shared memory virtual address when
 *   the call succeeds
 * @param [out] size
 *   If non-null, set to the shared memory region size (in bytes)
 *   when the call succeeds
 * @return
 *   0 on success
 *   -EINVAL if id is NULL
 *   -ENOENT if no region with that id is found
 *   -EBADSLT if the region has not been configured yet
 */
int uk_plat_xen_shm_get(const char *id, void **mem, __sz *size);

#endif /* CONFIG_XEN_STATIC_SHM */

#endif /* __UK_PLAT_XEN_ARM_SHM_H__ */
