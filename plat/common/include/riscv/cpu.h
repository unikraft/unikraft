/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *	    Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
 *
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

#ifndef __PLAT_COMMON_RISCV64_CPU_H__
#define __PLAT_COMMON_RISCV64_CPU_H__

#include <uk/essentials.h>
#include <uk/arch/types.h>
#include <stdint.h>

void halt(void);
void system_off(void);

#ifdef CONFIG_FPSIMD
struct fpsimd_state {
	__u64 regs[32];
	__u32 fcsr;
	/* There's no SIMD on RISC-V, yet */
};

extern void fpsimd_save_state(uintptr_t ptr);
extern void fpsimd_restore_state(uintptr_t ptr);

static inline void save_extregs(void *ectx)
{
	fpsimd_save_state((uintptr_t) ectx);
}

static inline void restore_extregs(void *ectx)
{
	fpsimd_restore_state((uintptr_t) ectx);
}

#else /* !CONFIG_FPSIMD */

struct fpsimd_state { };

static inline void save_extregs(void *ectx __unused)
{
}

static inline void restore_extregs(void *ectx __unused)
{
}
#endif /* CONFIG_FPSIMD */

#endif
