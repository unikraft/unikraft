/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

SECTIONS {
	. = ALIGN(8);
	PROVIDE(uk_boot_earlytab_start = .);
	.uk_boot_earlytab : {
		KEEP(*(SORT_BY_NAME(.uk_boot_earlytab[0-9])));
	}
	PROVIDE(uk_boot_earlytab_end = .);
}
INSERT AFTER .rodata
