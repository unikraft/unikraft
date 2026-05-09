/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

int _ukplat_mem_mappings_init(void)
{
	/* Hyperlight sets up paging before guest entry */
	return 0;
}

