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

/*
 * The registers used by _libkvmplat_start:
 * x0 - FDT pointer
 */

.text
ENTRY(_libkvmplat_entry)
	/* Boot stack is placed after pagetable area temporarily */
	ldr x26, =_end
	add x26, x26, #PAGE_TABLE_SIZE
	add x27, x26, #__STACK_SIZE

	/*
	 * Clean the boot stack. As _end, PAGE_TABLE_SIZE and BOOT_STACK_SIZE
	 * are page_size alignment, the boot stack can be 64-bytes alignment
	 * too. Execute 4 stp consecutively without boundary check would be
	 * safe here.
	 */
1:
	stp xzr, xzr, [x26], #16
	stp xzr, xzr, [x26], #16
	stp xzr, xzr, [x26], #16
	stp xzr, xzr, [x26], #16
	cmp x26, x27
	b.lo 1b

	mov sp, x27

	/* Load dtb address to x0 as a parameter */
	ldr x0, =_dtb
	b _libkvmplat_start
END(_libkvmplat_entry)
