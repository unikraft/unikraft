/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#include <uk/arch/limits.h>
#include <uk/asm.h>
#include <kvm-arm/mm.h>
#include <arm/cpu_defs.h>

/*
 * The registers used by _libkvmplat_start:
 * x0 - FDT pointer
 */

.text
ENTRY(_libkvmplat_entry)

	/*
	 * Clean the boot stack and page table. As _end, PAGE_TABLE_SIZE and
	 * BOOT_STACK_SIZE are page_size alignment, the boot stack can be
	 * 64-bytes alignment too. Execute 4 stp consecutively without boundary
	 * check would be safe here.
	 */
	ldr x26, =_end
	add x27, x26, #PAGE_TABLE_SIZE
	add x27, x27, #__STACK_SIZE
1:
	stp xzr, xzr, [x26], #16
	stp xzr, xzr, [x26], #16
	stp xzr, xzr, [x26], #16
	stp xzr, xzr, [x26], #16
	cmp x26, x27
	b.lo 1b

	mov sp, x27

	/*
	 * Disable the MMU. We may have entered the kernel with it on and
	 * will need to update the tables later. If this has been set up
	 * with anything other than a VA == PA map then this will fail,
	 * but in this case the code to find where we are running from
	 * would have also failed.
	 */
	dsb sy
	mrs x2, sctlr_el1
	bic x2, x2, #SCTLR_M
	msr sctlr_el1, x2
	isb

	/* Set the context id */
	msr contextidr_el1, xzr

	/* Create a pagetable to do PA == VA mapping */
	bl create_pagetables

	/* Setup exception vector table address before enable MMU */
	ldr x29, =vector_table
	msr VBAR_EL1, x29

	/* Enable the mmu */
	bl start_mmu

	/* Load dtb address to x0 as a parameter */
	ldr x0, =_dtb
	b _libkvmplat_start
END(_libkvmplat_entry)

ENTRY(_libkvmplat_newstack)
	/* Setup new stack */
	mov sp, x0

	/* Setup parameter for _libkvmplat_entry2 */
	mov x0, x2

	/* Branch to _libkvmplat_entry2 */
	br x1
END(_libkvmplat_newstack)
