/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_X86_PEB_H__
#define __HYPERLIGHT_X86_PEB_H__

#include <uk/arch/types.h>

/**
 * A memory region in the guest address space.
 * Maps to hyperlight_common::mem::GuestMemoryRegion
 */
struct hyperlight_mem_region {
	__u64 size;     /* The size of the memory region */
	__u64 ptr;      /* The address of the memory region */
};

/**
 * Hyperlight Process Environment Block (PEB).
 * Maps to hyperlight_common::mem::HyperlightPEB (v0.13.1)
 *
 * This structure is passed to the guest at entry and contains all
 * the information needed to communicate with the host and access
 * memory regions.
 */
struct hyperlight_peb {
	struct hyperlight_mem_region input_stack;
	struct hyperlight_mem_region output_stack;
	struct hyperlight_mem_region init_data;
	struct hyperlight_mem_region guest_heap;
	struct hyperlight_mem_region file_mappings;
};

#endif /* __HYPERLIGHT_X86_PEB_H__ */
