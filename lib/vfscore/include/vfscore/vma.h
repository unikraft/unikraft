/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __VFSCORE_VMA_H__
#define __VFSCORE_VMA_H__

#include <uk/vma_types.h>

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

#endif /* __VFSCORE_VMA_H__ */
