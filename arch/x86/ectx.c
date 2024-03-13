/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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
#include <uk/hexdump.h>
#include <uk/isr/string.h> /* memset_isr */

enum x86_save_method {
	X86_SAVE_NONE = 0,
	X86_SAVE_FSAVE,
	X86_SAVE_FXSAVE,
	X86_SAVE_XSAVE,
	X86_SAVE_XSAVEOPT
};

static enum x86_save_method ectx_method;
static __sz ectx_size;
static __sz ectx_align = 0x0;

static void _init_ectx_store(void)
{
	__u32 eax, ebx, ecx, edx;

	/* Why are we saving the eax register content to the eax variable with
	 * "=a(eax)", but then never use it?
	 * Because gcc otherwise will assume that the eax register still
	 * contains "1" after this asm expression. See the "Warning" note at
	 * https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#InputOperands
	 */
	ukarch_x86_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (ecx & X86_CPUID1_ECX_OSXSAVE) {
		ukarch_x86_cpuid(0xd, 1, &eax, &ebx, &ecx, &edx);
		if (eax & X86_CPUIDD1_EAX_XSAVEOPT) {
			ectx_method = X86_SAVE_XSAVEOPT;
			uk_pr_debug("Load/store of extended CPU state: XSAVEOPT\n");
		} else {
			ectx_method = X86_SAVE_XSAVE;
			uk_pr_debug("Load/store of extended CPU state: XSAVE\n");
		}
		ukarch_x86_cpuid(0xd, 0, &eax, &ebx, &ecx, &edx);
		ectx_size = ebx;
		ectx_align = 64;
	} else if (edx & X86_CPUID1_EDX_FXSR) {
		ectx_method = X86_SAVE_FXSAVE;
		ectx_size = 512;
		ectx_align = 16;
		uk_pr_debug("Load/store of extended CPU state: FXSAVE\n");
	} else {
		ectx_method = X86_SAVE_FSAVE;
		ectx_size = 108;
		ectx_align = 1;
		uk_pr_debug("Load/store of extended CPU state: FSAVE\n");
	}

	/* NOTE: In case a condition is added here that disables extregs
	 *       (size=0), please make sure that align is still set to 1
	 *       so that we can detect if _init_ectx_store() was called.
	 */
}
UK_CTOR_PRIO(_init_ectx_store, 0);

__sz ukarch_ectx_size(void)
{
	UK_ASSERT(ectx_align); /* Do not call when not yet initialized */

	return ectx_size;
}

__sz ukarch_ectx_align(void)
{
	UK_ASSERT(ectx_align); /* Do not call when not yet initialized */

	return ectx_align;
}

void ukarch_ectx_sanitize(struct ukarch_ectx *state)
{
	UK_ASSERT(ectx_align); /* Do not call when not yet initialized */
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr) state, ectx_align));

	switch (ectx_method) {
	case X86_SAVE_XSAVE:
	case X86_SAVE_XSAVEOPT:
		/* XSAVE* & XRSTOR rely on sane values in the XSAVE header
		 * (64 bytes starting at offset 512 from the base address)
		 * and will raise #GP on garbage data. We must zero them out.
		 */
		((__u64 *)state)[64] = 0;
		((__u64 *)state)[65] = 0;
		((__u64 *)state)[66] = 0;
		((__u64 *)state)[67] = 0;
		((__u64 *)state)[68] = 0;
		((__u64 *)state)[69] = 0;
		((__u64 *)state)[70] = 0;
		((__u64 *)state)[71] = 0;
		break;
	default: /* Nothing to be done in the general case. */
		break;
	}
}

void ukarch_ectx_init(struct ukarch_ectx *state)
{
	UK_ASSERT(ectx_align); /* Do not call when not yet initialized */
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr) state, ectx_align));

	/* Initialize extregs area:
	 * Zero out and then save a valid layout to it.
	 */
	memset_isr(state, 0, ectx_size);
	ukarch_ectx_store(state);
}

void ukarch_ectx_store(struct ukarch_ectx *state)
{
	UK_ASSERT(ectx_align); /* Do not call when not yet initialized */
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr) state, ectx_align));

	switch (ectx_method) {
	case X86_SAVE_NONE:
		/* nothing to do */
		break;
	case X86_SAVE_FSAVE:
		asm volatile("fsave (%0)" :: "r"(state) : "memory");
		break;
	case X86_SAVE_FXSAVE:
		asm volatile("fxsave (%0)" :: "r"(state) : "memory");
		break;
	case X86_SAVE_XSAVE:
		asm volatile("xsave (%0)" :: "r"(state),
			     "a"(0xffffffff), "d"(0xffffffff) : "memory");
		break;
	case X86_SAVE_XSAVEOPT:
		asm volatile("xsaveopt (%0)" :: "r"(state),
			     "a"(0xffffffff), "d"(0xffffffff) : "memory");
		break;
	}
}

void ukarch_ectx_load(struct ukarch_ectx *state)
{
	UK_ASSERT(ectx_align); /* Do not call when not yet initialized */
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr) state, ectx_align));

	switch (ectx_method) {
	case X86_SAVE_NONE:
		/* nothing to do */
		break;
	case X86_SAVE_FSAVE:
		asm volatile("frstor (%0)" :: "r"(state));
		break;
	case X86_SAVE_FXSAVE:
		asm volatile("fxrstor (%0)" :: "r"(state));
		break;
	case X86_SAVE_XSAVE:
	case X86_SAVE_XSAVEOPT:
		asm volatile("xrstor (%0)" :: "r"(state),
			     "a"(0xffffffff), "d"(0xffffffff));
		break;
	}
}

#ifdef CONFIG_ARCH_X86_64
void ukarch_ectx_assert_equal(struct ukarch_ectx *state)
{
	__u8 ectxbuf[ectx_size + ectx_align];
	struct ukarch_ectx *current;

	/* Store the current state */
	current = (struct ukarch_ectx *)ALIGN_UP((__uptr)ectxbuf, ectx_align);
	ukarch_ectx_init(current);

	if (memcmp_isr(current, state, ectx_size) != 0) {
		uk_pr_crit("Modified ECTX detected!\n");
		uk_pr_crit("Current:\n");
		uk_hexdumpk(KLVL_CRIT, current, ectx_size,
			    UK_HXDF_ADDR | UK_HXDF_GRPQWORD | UK_HXDF_COMPRESS,
			    2);

		uk_pr_crit("Expected:\n");
		uk_hexdumpk(KLVL_CRIT, state, ectx_size,
			    UK_HXDF_ADDR | UK_HXDF_GRPQWORD | UK_HXDF_COMPRESS,
			    2);

		UK_CRASH("Modified ECTX\n");
	}
}
#endif
