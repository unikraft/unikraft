/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __XEN_ARM_SHM_H__
#define __XEN_ARM_SHM_H__

#include <uk/essentials.h>
#include <uk/arch/types.h>

#define UK_SHM_RW 0x0
#define UK_SHM_RO 0x1

#ifdef CONFIG_XEN_STATIC_SHM
#define UK_SHM_MAX_ENTRIES CONFIG_XEN_SHM_MAX_ENTRIES
#define UK_SHM_ID_MAXLEN 32

struct uk_shm_entry {
	char id[UK_SHM_ID_MAXLEN];
	__vaddr_t vaddr;
	__sz size;
};

/* shm entries */
extern struct uk_shm_entry shm_entries[UK_SHM_MAX_ENTRIES];
extern int shm_count;

/**
 * Map a sub-range of an shm region with specified permission.
 *
 * @param id
 *   The shm id (xen,id) derived from device tree node
 * @param len
 *   Size in bytes to map (must be page-aligned)
 * @param perm
 *   UK_SHM_RW or UK_SHM_RO
 * @param offset
 *   Byte offset from the start of the shm region (must be page-aligned)
 * @return
 *   Virtual address at (region_base + offset), or NULL on error
 */
void *uk_shm_map(const char *id, __sz len, __u16 perm, __off offset);

#else /* !CONFIG_XEN_STATIC_SHM */

static inline void *uk_shm_map(const char *id __unused, __sz len __unused,
			       __u16 perm __unused, __off offset __unused)
{
	return __NULL;
}

#endif /* CONFIG_XEN_STATIC_SHM */

#endif /* __XEN_ARM_SHM_H__ */
