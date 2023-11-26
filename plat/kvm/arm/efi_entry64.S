/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/config.h>
#include <uk/asm.h>

.section .text
ENTRY(uk_efi_entry64)
	stp	x0, x1, [sp, #16]

	mov	x0, xzr
	mov	x1, xzr
	bl	do_uk_reloc
	ldp	x0, x1, [sp, #16]

	bl	uk_efi_main

uk_efi_fail:
	wfi
	b	uk_efi_fail
END(uk_efi_entry64)
