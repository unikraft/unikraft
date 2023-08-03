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

#ifndef __PLAT_COMMON_ARM64_CPU_H__
#define __PLAT_COMMON_ARM64_CPU_H__

#include <inttypes.h>
#include <uk/essentials.h>
#include <uk/plat/bootstrap.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <arm/smccc.h>
#include <uk/plat/common/lcpu.h>

static inline void _init_cpufeatures(void)
{
}

/* Define compatibility IO macros */
#define outb(addr, v)   UK_BUG()
#define outw(addr, v)   UK_BUG()
#define inb(addr)       UK_BUG()

/*
 * PSCI conduit method to call functions, based on the SMC Calling Convention.
 */
extern smccc_conduit_fn_t smccc_psci_call;

/* CPU native APIs */
void halt(void);
void reset(void);
void system_off(enum ukplat_gstate request);
#ifdef CONFIG_HAVE_SMP
int cpu_on(__lcpuid id, __paddr_t entry, void *arg);
#endif /* CONFIG_HAVE_SMP */

#ifdef CONFIG_FPSIMD

struct fpsimd_state {
	__u64		regs[32 * 2];
	__u32		fpsr;
	__u32		fpcr;
};

extern void fpsimd_save_state(uintptr_t ptr);
extern void fpsimd_restore_state(uintptr_t ptr);

static inline void save_extregs(void *ectx)
{
	fpsimd_save_state((uintptr_t) ectx);

	/* make sure sysreg writing takes effects */
	isb();
}

static inline void restore_extregs(void *ectx)
{
	fpsimd_restore_state((uintptr_t) ectx);

	/* make sure sysreg writing takes effects */
	isb();
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

#endif /* __PLAT_COMMON_ARM64_CPU_H__ */
