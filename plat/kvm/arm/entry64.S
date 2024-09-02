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
#include <uk/reloc.h>

#define BOOTSTACK_SIZE	4096

/* Prefer using in-image stack, to avoid conflicts when unmapping is done by
 * paging_init (it invalidates our stack)
 */
.section .bss
.space BOOTSTACK_SIZE
lcpu_bootstack:

.text
ENTRY(_libkvmplat_entry)

#ifdef CONFIG_KVM_BOOT_PROTO_LXBOOT
	mov x25, x0 /* preserve dtb addr */
#endif

#ifdef CONFIG_FPSIMD
	/* Enable fp/simd support */
	ldr        x0, =(3 << 20)
	msr        cpacr_el1, x0
	isb
#endif /* CONFIG_FPSIMD */

	/* If we boot via the linux boot protocol we expect that the MMU is
	 * disabled and the cache for the region of the image is clean.
	 * If we find that the MMU is enabled, we consider the cache state
	 * unpredicatable. In that case we clean-invalidate the cache for
	 * the entire image and disable the MMU.
	 *
	 * Notice: A clean cache would suffice as long as we invalidate any
	 *         memory we touch while the MMU is disabled. But since PIE
	 *         can write anywhere we also invalidate the entire image.
	 */
	mrs	x19, sctlr_el1
	and	x19, x19, #SCTLR_EL1_M_BIT
#if CONFIG_LIBUKRELOC
	/* If PIE is enabled and the MMU is disabled we proceed to
	 * invalidate the entire image region anyway, as libukreloc
	 * right now does not invalidate the individual lines it touches.
	 */
	cbz	x19, 0f
#else /* !CONFIG_LIBUKRELOC */
	cbz	x19, 1f
#endif /* !CONFIG_LIBUKRELOC */

	ur_ldr	x0, _start_ram_addr
	ur_ldr	x1, _end
	sub	x1, x1, x0
	bl	clean_and_invalidate_dcache_range

	/* Disable the MMU and D-Cache. */
	dsb	sy
	mrs	x2, sctlr_el1
	mov	x3, #SCTLR_EL1_M_BIT|SCTLR_EL1_C_BIT
	bic	x2, x2, x3
	msr	sctlr_el1, x2
	isb
#if CONFIG_LIBUKRELOC
	b	1f
0:
	ur_ldr	x0, _start_ram_addr
	ur_ldr	x1, _end
	sub	x1, x1, x0
	bl	invalidate_dcache_range
#endif /* CONFIG_LIBUKRELOC */
1:
	/* Set the boot stack.Invalidate the corresponding cache lines to
	 * avoid stale cache contents shadowing our changes once the MMU
	 * and D-cache are enabled.
	 */
	ur_ldr	x26, lcpu_bootstack
	and	x26, x26, ~(__STACK_ALIGN_SIZE - 1)
	mov	sp, x26
	mov	x0, x26
	mov	x1, #BOOTSTACK_SIZE
	dmb	sy
	bl	invalidate_dcache_range

	/* Zero-out the BSS */
	ldr x26, =__bss_start
	ldr x27, =_end
2:
	str	xzr, [x26], #8
	cmp	x26, x27
	b.lo	2b

	/* Set the context id */
	msr contextidr_el1, xzr

	mov     x0, xzr
	mov     x1, xzr
	bl      do_uk_reloc

	/* Setup exception vector table address before enable MMU */
	ur_ldr  x29, vector_table
	msr VBAR_EL1, x29

	/* Enable the mmu */
	bl start_mmu

	/* Jump to early init */
#if defined(CONFIG_KVM_BOOT_PROTO_LXBOOT)
	mov x0, x25      /* dtb in x0 */
#elif defined(CONFIG_KVM_BOOT_PROTO_QEMU_VIRT)
	ur_ldr  x0, _dtb /* dtb in x0 */
#endif
	bl	ukplat_bootinfo_fdt_setup
	b	_ukplat_entry
END(_libkvmplat_entry)
