/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/config.h>
#include <uk/asm.h>

.section .text
ENTRY(uk_efi_entry64)
	/* EFI gives us a stack misaligned by 8 */
	and	$~0xf, %rsp

	xorq	%rdi, %rdi
	xorq	%rsi, %rsi
	pushq	%rdx
	pushq	%rcx
	call	do_uk_reloc
	popq	%rcx
	popq	%rdx

	call	uk_efi_main

uk_efi_fail:
	hlt
	jmp	uk_efi_fail
END(uk_efi_entry64)
