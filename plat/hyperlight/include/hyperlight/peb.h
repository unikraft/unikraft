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
 * passes its address in RDI (x0 on arm64).  This kernel reads nothing
 * from it: the initrd and the command line come over host calls
 * (GetInitrdBase, GetInitrdSize, GetCmdLine), and host calls go over the
 * virtqueue transport (transport.h).  The struct records the layout of
 * hyperlight-common's HyperlightPEB (mem.rs); since the kernel reads none
 * of the PEB, a host that passes a longer one boots it too.
 */

#ifndef __HYPERLIGHT_PEB_H__
#define __HYPERLIGHT_PEB_H__

#include <uk/essentials.h>

struct hyperlight_mem_region {
	__u64 size;
	__u64 ptr;
} __packed;

/*
 * The PEB is 16 bytes: the init data region.
 *
 * init_data:    the host's init data blob; hluk, its host, passes none
 */
struct hyperlight_peb {
	struct hyperlight_mem_region init_data;
} __packed;

/* hyperlight-common's HyperlightPEB (mem.rs) is 16 bytes. */
UK_CTASSERT(sizeof(struct hyperlight_peb) == 16);

#endif /* __HYPERLIGHT_PEB_H__ */
