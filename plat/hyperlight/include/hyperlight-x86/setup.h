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
 * Get the hostfs mount path advertised by the Hyperlight host via the
 * HLHSMNT TLV in init_data, or NULL when the host didn't specify one
 * (in which case callers should fall back to their kconfig default).
 */
const char *hyperlight_hostfs_mountpoint_from_host(void);

#endif /* __HYPERLIGHT_X86_SETUP_H__ */
