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

#include <stdbool.h>
#include <uk/arch/ctx.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/types.h>
#include <uk/ctors.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/print/hexdump.h>
#include <uk/isr/string.h> /* memset_isr */

enum x86_save_method {
	X86_SAVE_NONE = 0,
	X86_SAVE_FSAVE,
	X86_SAVE_FXSAVE,
	X86_SAVE_XSAVE,
	X86_SAVE_XSAVEOPT
};

struct x86_fsave_ctx {
	__u8 state[108];
} __packed;

struct x86_fxsave_ctx {
	__u8 state[416];
	__u8 avail[96];
} __packed __align(16);

struct x86_xsave_hdr {
#define X86_XSAVE_HDR_XSTATE_BV_X87F			(1UL <<  0)
#define X86_XSAVE_HDR_XSTATE_BV_SSEF			(1UL <<  1)
#define X86_XSAVE_HDR_XSTATE_BV_AVXF			(1UL <<  2)
	__u64 xstate_bv;
#define X86_XSAVE_HDR_XCOMP_BV_COMPF			(1UL << 63)
	__u64 xcomp_bv;
	/* Bytes 63:16 of the XSAVE header are reserved */
	__u8 rsvd[48];
} __packed;

struct x86_xsave_ctx {
	/* x87 state comprises bytes 23:0 and bytes 159:32 */
	__u8 x87_state1[24];
	__u32 mxcsr;
	__u32 mxcsr_mask;
	__u8 x87_state2[128];
	/* SSE state comprises bytes 31:24 and bytes 415:160 */
	__u8 sse_state[256];
	__u8 avail[96];
	struct x86_xsave_hdr xsave_hdr;
	/*
	 * AVX state comprises bytes 831:576, after the XSAVE header,
	 * at the beginning of the extended xsave area.
	 *
	 * AVX state has 256 bytes: 127:0 for YMM0_H–YMM7_H and 255:128
	 * for YMM8_H–YMM15_H.
	 */
	__u8 avx_state[256];
} __packed __align(64);

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
		ectx_align = __alignof(struct x86_xsave_ctx);

		UK_ASSERT(ectx_size == sizeof(struct x86_xsave_ctx) ||
			  ectx_size == __offsetof(struct x86_xsave_ctx,
						  avx_state));
	} else if (edx & X86_CPUID1_EDX_FXSR) {
		ectx_method = X86_SAVE_FXSAVE;
		ectx_size = sizeof(struct x86_fxsave_ctx);
		ectx_align = __alignof(struct x86_fxsave_ctx);
		uk_pr_debug("Load/store of extended CPU state: FXSAVE\n");
	} else {
		ectx_method = X86_SAVE_FSAVE;
		ectx_size = sizeof(struct x86_fsave_ctx);
		ectx_align = __alignof(struct x86_fsave_ctx);
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
		asm volatile("xsave64 (%0)" :: "r"(state),
			     "a"(0xffffffff), "d"(0xffffffff) : "memory");
		break;
	case X86_SAVE_XSAVEOPT:
		asm volatile("xsaveopt64 (%0)" :: "r"(state),
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

static inline int x86_xsave_substate_memcmp(const struct x86_xsave_ctx *ctx1,
					    const struct x86_xsave_ctx *ctx2,
					    __u64 substate)
{
	int rc;

	switch (substate) {
	case X86_XSAVE_HDR_XSTATE_BV_X87F:
		if ((rc = memcmp_isr(ctx1->x87_state1, ctx2->x87_state1,
				     sizeof(ctx1->x87_state1))) ||
		    (rc = memcmp_isr(ctx1->x87_state2, ctx2->x87_state2,
				     sizeof(ctx1->x87_state2)))) {
			uk_pr_debug("x87 state differs!\n");
			return rc;
		}
		break;
	case X86_XSAVE_HDR_XSTATE_BV_SSEF:
		if ((rc = memcmp_isr(ctx1->sse_state, ctx2->sse_state,
				     sizeof(ctx1->sse_state)))) {
			uk_pr_debug("SSE state differs!\n");
			return rc;
		}
		break;
	case X86_XSAVE_HDR_XSTATE_BV_AVXF:
		if ((rc = memcmp_isr(ctx1->avx_state, ctx2->avx_state,
				     sizeof(ctx1->avx_state)))) {
			uk_pr_debug("AVX state differs!\n");
			return rc;
		}
		break;
	default:
		UK_CRASH("Unknown XSAVE substate: %lu\n", substate);
	}

	return 0;
}

static inline bool mem_iszero(const void *mem, __sz len)
{
	for (__sz i = 0; i < len; i++)
		if (((const __u8 *)mem)[i] != 0)
			return false;

	return true;
}

/*
 * Check if a state component has the initial values defined by the
 * architecture (all zeroes).
 */
static inline bool x86_xsave_substate_isinit(const struct x86_xsave_ctx *ctx,
					     __u64 substate)
{
	switch (substate) {
	case X86_XSAVE_HDR_XSTATE_BV_X87F:
		return mem_iszero(ctx->x87_state1, sizeof(ctx->x87_state1)) &&
		       mem_iszero(ctx->x87_state2, sizeof(ctx->x87_state2));
	case X86_XSAVE_HDR_XSTATE_BV_SSEF:
		return mem_iszero(ctx->sse_state, sizeof(ctx->sse_state));
	case X86_XSAVE_HDR_XSTATE_BV_AVXF:
		return mem_iszero(ctx->avx_state, sizeof(ctx->avx_state));
	default:
		UK_CRASH("Unknown XSAVE substate: %lu\n", substate);
	}
}

static inline bool x86_xsave_mxcsr_iseq(const struct x86_xsave_ctx *ctx1,
					const struct x86_xsave_ctx *ctx2)
{
	/*
	 * Bytes 27:24 of XSAVE area are for the MXCSR register which is
	 * loaded regardless of XSTATE_BV bitmap, so check unconditionally.
	 */
	return ctx1->mxcsr == ctx2->mxcsr;
}

static bool x86_xsave_substate_iseq(const struct x86_xsave_ctx *ctx1,
				    const struct x86_xsave_ctx *ctx2,
				    __u64 substate)
{
	__u64 ctx1_bitf, ctx2_bitf;

	ctx1_bitf = ctx1->xsave_hdr.xstate_bv & substate;
	ctx2_bitf = ctx2->xsave_hdr.xstate_bv & substate;

	if (!ctx1_bitf && !ctx2_bitf)
		return true;

	if (ctx1_bitf != ctx2_bitf)
		return x86_xsave_substate_isinit(ctx1_bitf ? ctx1 : ctx2,
						 substate);

	return x86_xsave_substate_memcmp(ctx1, ctx2, substate) == 0;
}

static inline bool x86_xsave_hdr_isvalid(const struct x86_xsave_ctx *ctx)
{
	const __u64 xhdr_supp_mask = X86_XSAVE_HDR_XSTATE_BV_X87F |
				     X86_XSAVE_HDR_XSTATE_BV_SSEF |
				     X86_XSAVE_HDR_XSTATE_BV_AVXF;
	const struct x86_xsave_hdr *xhdr = &ctx->xsave_hdr;

	/*
	 * It is impossible for XCOMP_BV[63] to be set since we do not
	 * use XSAVEC.
	 */
	return xhdr->xcomp_bv == 0 &&
	       (xhdr->xstate_bv & ~xhdr_supp_mask) == 0 &&
	       mem_iszero(xhdr->rsvd, sizeof(xhdr->rsvd));
}

void ukarch_ectx_assert_equal(struct ukarch_ectx *state)
{
	__u8 ectxbuf[ectx_size + ectx_align];
	struct ukarch_ectx *current;

	/* Store the current state */
	current = (struct ukarch_ectx *)ALIGN_UP((__uptr)ectxbuf, ectx_align);
	ukarch_ectx_init(current);

	/*
	 * When using XSAVE(OPT) two ectx memory areas may differ
	 * but be equivalent on XRSTOR, thus we cannot simply do memcmp.
	 *
	 * According to the Intel SDM, XSTATE_BV is a bitmap that, depending
	 * on which extended register context subcomponent is used, it may
	 * have its corresponding bit marked as dirty through the CPU-internal
	 * state tracking structure XINUSE. If a subcomponent is enabled
	 * (RFBM[i] = 1) then it is stated that: if the state component is in
	 * its initial configuration, XINUSE[i] may be either 0 or 1, and
	 * XSTATE_BV[i] may be written with either 0 or 1. In other words,
	 * if the component happens to be zeroed out entirely, its
	 * XSTATE_BV[i] can be either 0 or 1, both being valid.
	 * Therefore, in some cases, following XSAVE(OPT), you can have two
	 * ectx that differ in memory but are equivalent when loaded in.
	 * Our comparison must take into account the state of the bitmaps
	 * for proper checking.
	 */
	switch (ectx_method) {
	case X86_SAVE_FSAVE:
		if (unlikely(memcmp_isr(current, state,
					sizeof(struct x86_fsave_ctx))))
			goto ectx_corrupted;
		break;
	case X86_SAVE_FXSAVE:
		/* According to Intel SDM, XSAVE does not use bytes 511:416 */
		struct x86_fxsave_ctx *fxsave1 = (struct x86_fxsave_ctx *)state;
		struct x86_fxsave_ctx *fxsave2 =
			(struct x86_fxsave_ctx *)current;

		if (unlikely(memcmp_isr(fxsave1->state, fxsave2->state,
					sizeof(fxsave1->state))))
			goto ectx_corrupted;
		break;
	case X86_SAVE_XSAVE:
	case X86_SAVE_XSAVEOPT:
		struct x86_xsave_ctx *xsave1 = (struct x86_xsave_ctx *)state;
		struct x86_xsave_ctx *xsave2 = (struct x86_xsave_ctx *)current;

		if (unlikely(!x86_xsave_hdr_isvalid(xsave2)))
			UK_CRASH("Error in saving current ectx\n");

		if (unlikely(!x86_xsave_hdr_isvalid(xsave1) ||
			     !x86_xsave_mxcsr_iseq(xsave1, xsave2) ||
			     !x86_xsave_substate_iseq(xsave1, xsave2,
					     X86_XSAVE_HDR_XSTATE_BV_X87F) ||
			     !x86_xsave_substate_iseq(xsave1, xsave2,
					     X86_XSAVE_HDR_XSTATE_BV_SSEF) ||
			     !x86_xsave_substate_iseq(xsave1, xsave2,
					     X86_XSAVE_HDR_XSTATE_BV_AVXF)))
			goto ectx_corrupted;
		break;
	default:
		UK_CRASH("Unknown ectx method: %d\n", ectx_method);
		return;
	}

	return;

ectx_corrupted:
	uk_pr_crit("Modified ECTX detected!\n");
	uk_pr_crit("Current:\n");
	uk_hexdumpk(UK_PRINT_KLVL_CRIT, current, ectx_size,
		    UK_HXDF_ADDR | UK_HXDF_GRPQWORD | UK_HXDF_COMPRESS,
		    2);

	uk_pr_crit("Expected:\n");
	uk_hexdumpk(UK_PRINT_KLVL_CRIT, state, ectx_size,
		    UK_HXDF_ADDR | UK_HXDF_GRPQWORD | UK_HXDF_COMPRESS,
		    2);

	UK_CRASH("Modified ECTX\n");
}
