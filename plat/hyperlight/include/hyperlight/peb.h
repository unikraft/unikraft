/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Process Environment Block (PEB) — the shared memory layout between
 * the Hyperlight host and the guest.
 *
 * The host allocates and populates the PEB before guest entry, then
 * passes its address in RDI.  The guest uses the PEB to locate the init
 * data region (initrd + command line) and the heap.  Host calls go over
 * the virtqueue transport instead (transport.h).
 *
 * Layout must match hyperlight-common mem.rs exactly.
 */

#ifndef __HYPERLIGHT_PEB_H__
#define __HYPERLIGHT_PEB_H__

#include <uk/essentials.h>

struct hyperlight_mem_region {
	__u64 size;
	__u64 ptr;
} __packed;

/*
 * The PEB is 32 bytes: two memory regions describing the guest's
 * shared memory areas.
 *
 * init_data:    initrd image + embedded TLV metadata (cmdline, mounts)
 * guest_heap:   heap memory for the guest allocator
 */
struct hyperlight_peb {
	struct hyperlight_mem_region init_data;
	struct hyperlight_mem_region guest_heap;
} __packed;

#endif /* __HYPERLIGHT_PEB_H__ */
