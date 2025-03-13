/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Utilities for anonymous volatile PoD */

#ifndef __UK_POD_ANON_H__
#define __UK_POD_ANON_H__

#include <uk/pod/types.h>

/**
 * Page-in function for anonymous memory; fills pages with zeros.
 */
__ssz uk_pod_anon_pagein(void *addr, __sz npages, __paddr_t pa,
			 void *arg, __sz pgoff);

/**
 * Writeback function for anonymous memory; no-op.
 */
__ssz uk_pod_anon_writeback(const void *addr, __sz npages,
			    void *arg, __sz pgoff);

/**
 * Pageout function for anonymous memory; no-op.
 */
void uk_pod_anon_pageout(const void *addr, __sz npages, __paddr_t pa,
			 void *arg, __sz pgoff);

/**
 * Page I/O ops for anonymous memory. Consists of above functions.
 */
extern const struct uk_pod_pgio uk_pod_anon_ops;

#endif /* __UK_POD_ANON_H__ */
