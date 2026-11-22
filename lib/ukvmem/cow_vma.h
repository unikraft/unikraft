/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UKVMEM_COW_VMA_H__
#define __UKVMEM_COW_VMA_H__

struct uk_vas;

/* The VMA list and every retirement batch have already been drained. */
void vmem_cow_vas_destroy(struct uk_vas *vas);

#endif /* __UKVMEM_COW_VMA_H__ */
