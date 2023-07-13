/* SPDX-License-Identifier: BSD-2-Clause */
/*-
 * This code is based on:
 * https://github.com/freebsd/freebsd/blob/master/sys/arm64/arm64/locore.S
 *
 * Authors: Andrew Turner
 *          Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2012-2014 Andrew Turner. All rights reserved.
 * Copyright (c) 2018 Arm Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */
#include <uk/arch/lcpu.h>
#include <uk/arch/limits.h>
#include <uk/asm.h>
#include <uk/reloc.h>

ENTRY(start_mmu)
	/*
	 * Using dsb here to guarantee the create_pagetables has
	 * been done.
	 */
	dsb sy

	/* Load ttbr0, pagetable starts from _end */
	ur_ldr  x27, arm64_bpt_l3_pt0
	msr ttbr0_el1, x27
	isb

	/* Clear the Monitor Debug System control register */
	msr mdscr_el1, xzr

	/* Invalidate the TLB to avoid stale one */
	tlbi vmalle1
	dsb nsh

	ldr x2, =MAIR_INIT_ATTR
	msr mair_el1, x2

#ifdef CONFIG_PAGING
	/* Set up TCR_EL1. The platform must provide a
	 * configuration compatible with the paging API.
	 */
	ldr x2, =TCR_INIT_FLAGS
	msr tcr_el1, x2
#else /* CONFIG_PAGING */
	/* Get VIRT_BITS from id_aa64mmfr0_el1.PARange */
	mrs x3, id_aa64mmfr0_el1
	ur_ldr  x5, tcr_ips_bits
	ubfx x4, x3, #0, #4
	ldrb w4, [x5, x4]
	cmp x4, #52
	b.lt setup_tcr_el1
	/*
	 * We currently do not provide 5-level page tables used in
	 * implementing 52-bit virtual addresing with 4K granularity, so we
	 * enforce the max VA to 48-bit for PARange >= 52.
	 */
	mov x4, #48
setup_tcr_el1:
	/* Setup TCR_EL1_TxSZ(64 - VIRT_BITS) for TCR_INIT_FLAGS */
	mov x5, #64
	sub x5, x5, x4
	mov x4, x5
	lsl x5, x5, #TCR_EL1_T1SZ_SHIFT
	orr x5, x4, x5
	ldr x2, =TCR_INIT_FLAGS
	orr x2, x5, x2
	bfi x2, x3, #32, #3
	msr tcr_el1, x2
#endif /* CONFIG_PAGING */

	/* save lr */
	mov x22, x30

	/* Setup SCTLR */
	ldr x2, =SCTLR_SET_BITS
	ldr x3, =SCTLR_CLEAR_BITS
	mrs x1, sctlr_el1
	bic x1, x1, x3	/* Clear the required bits */
	orr x1, x1, x2	/* Set the required bits */
	msr sctlr_el1, x1
	isb

	/* restore lr */
	mov x30, x22

	ret
END(start_mmu)

