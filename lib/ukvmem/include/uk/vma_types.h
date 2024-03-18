/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_VM_TYPES_H__
#define __UK_VM_TYPES_H__

#include <uk/vmem.h>
#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/arch/paging.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Address Reservations --------------------------------------------------------
 *
 * A reserved VMA only serves as address reservation in the address space so
 * that the address range covered by the VMA cannot be allocated to other
 * areas. No actual memory is consumed and there are no mappings in the page
 * table created. Accessing a reserved address range leads to a non-resolvable
 * page fault.
 *
 * Since a reserved VMA does not modify the page table it can also be used to
 * represent a mapping already existing in the page table (e.g., kernel
 * code and data segments). In this case, memory accesses behave in whatever
 * way dictated by the page table.
 *
 * To use a previously reserved memory range perform a mapping that specifies
 * the exact virtual address and the UK_VMA_MAP_REPLACE flag.
 */
extern const struct uk_vma_ops uk_vma_rsvd_ops;

/**
 * Reserves a range in the address space. See uk_vma_map() for a description of
 * the parameters.
 */
static inline int uk_vma_reserve(struct uk_vas *vas, __vaddr_t *vaddr, __sz len)
{
	return uk_vma_map(vas, vaddr, len, 0, 0, __NULL,
			  &uk_vma_rsvd_ops, __NULL);
}

/**
 * Reserves a range in the address space to reflect a mapping already existing
 * in the page table or to replace an existing mapping (thereby unmapping it).
 * If the reservation should represent an existing mapping inthe page table,
 * the supplied attributes and flags must match the actual page table
 * configuation. Otherwise, behavior is undefined. See uk_vma_map() for a
 * description of the parameters.
 */
static inline int uk_vma_reserve_ex(struct uk_vas *vas, __vaddr_t *vaddr,
				    __sz len, unsigned long attr,
				    unsigned long flags, const char *name)
{
	UK_ASSERT(flags == 0 || flags & UK_VMA_MAP_REPLACE);

	return uk_vma_map(vas, vaddr, len, attr, flags, name,
			  &uk_vma_rsvd_ops, __NULL);
}

/**
 * Anonymous Memory ------------------------------------------------------------
 *
 * Anonymous memory is purely backed by RAM and has no specific semantic
 * attached to it. The memory is usually zero-initialized but it can be left
 * uninitialized for performance reasons (see UK_VMA_MAP_UNINITIALIZED). All
 * contents is lost when the memory is unmapped. Demand paging is supported.
 */
extern const struct uk_vma_ops uk_vma_anon_ops;

/**
 * Creates a new anonymous memory mapping. See uk_vma_map() for a description
 * of the parameters.
 */
static inline int uk_vma_map_anon(struct uk_vas *vas, __vaddr_t *vaddr,
				  __sz len, unsigned long attr,
				  unsigned long flags, const char *name)
{
	return uk_vma_map(vas, vaddr, len, attr, flags, name,
			  &uk_vma_anon_ops, __NULL);
}

/**
 * Stack -----------------------------------------------------------------------
 *
 * A stack VMA can be used as thread stack that   ┌────────────┐    Stack Base
 * automatically grows as needed. The stack       │/  /  /  /  │ │
 * behaves like regular anonymous memory with a   │  /STACK/  /│ │
 * guard page at the end to detect simple stack   │ /  /  /  / │ │
 * overflows. Furthermore, it cannot be split     ├────────────┤ ▼  Stack Top
 * into multiple VMAs or merged with neighboring  │            │
 * ones. The stack has an initial size and        │            │
 * grows up to the full size of the VMA. Due to   │  RESERVED  │
 * the guard page the maximum actual stack size   │            │
 * is one page less than the VMA size. Memory is  │            │
 * only consumed for the allocated parts. Stack   ├────────────┤
 * memory is not freed after having been          │ GUARD PAGE │
 * allocated on the first touch.                  └────────────┘
 *
 * Stack VMAs are never merged or split to keep the guard page intact.
 */
extern const struct uk_vma_ops uk_vma_stack_ops;

/* Stack flags */
#define UK_VMA_STACK_GROWS_UP		(0x1UL << UK_VMA_MAP_EXTF_SHIFT)

#define STACK_TOP_GUARD_SIZE				\
	(CONFIG_LIBUKVMEM_STACK_GUARD_PAGES_TOP * PAGE_SIZE)

#define STACK_BOTTOM_GUARD_SIZE				\
	(CONFIG_LIBUKVMEM_STACK_GUARD_PAGES_BOTTOM * PAGE_SIZE)

#define STACK_GUARDS_SIZE	(STACK_TOP_GUARD_SIZE + STACK_BOTTOM_GUARD_SIZE)

/**
 * Creates a new stack VMA. See uk_vma_map() for a description of the
 * parameters not listed here.
 *
 * NOTE: len does not include the total size of the guards.
 *
 * @param initial_len
 *   Number of bytes pre-allocated for the stack.
 */
static inline int uk_vma_map_stack(struct uk_vas *vas, __vaddr_t *vaddr,
				   __sz len, unsigned long flags,
				   const char *name, __sz initial_len)
{
	__vaddr_t va;
	int rc;

	UK_ASSERT(PAGE_ALIGNED(initial_len));
	UK_ASSERT(UK_VMA_MAP_SIZE_TO_ORDER(flags) == 0);

	flags |= UK_VMA_MAP_SIZE(PAGE_SHIFT);

	/* Just populate the whole stack */
	if (initial_len >= len)
		flags |= UK_VMA_MAP_POPULATE;

	rc = uk_vma_map(vas, vaddr, len + STACK_GUARDS_SIZE,
			PAGE_ATTR_PROT_RW, flags, name,
			&uk_vma_stack_ops, __NULL);
	if (rc || initial_len == 0 || (flags & UK_VMA_MAP_POPULATE))
		return rc;

	UK_ASSERT(PAGE_ALIGNED(len));

	va = *vaddr;
	if (!(flags & UK_VMA_STACK_GROWS_UP))
		va += STACK_BOTTOM_GUARD_SIZE + len - initial_len;
	else
		va += STACK_BOTTOM_GUARD_SIZE;

	return uk_vma_advise(vas, va, initial_len, UK_VMA_ADV_WILLNEED, 0);
}

/**
 * Direct-Mapped Physical Memory -----------------------------------------------
 *
 * A direct memory access (DMA) VMA can be used to map a particular physical
 * address range to virtual memory, for instance, to map device memory or
 * manually allocated physical memory. Multiple mappings of the same physical
 * address range can be created. Unmapping a DMA VMA will not release the
 * physical memory. Note that if device memory is addressed which needs special
 * mapping attributes (e.g., write through caching) these have to be
 * explicitly provided during mapping. The same applies to the page size.
 * Demand paging is supported, but UK_VMA_MAP_POPULATE prevents (minor) page
 * faults on first access.
 */
extern const struct uk_vma_ops uk_vma_dma_ops;

struct uk_vma_dma {
	struct uk_vma base;

	/** Physical base address of the mapped physical region */
	__paddr_t paddr;
};

struct uk_vma_dma_args {
	__paddr_t paddr;
};

/**
 * Creates a new direct-mapped physical memory mapping. See uk_vma_map() for a
 * description of the parameters not listed here.
 *
 * @param paddr
 *   The base physical address of the physical address range which
 *   should be mapped in the new virtual memory area. Must be aligned to the
 *   page size.
 */
static inline int uk_vma_map_dma(struct uk_vas *vas, __vaddr_t *vaddr,
				 __sz len, unsigned long attr,
				 unsigned long flags, const char *name,
				 __paddr_t paddr)
{
	struct uk_vma_dma_args args = {
		.paddr = paddr,
	};

	UK_ASSERT(PAGE_ALIGNED(paddr));
	UK_ASSERT(!(flags & UK_VMA_MAP_UNINITIALIZED));

	return uk_vma_map(vas, vaddr, len, attr, flags, name,
			  &uk_vma_dma_ops, &args);
}

#ifdef CONFIG_LIBVFSCORE
#include <vfscore/file.h>

/**
 * File Mapping VMA ------------------------------------------------------------
 *
 * A file mapping can be used to map the contents of a file into memory. File
 * mappings use the file name as VMA name. The file will be kept open for the
 * lifetime of the mapping.
 *
 * Note that the current file mapping implementation only allows private
 * mappings and will return -ENOTSUP for shared mappings. Accordingly,
 * modifications are not synched back to the file. Similarly, changes to the
 * file via regular read() and write() operations are not visible in the
 * mapping. Instead, the whole file contents is loaded into memory when the
 * mapping is established.
 */
extern const struct uk_vma_ops uk_vma_file_ops;

/* File mapping flags */
#define UK_VMA_FILE_SHARED		(0x1UL << UK_VMA_MAP_EXTF_SHIFT)

struct uk_vma_file {
	struct uk_vma base;

	/** File mapped in this VMA */
	struct vfscore_file *f;

	/** Start offset describing what position in the file is mapped */
	__off offset;
};

struct uk_vma_file_args {
	int fd;
	__off offset;
};

/**
 * Creates a new file mapping. See uk_vma_map() for a description of the
 * parameters not listed here.
 *
 * @param fd
 *   File descriptor of the file to map into memory. The file descriptor must
 *   have been opened with sufficient permissions to allow for all operations
 *   permitted by the VMA's attributes.
 * @param offset
 *   Offset within the file to map in the VMA. Must be aligned to the page size.
 */
static inline int uk_vma_map_file(struct uk_vas *vas, __vaddr_t *vaddr,
				  __sz len, unsigned long attr,
				  unsigned long flags, int fd, __off offset)
{
	struct uk_vma_file_args args = {
		.fd = fd,
		.offset = offset,
	};

	UK_ASSERT(fd >= 0);
	UK_ASSERT(offset >= 0);
	UK_ASSERT(PAGE_ALIGNED(offset));

	return uk_vma_map(vas, vaddr, len, attr, flags, __NULL,
			  &uk_vma_file_ops, &args);
}
#endif /* CONFIG_LIBVFSCORE */

#ifdef __cplusplus
}
#endif

#endif /* __UK_VM_TYPES_H__ */
