/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_SETUP_H__
#define __HYPERLIGHT_SETUP_H__

#include <uk/arch/types.h>

/**
 * Entry arguments passed from the Hyperlight host.
 *
 * Hyperlight passes four values in registers at guest entry:
 *   RDI = PEB address   (Process Environment Block)
 *   RSI = random seed
 *   RDX = OS page size
 *   RCX = max log level
 *
 * entry64.S saves these into a .data struct before calling C code.
 */
struct hyperlight_entry_args {
	__u64 peb_address;
	__u64 seed;
	__u64 page_size;
	__u64 max_log_level;
};

#endif /* __HYPERLIGHT_SETUP_H__ */
