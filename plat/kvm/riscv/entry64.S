/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/arch/limits.h>
#include <uk/asm.h>
#include <uk/plat/common/sections.h>
#include <uk/config.h>

.section .bss
.space 4096
bootstack:
.balign __STACK_ALIGN_SIZE

.section .data.boot
#include "pagetable64.S"

#define SATP_MODE_SV39 8
#define SATP_MODE_SHIFT 60

.section .text
ENTRY(_libkvmplat_entry)
	/*
	 * As per the RISC-V SBI spec, execution starts in supervisor mode,
	 * with each hart (hardware thread) having its hartid placed in
	 * the a0 register. Choose the hart with the id 0 to manage the
	 * booting process and suspend the others for the moment.
	 */
	bnez a0, 2f

	/* Enable paging */
	li t0, SATP_MODE_SV39
	slli t0, t0, SATP_MODE_SHIFT
	la t1, riscv64_pt_l2_0
	srli t1, t1, PAGE_SHIFT
	or t1, t0, t1
	csrw satp, t1
	sfence.vma x0, x0

	/* Setup the temporary bootstack */
	la sp, bootstack

	/* The a1 register holds the address of the DTB  */
	call _libkvmplat_start
2:
	wfi
	j 2b

END(_libkvmplat_entry)

ENTRY(_libkvmplat_newstack)
	/* Switch to the new stack */
	mv sp, a0

	/* Load argument for the next function */
	mv a0, a2

	/* Jump to the next function */
	jr a1
END(_libkvmplat_newstack)
