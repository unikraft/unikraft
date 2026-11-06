/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef PLAT_XEN_INCLUDE_XEN_ARM_SHM_H_
#define PLAT_XEN_INCLUDE_XEN_ARM_SHM_H_

#include <stdbool.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/arch/types.h>

#ifdef CONFIG_XEN_STATIC_SHM

/**
 * Maximum length of the "xen,shm-id" string property from the device tree,
 * including trailing '\0'.
 * (From Xen's docs/misc/arm/device-tree/booting.txt)
 */
#define UK_SHM_ID_MAXLEN 16

/**
 * Map the shm regions declared in the device tree (read-only).
 * Must be called once the page tables are managed by the platform
 */
void uk_plat_xen_shm_init(void);

/**
 * Get a pointer to (a sub-range of) an shm region with specified permission.
 *
 * @param id
 *   The shared memory region identifier (xen,shm-id in the device tree)
 * @param len
 *   Size in bytes to map (must be page-aligned)
 * @param readonly
 *   True if the region should be mapped read-only
 * @param offset
 *   Byte offset from the start of the shm region (must be page-aligned)
 * @param mem
 *   Virtual address at (region_base + offset), or NULL on error
 * @return
 *   0 on success
 *   -EINVAL if the requested mapping is invalid (not aligned, NULL id)
 *   -ENOENT if the requested memory region cannot be found
 *   -EIO if the mapping could not be configured
 */
int uk_plat_xen_shm_get(const char *id, __sz len, bool readonly,
			__off offset, void **mem);

#endif /* CONFIG_XEN_STATIC_SHM */

#endif /* PLAT_XEN_INCLUDE_XEN_ARM_SHM_H_ */
