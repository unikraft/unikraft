/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* File mapping VMA. */

#ifndef __UK_POSIX_FILE_VMA_H__
#define __UK_POSIX_FILE_VMA_H__

#include <uk/posix-fd.h>
#include <uk/vmem/vma_types.h>

extern const struct uk_vma_ops uk_fdio_vma_ops;
#if CONFIG_LIBUKVMEM_COW_VMA
extern const struct uk_vma_ops uk_fdio_cow_vma_ops;
extern const struct uk_vma_ops uk_fdio_shared_vma_ops;
#endif

struct uk_fdio_vma_args {
	struct uk_ofile *of;
	__off offset;
};

/**
 * Creates a new file mapping. See uk_vma_map() for a description of the
 * parameters not listed here.
 *
 * @param of
 *   Open file description of the file to map into memory. The file must have
 *   been opened with sufficient permissions to allow for all operations
 *   permitted by the VMA's attributes.
 * @param offset
 *   Offset within the file to map in the VMA. Must be aligned to the page size.
 */
static inline
int uk_fdio_map_file(struct uk_vas *vas, __vaddr_t *vaddr,
		     __sz len, unsigned long attr, unsigned long flags,
		     struct uk_ofile *of, __off offset)
{
	struct uk_fdio_vma_args args = {
		.of = of,
		.offset = offset,
	};
	const struct uk_vma_ops *ops = &uk_fdio_vma_ops;

#if CONFIG_LIBUKVMEM_COW_VMA
	if (flags & UK_VMA_FILE_SHARED)
		ops = &uk_fdio_shared_vma_ops;
	else if (vas->cow)
		ops = &uk_fdio_cow_vma_ops;
#endif

	return uk_vma_map(vas, vaddr, len, attr, flags, __NULL,
			  ops, &args);
}

#endif /* __UK_POSIX_FILE_VMA_H__ */
