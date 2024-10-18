/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/print/store.h>
#include <uk/store.h>

extern unsigned int uk_print_console_lvl;

static int uk_print_get_console_lvl(void *cookie __unused, __u8 *lvl)
{
	*lvl = uk_print_console_lvl;
	return 0;
}

static int uk_print_set_console_lvl(void *cookie __unused, __u8 lvl)
{
	uk_print_console_lvl = lvl;
	return 0;
}

UK_STORE_STATIC_ENTRY(UK_PRINT_KLVL_ENTRY_ID,
		      console_lvl, u8,
		      uk_print_get_console_lvl,
		      uk_print_set_console_lvl);
