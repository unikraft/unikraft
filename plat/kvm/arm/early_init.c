/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/plat/common/bootinfo.h>

/* KVM early init
 *
 * Initialize early devices and coalesce mrds.
 * TODO Replace with init-based callback registration.
 */
void ukplat_early_init(void)
{
	struct ukplat_bootinfo *bi;

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Could not obtain bootinfo\n");

	ukplat_memregion_list_coalesce(&bi->mrds);
}
