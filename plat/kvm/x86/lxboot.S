/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/asm.h>
#include <uk/lcpu.h>

#include <kvm-x86/lxboot.h>

/**
 * Stack and entry function to use during CPU initialization
 */
.section .bss
.space 4096
lxboot_bootstack:

/**
 * 64-bit Linux Boot Protocol entry function
 *
 * RSI holds base address of struct lxboot_params. Flat 4GiB CS and DS segments,
 * with ES and SS set to DS. A20 gate enabled. Protected mode enabled, paging
 * enabled with ident mapping for loaded kernel, zero page (i.e., struct
 * lxboot_params) and command line buffer. Interrupts disabled.
 */
.code64
.section .text.64.boot
ENTRY(_lxboot_entry)
	cmpl	$LXBOOT_HDR_HEADER_MAGIC, LXBOOT_HDR_HEADER_OFFSET(%rsi)
	jne	no_lxboot

#if CONFIG_LIBUKRELOC
	/* We are going to taint the early lcpu_bootstack but we do not care.
	 * Since it's a stack, it does not need to not be tainted anyway.
	 */
	leaq	lxboot_bootstack(%rip), %rsp
	/* Make sure it is aligned */
	andq	$~0xf, %rsp
	pushq	%rsi
	subq	$0x8, %rsp
	xorl	%esi, %esi
	xorl	%edi, %edi
	call	do_uk_reloc
	addq	$0x8, %rsp
	popq	%rsi
	/* We do not restore the previous %rsp. At this time it does not seem to
	 * be needed.
	 */
#endif /* CONFIG_LIBUKRELOC */

	leaq	lxboot_bootstack(%rip), %rax
	movq	%rax, uk_plat_native_sstackp(%rip)

	leaq	lxboot_entry(%rip), %rax
	movq	%rax, uk_plat_native_sentry(%rip)

	movq	%rsi, uk_plat_native_sarg(%rip)

	leaq	x86_bpt_pml4(%rip), %rax
	movq	%rax, %cr3
	jmp	lcpu_start64

no_lxboot:
	cli
1:
	hlt
	jmp	1b
END(_lxboot_entry)
