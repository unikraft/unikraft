/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * This source code is based on:
 * https://github.com/freebsd/freebsd/blob/9b718f1d0125/sys/arm64/arm64/
 * cpufunc_asm.S
 *
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2014 Robin Randhawa
 * Copyright (c) 2015 The FreeBSD Foundation
 * All rights reserved.
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
#include <uk/asm.h>
#include <arm/cpu_defs.h>

/*
 * Clean and invalidate data cache. This takes the start address in x0,
 * length in x1. It will corrupt x0 ~ x5.
 */
ENTRY(clean_and_invalidate_dcache_range)
	/* Get information about the caches from CTR_EL0 */
	mrs	x4, ctr_el0
	mov	x2, #CTR_BYTES_PER_WORD

	/* Get minimum D cache line size */
	ubfx	x3, x4, #CTR_DMINLINE_SHIFT, #CTR_DMINLINE_WIDTH
	lsl	x3, x2, x3

	/* Align the start address to line size */
	sub	x4, x3, #1
	and	x2, x0, x4
	add	x1, x1, x2
	bic	x0, x0, x4
1:
	/* clean and invalidate D cache by D cache line size */
	dc	civac, x0
	dsb	nsh

	/* Move to next line and reduce the size */
	add	x0, x0, x3
	subs	x1, x1, x3

	/* Check if all range has been invalidated */
	b.hi	1b

	isb

	ret
END(clean_and_invalidate_dcache_range)
