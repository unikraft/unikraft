/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Robert Kuban <robert.kuban@opensynergy.com>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH All rights reserved.
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
#include <uk/arch/ctx.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/types.h>
#include <uk/ctors.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <string.h> /* memset */
#include <riscv/cpu.h>

__sz ukarch_ectx_size(void)
{
	return sizeof(struct fpsimd_state);
}

/* TODO: find good alignment for ectx on RISC-V*/
#define ECTX_ALIGN 16

__sz ukarch_ectx_align(void)
{
	return ECTX_ALIGN;
}

void ukarch_ectx_init(struct ukarch_ectx *state)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr) state, ECTX_ALIGN));

	/* Initialize extregs area:
	 * Zero out and then save a valid layout to it.
	 */
	memset(state, 0, sizeof(struct fpsimd_state));
	ukarch_ectx_store(state);
}

void ukarch_ectx_store(struct ukarch_ectx *state)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr) state, ECTX_ALIGN));

	save_extregs(state);
}

void ukarch_ectx_load(struct ukarch_ectx *state)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr) state, ECTX_ALIGN));

	restore_extregs(state);
}
