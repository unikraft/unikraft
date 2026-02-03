/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Răzvan Vîrtan <virtanrazvan@gmail.com>
 *          Justin He     <justin.he@arm.com>
 *
 * Copyright (c) 2021, Arm Ltd. All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
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

 #include <uk/asm.h>
 #include <uk/lcpu.h>
 #include <uk/reloc.h>

.text
/*
 * Secondary CPU entry point.
 *
 * Entered with MMU off. x0 holds the CPU index, passed via PSCI
 * CPU_ON context_id (a3).
 */
ENTRY(lcpu_start)
	/* Disable interrupts */
	msr	daifset, #2

	/* Set the context id */
	msr	contextidr_el1, xzr

	/* Preserve CPU index across start_mmu (x19 is not clobbered) */
	mov	x19, x0

	/* Setup exception vector table address before enable MMU */
	ur_addr	x29, vector_table
	msr	VBAR_EL1, x29

	/* Enable the mmu */
	bl	start_mmu

	/* Set TPIDR_EL1 = _uk_pcpuvar_base + idx * _uk_pcpuvar_tmpl_size */
	uk_pcpuvar_arm64_tpidrval	x0, x19
	adrp	x1, _uk_pcpuvar_base
	add	x1, x1, #:lo12:_uk_pcpuvar_base
	add	x0, x0, x1
	msr	tpidr_el1, x0

	/* Load the stack pointer */
	uk_pcpuvar_arm64_ldr	x0, UK_LCPU_SSTACKP_SYM, x1
	mov	sp, x0

	/* Load the entry address and jump to it */
	uk_pcpuvar_arm64_ldr	x0, UK_LCPU_SARG_SYM, x1
	uk_pcpuvar_arm64_ldr	x1, UK_LCPU_SENTRY_SYM, x2
	br	x1
END(lcpu_start)
