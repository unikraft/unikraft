/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Default VMA operation handlers */

#ifndef __UK_VMEM_VMA_OPS_H__
#define __UK_VMEM_VMA_OPS_H__

#include <uk/vmem.h>

/* Argument list is empty to accept (and ignore) any number of args.
 *
 * This is deprecated in modern versions of C; consider refactoring ASAP.
 */
/**
 * Always fail, denying the operation.
 */
int uk_vma_op_deny();

/**
 * Unconditionally unmap pages and free their page frames.
 */
int uk_vma_op_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len);

/**
 * Unconditionally set page attributes without checks.
 */
int uk_vma_op_set_attr(struct uk_vma *vma, unsigned long attr);

/**
 * Advise handler for anonymous memory.
 */
int uk_vma_op_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
		     unsigned long advice);

#endif /* __UK_VMEM_VMA_OPS_H__ */
