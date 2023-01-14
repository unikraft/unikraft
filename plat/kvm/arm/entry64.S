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
 */
#include <uk/arch/lcpu.h>
#include <uk/arch/limits.h>
#include <uk/asm.h>
#include <uk/plat/common/sections.h>
#include <uk/config.h>

/*
 * The registers used by _libkvmplat_start:
 * x0 - FDT pointer
 */

.text
ENTRY(_libkvmplat_entry)

	ldr x25, =_dtb
	ldr x26, =_end

#ifdef CONFIG_FPSIMD
	/* Enable fp/simd support */
	ldr        x0, =(3 << 20)
	msr        cpacr_el1, x0
	isb
#endif /* CONFIG_FPSIMD */

	/*
	 * We will disable MMU and cache before the pagetables are ready.
	 * This means we will change memory with cache disabled, so we need to
	 * invalidate the cache to ensure there is no stale data in it.
	 * But we don't know the size of the RAM either. And it would be
	 * expensive to invalidate the whole cache. In this case, just
	 * just need to invalidate what we are going to use:
	 * DTB, TEXT, DATA, BSS, and bootstack.
	 */
	ldr x0, = _start_ram_addr;
	add x27, x26, #__STACK_SIZE
	sub x1, x27, x25
	bl clean_and_invalidate_dcache_range

	/* Disable the MMU and D-Cache. */
	dsb sy
	mrs x2, sctlr_el1
	mov x3, #SCTLR_EL1_M_BIT|SCTLR_EL1_C_BIT
	bic x2, x2, x3
	msr sctlr_el1, x2
	isb

	/* Set the boot stack */
	mov sp, x27

	/* Set the context id */
	msr contextidr_el1, xzr

	/* Setup exception vector table address before enable MMU */
	ldr x29, =vector_table
	msr VBAR_EL1, x29

	/* Enable the mmu */
	bl start_mmu

	/* Load dtb address to x0 as a parameter */
	ldr x0, =_dtb
	b _libkvmplat_start
END(_libkvmplat_entry)

