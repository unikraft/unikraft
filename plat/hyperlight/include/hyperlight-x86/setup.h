/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_X86_SETUP_H__
#define __HYPERLIGHT_X86_SETUP_H__

#include <hyperlight-x86/peb.h>

/**
 * Entry point arguments passed from the Hyperlight host.
 */
struct hyperlight_entry_args {
	__u64 peb_address;      /* RDI: Address of the PEB structure */
	__u64 seed;             /* RSI: Random seed */
	__u64 page_size;        /* RDX: OS page size */
	__u64 max_log_level;    /* RCX: Maximum log level */
};

/**
 * Get the PEB structure.
 * Returns NULL if not initialized.
 */
struct hyperlight_peb *hyperlight_get_peb(void);

/**
 * Get the entry arguments.
 */
const struct hyperlight_entry_args *hyperlight_get_entry_args(void);

/**
 * Number of host-provided preopens advertised by the Hyperlight host
 * via the HLHSMNT TLV. Zero when the host didn't pass any.
 */
unsigned int hyperlight_hostfs_preopen_count(void);

/**
 * Guest mount path of preopen `i`, or NULL if out of range. Callable
 * from lib/hostfs at auto-mount time to iterate over the preopens.
 */
const char *hyperlight_hostfs_preopen(unsigned int i);

/**
 * Host-provided wall time at VM boot, in ns since the Unix epoch.
 * Zero if the host didn't advertise one (old builds). Callers should
 * add the monotonic delta since boot to obtain "now".
 */
__u64 hyperlight_wall_boot_ns_from_host(void);

#endif /* __HYPERLIGHT_X86_SETUP_H__ */
