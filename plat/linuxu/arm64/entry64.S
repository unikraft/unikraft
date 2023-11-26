/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>,
 *			Florin Postolache <florin.postolache80@gmail.com>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation,
 *               2022, Florin Postolache, <florin.postolache80@gmail.com>
 *               All rights reserved.
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

/*
 * We do not want to unmap anything.
 */
.section .bss
.align	4
.globl	bpt_unmap_mrd
bpt_unmap_mrd:
.space	64

.section .text

/* For now, although inaccurate, set the
 * _base_addr symbol to _liblinuxuplat_start, since that is where most images
 * happen to begin anyway. We do not use it in linuxu anyway, so we do not care.
 * We only do this so that references to this symbol do not generate build
 * errors, although it is not used.
 */
.globl	_base_addr
.set	_base_addr, _liblinuxuplat_start

.global _liblinuxuplat_start
_liblinuxuplat_start:
	ldr x0, [sp]		// argc to r0
	add x1, sp, #8		// argv to r1
	bl _liblinuxuplat_entry

	// Protection
_liblinuxuplat_err:
	b _liblinuxuplat_err
