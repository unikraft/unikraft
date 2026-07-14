/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */


#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/arch/x86_64.h>
#include <uk/asm.h>
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
#define X86_XSAVE_HDR_XSTATE_BV_MPX_BNDREGSF		(1UL <<  3)
#define X86_XSAVE_HDR_XSTATE_BV_MPX_BNDCSRF		(1UL <<  4)
#define X86_XSAVE_HDR_XSTATE_BV_AVX512_OPMASKF		(1UL <<  5)
#define X86_XSAVE_HDR_XSTATE_BV_AVX512_ZMM_HI256F	(1UL <<  6)
#define X86_XSAVE_HDR_XSTATE_BV_AVX512_HI16_ZMMF	(1UL <<  7)
	__u64 xstate_bv;
#define X86_XSAVE_HDR_XCOMP_BV_COMPF			(1UL << 63)
	__u64 xcomp_bv;
	/* Bytes 63:16 of the XSAVE header are reserved */
	__u8 rsvd[48];
} __packed;

/* AVX-512 state components */
struct x86_avx512_ctx {
	/*
	 * AVX-512 opmask registers (k0-k7)
	 * 64 bytes (8 registers x 8 bytes each)
	 */
	__u8 opmask[64];

	/*
	 * AVX-512 ZMM_Hi256 state (ZMM0-ZMM15 upper 256 bits)
	 * 512 bytes (16 registers x 32 bytes each)
	 * Bits 511:256 of ZMM0-ZMM15
	 */
	__u8 zmm_hi256[512];

	/*
	 * AVX-512 Hi16_ZMM state (ZMM16-ZMM31)
	 * 1024 bytes (16 registers x 64 bytes each)
	 * Full 512-bit ZMM16-ZMM31 registers
	 */
	__u8 hi16_zmm[1024];
} __packed;

/*
 * MPX state (components 3 and 4).
 * BNDREGS is component 3, BNDCSR is component 4.
 * They are contiguous and together constitute 128 bytes.
 */
struct x86_mpx_ctx {
	/* component 3: BNDREGS */
	__u8 bndregs[64];
	/* component 4: BNDCSR  */
	__u8 bndcsr[64];
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
	 * at the beginning of the extended XSAVE area.
	 *
	 * AVX state has 256 bytes: 127:0 for YMM0_H-YMM7_H and 255:128
	 * for YMM8_H-YMM15_H.
	 */
	__u8 avx_state[256];

	/*
	 * The layout of the XSAVE area after the AVX state depends on whether
	 * MPX is supported or not. It seems to not matter whether it is
	 * enabled in the guest or not, it will take up space either way.
	 * As reported by CPUID, we should compute at runtime all offsets and
	 * sizes, but we choose to be pragmatic and use the offsets/sizes that
	 * have been observed in practice and assert at runtime if any of the
	 * assumptions are wrong.
	 * That being said, we assume MPX components have a fixed offset of 960,
	 * meaning there is a 128-byte architectural gap between the end of
	 * AVX state (832) and BNDREGS (960) - this seems to be the case in
	 * practice.
	 * When MPX is absent, AVX-512 components follow AVX directly from
	 * offset 832.
	 *
	 * Without MPX:
	 *   AVX-512 k regs    (comp 5) at offset 832
	 *   AVX-512 ZMM_Hi256 (comp 6) at offset 896
	 *   AVX-512 Hi16_ZMM  (comp 7) at offset 1408
	 *
	 * With MPX:
	 *   [architectural gap]        at offset 832,  size 128
	 *   BNDREGS (comp 3)           at offset 960,  size 64
	 *   BNDCSR  (comp 4)           at offset 1024, size 64
	 *   AVX-512 k regs    (comp 5) at offset 1088
	 *   AVX-512 ZMM_Hi256 (comp 6) at offset 1152
	 *   AVX-512 Hi16_ZMM  (comp 7) at offset 1664
	 */
	union {
		/* No MPX: AVX-512 follows AVX directly at offset 832. */
		struct x86_avx512_ctx avx512_state;

		/* MPX present: 128-byte gap at 832, MPX at 960, AVX-512 at
		 * 1088.
		 */
		struct _mpx_and_avx512 {
			__u8 pre_mpx_gap[128];              /* offset 832 */
			struct x86_mpx_ctx mpx_state;       /* offset 960 */
			struct x86_avx512_ctx avx512_state; /* offset 1088 */
		} __packed mpx_and_avx512_state;
	} post_avx_state;
} __packed __align(64);

UK_CTASSERT(sizeof(struct x86_xsave_hdr) == 64);
UK_CTASSERT(sizeof(struct x86_avx512_ctx) == 1600);
UK_CTASSERT(sizeof(struct x86_mpx_ctx) == 128);

/* _mpx_and_avx512 internal offsets and total size */
UK_CTASSERT(__offsetof(struct _mpx_and_avx512, pre_mpx_gap) == 0);
UK_CTASSERT(__offsetof(struct _mpx_and_avx512, mpx_state) == 128);
UK_CTASSERT(__offsetof(struct _mpx_and_avx512, avx512_state) == 256);
UK_CTASSERT(sizeof(struct _mpx_and_avx512) == 1856);

/* x86_xsave_ctx field offsets against Intel SDM */
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, x87_state1) == 0);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, mxcsr) == 24);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, mxcsr_mask) == 28);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, x87_state2) == 32);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, sse_state) == 160);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, avail) == 416);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, xsave_hdr) == 512);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, avx_state) == 576);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, post_avx_state) == 832);

/* Union arms: without MPX, AVX-512 is at 832; with MPX, gap at 832,
 * MPX at 960, AVX-512 at 1088.
 */
UK_CTASSERT(__offsetof(struct x86_xsave_ctx,
		post_avx_state.avx512_state) == 832);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx,
		post_avx_state.mpx_and_avx512_state.pre_mpx_gap) == 832);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx,
		post_avx_state.mpx_and_avx512_state.mpx_state) == 960);
UK_CTASSERT(__offsetof(struct x86_xsave_ctx,
		post_avx_state.mpx_and_avx512_state.avx512_state) == 1088);

/* Cross-check via two-term form kept in sync with runtime expressions */
UK_CTASSERT(__offsetof(struct x86_xsave_ctx, post_avx_state) +
	    __offsetof(struct _mpx_and_avx512, avx512_state) == 1088);

/* sizeof covers the widest layout: AVX + gap + MPX + AVX-512 */
UK_CTASSERT(sizeof(struct x86_xsave_ctx) == 2688);

/* TODO: Take PKRU into account */

static enum x86_save_method ectx_method;
static __sz ectx_size;
static __sz ectx_align;
static __bool have_mpx;

static void _init_ectx_store(void)
{
	__u32 eax, ebx, ecx, edx;

	/* Why are we saving the eax register content to the eax variable with
	 * "=a(eax)", but then never use it?
	 * Because gcc otherwise will assume that the eax register still
	 * contains "1" after this asm expression. See the "Warning" note at
	 * https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#InputOperands
	 */
	uk_arch_x86_64_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (ecx & UK_ARCH_X86_64_CPUID1_ECX_OSXSAVE) {
		uk_arch_x86_64_cpuid(0xd, 1, &eax, &ebx, &ecx, &edx);
		if (eax & UK_ARCH_X86_64_CPUIDD1_EAX_XSAVEOPT) {
			ectx_method = X86_SAVE_XSAVEOPT;
			uk_pr_debug("Load/store of extended CPU state: XSAVEOPT\n");
		} else {
			ectx_method = X86_SAVE_XSAVE;
			uk_pr_debug("Load/store of extended CPU state: XSAVE\n");
		}
		uk_arch_x86_64_cpuid(0xd, 0, &eax, &ebx, &ecx, &edx);
		ectx_size = ebx;
		ectx_align = __alignof(struct x86_xsave_ctx);

		/*
		 * Assert that ectx_size matches one of the known layouts.
		 * Ideally all offsets and sizes would be computed at runtime
		 * from CPUID, but for now we hardcode the known values and
		 * rely on this assert to catch unexpected configurations.
		 *
		 * Known sizes:
		 *   576  — x87 + SSE + XSAVE header only (no AVX)
		 *   832  — AVX, no MPX, no AVX-512
		 *   1088 — AVX + MPX, no AVX-512
		 *          (128-byte gap at 832, BNDREGS+BNDCSR at 960)
		 *   2432 — AVX + AVX-512, no MPX
		 *          (AVX-512 starts directly at 832)
		 *   2688 — AVX + MPX + AVX-512
		 *          (AVX-512 starts at 1088)
		 */
		UK_ASSERT(ectx_size == __offsetof(struct x86_xsave_ctx,
						  avx_state) ||
			  ectx_size == __offsetof(struct x86_xsave_ctx,
						  post_avx_state) ||
			  ectx_size == __offsetof(struct x86_xsave_ctx,
						  post_avx_state) +
				       __offsetof(struct _mpx_and_avx512,
						  avx512_state) ||
			  ectx_size == __offsetof(struct x86_xsave_ctx,
						  post_avx_state) +
				       sizeof(struct x86_avx512_ctx) ||
			  ectx_size == sizeof(struct x86_xsave_ctx));

		/*
		 * MPX is present when the XSAVE area includes the gap+MPX
		 * region before AVX-512, i.e. AVX-512 starts at 1088 rather
		 * than 832. This covers the MPX-only case (1088) and the
		 * MPX+AVX-512 case (2688).
		 *
		 * Deducing MPX presence from the size instead of doing another
		 * CPUID...
		 */
		have_mpx = (ectx_size == __offsetof(struct x86_xsave_ctx,
						    post_avx_state) +
					 __offsetof(struct _mpx_and_avx512,
						    avx512_state) ||
			    ectx_size == sizeof(struct x86_xsave_ctx));

		if (have_mpx)
			uk_pr_debug("MPX state components present in XSAVE area\n");

	} else if (edx & UK_ARCH_X86_64_CPUID1_EDX_FXSR) {
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

/*
 * Return a pointer to the AVX-512 sub-state within an XSAVE context,
 * selecting the correct union arm depending on whether MPX is present.
 */
static inline const struct x86_avx512_ctx *
x86_xsave_avx512(const struct x86_xsave_ctx *ctx)
{
	if (have_mpx)
		return &ctx->post_avx_state.mpx_and_avx512_state.avx512_state;
	return &ctx->post_avx_state.avx512_state;
}

void uk_plat_native_ectx_sanitize(struct uk_plat_native_ectx *state)
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

void uk_plat_native_ectx_store(struct uk_plat_native_ectx *state)
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

void uk_plat_native_ectx_init(struct uk_plat_native_ectx *state)
{
	UK_ASSERT(ectx_align); /* Do not call when not yet initialized */
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr)state, ectx_align));

	/* Initialize extregs area:
	 * Zero out and then save a valid layout to it.
	 */
	memset_isr(state, 0, ectx_size);
	uk_plat_native_ectx_store(state);
}

void uk_plat_native_ectx_load(struct uk_plat_native_ectx *state)
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
	const struct x86_avx512_ctx *avx512_1, *avx512_2;
	const struct x86_mpx_ctx *mpx1, *mpx2;
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
	case X86_XSAVE_HDR_XSTATE_BV_MPX_BNDREGSF:
		UK_ASSERT(have_mpx);
		mpx1 = &ctx1->post_avx_state.mpx_and_avx512_state.mpx_state;
		mpx2 = &ctx2->post_avx_state.mpx_and_avx512_state.mpx_state;
		if ((rc = memcmp_isr(mpx1->bndregs, mpx2->bndregs,
				     sizeof(mpx1->bndregs)))) {
			uk_pr_debug("MPX BNDREGS state differs!\n");
			return rc;
		}
		break;
	case X86_XSAVE_HDR_XSTATE_BV_MPX_BNDCSRF:
		UK_ASSERT(have_mpx);
		mpx1 = &ctx1->post_avx_state.mpx_and_avx512_state.mpx_state;
		mpx2 = &ctx2->post_avx_state.mpx_and_avx512_state.mpx_state;
		if ((rc = memcmp_isr(mpx1->bndcsr, mpx2->bndcsr,
				     sizeof(mpx1->bndcsr)))) {
			uk_pr_debug("MPX BNDCSR state differs!\n");
			return rc;
		}
		break;
	case X86_XSAVE_HDR_XSTATE_BV_AVX512_OPMASKF:
		avx512_1 = x86_xsave_avx512(ctx1);
		avx512_2 = x86_xsave_avx512(ctx2);
		if ((rc = memcmp_isr(avx512_1->opmask, avx512_2->opmask,
				     sizeof(avx512_1->opmask)))) {
			uk_pr_debug("AVX-512 opmask state differs!\n");
			return rc;
		}
		break;
	case X86_XSAVE_HDR_XSTATE_BV_AVX512_ZMM_HI256F:
		avx512_1 = x86_xsave_avx512(ctx1);
		avx512_2 = x86_xsave_avx512(ctx2);
		if ((rc = memcmp_isr(avx512_1->zmm_hi256, avx512_2->zmm_hi256,
				     sizeof(avx512_1->zmm_hi256)))) {
			uk_pr_debug("AVX-512 ZMM upper 256 bits state differs!\n");
			return rc;
		}
		break;
	case X86_XSAVE_HDR_XSTATE_BV_AVX512_HI16_ZMMF:
		avx512_1 = x86_xsave_avx512(ctx1);
		avx512_2 = x86_xsave_avx512(ctx2);
		if ((rc = memcmp_isr(avx512_1->hi16_zmm, avx512_2->hi16_zmm,
				     sizeof(avx512_1->hi16_zmm)))) {
			uk_pr_debug("AVX-512 Hi16 ZMM state differs!\n");
			return rc;
		}
		break;
	default:
		UK_CRASH("Unknown XSAVE substate: %lu\n", substate);
	}

	return 0;
}

static inline __bool mem_iszero(const void *mem, __sz len)
{
	for (__sz i = 0; i < len; i++)
		if (((const __u8 *)mem)[i] != 0)
			return __false;

	return __true;
}

/*
 * Check if a state component has the initial values defined by the
 * architecture (all zeroes).
 */
static inline __bool x86_xsave_substate_isinit(const struct x86_xsave_ctx *ctx,
					       __u64 substate)
{
	const struct x86_avx512_ctx *avx512;
	const struct x86_mpx_ctx *mpx;

	switch (substate) {
	case X86_XSAVE_HDR_XSTATE_BV_X87F:
		return mem_iszero(ctx->x87_state1, sizeof(ctx->x87_state1)) &&
		       mem_iszero(ctx->x87_state2, sizeof(ctx->x87_state2));
	case X86_XSAVE_HDR_XSTATE_BV_SSEF:
		return mem_iszero(ctx->sse_state, sizeof(ctx->sse_state));
	case X86_XSAVE_HDR_XSTATE_BV_AVXF:
		return mem_iszero(ctx->avx_state, sizeof(ctx->avx_state));
	case X86_XSAVE_HDR_XSTATE_BV_MPX_BNDREGSF:
		UK_ASSERT(have_mpx);
		mpx = &ctx->post_avx_state.mpx_and_avx512_state.mpx_state;
		return mem_iszero(mpx->bndregs, sizeof(mpx->bndregs));
	case X86_XSAVE_HDR_XSTATE_BV_MPX_BNDCSRF:
		UK_ASSERT(have_mpx);
		mpx = &ctx->post_avx_state.mpx_and_avx512_state.mpx_state;
		return mem_iszero(mpx->bndcsr, sizeof(mpx->bndcsr));
	case X86_XSAVE_HDR_XSTATE_BV_AVX512_OPMASKF:
		avx512 = x86_xsave_avx512(ctx);
		return mem_iszero(avx512->opmask, sizeof(avx512->opmask));
	case X86_XSAVE_HDR_XSTATE_BV_AVX512_ZMM_HI256F:
		avx512 = x86_xsave_avx512(ctx);
		return mem_iszero(avx512->zmm_hi256, sizeof(avx512->zmm_hi256));
	case X86_XSAVE_HDR_XSTATE_BV_AVX512_HI16_ZMMF:
		avx512 = x86_xsave_avx512(ctx);
		return mem_iszero(avx512->hi16_zmm, sizeof(avx512->hi16_zmm));
	default:
		UK_CRASH("Unknown XSAVE substate: %lu\n", substate);
	}
}

static inline __bool x86_xsave_mxcsr_iseq(const struct x86_xsave_ctx *ctx1,
					  const struct x86_xsave_ctx *ctx2)
{
	/*
	 * Bytes 27:24 of XSAVE area are for the MXCSR register which is
	 * loaded regardless of XSTATE_BV bitmap, so check unconditionally.
	 */
	return ctx1->mxcsr == ctx2->mxcsr;
}

static __bool x86_xsave_substate_iseq(const struct x86_xsave_ctx *ctx1,
				      const struct x86_xsave_ctx *ctx2,
				      __u64 substate)
{
	__u64 ctx1_bitf, ctx2_bitf;

	ctx1_bitf = ctx1->xsave_hdr.xstate_bv & substate;
	ctx2_bitf = ctx2->xsave_hdr.xstate_bv & substate;

	if (!ctx1_bitf && !ctx2_bitf)
		return __true;

	if (ctx1_bitf != ctx2_bitf)
		return x86_xsave_substate_isinit(ctx1_bitf ? ctx1 : ctx2,
						 substate);

	return x86_xsave_substate_memcmp(ctx1, ctx2, substate) == 0;
}

static inline __bool x86_xsave_hdr_isvalid(const struct x86_xsave_ctx *ctx)
{
	__u64 xhdr_supp_mask = X86_XSAVE_HDR_XSTATE_BV_X87F |
			       X86_XSAVE_HDR_XSTATE_BV_SSEF |
			       X86_XSAVE_HDR_XSTATE_BV_AVXF |
			       X86_XSAVE_HDR_XSTATE_BV_AVX512_OPMASKF |
			       X86_XSAVE_HDR_XSTATE_BV_AVX512_ZMM_HI256F |
			       X86_XSAVE_HDR_XSTATE_BV_AVX512_HI16_ZMMF;
	const struct x86_xsave_hdr *xhdr = &ctx->xsave_hdr;

	if (have_mpx)
		xhdr_supp_mask |= X86_XSAVE_HDR_XSTATE_BV_MPX_BNDREGSF |
				  X86_XSAVE_HDR_XSTATE_BV_MPX_BNDCSRF;

	/*
	 * It is impossible for XCOMP_BV[63] to be set since we do not
	 * use XSAVEC.
	 */
	return xhdr->xcomp_bv == 0 &&
	       (xhdr->xstate_bv & ~xhdr_supp_mask) == 0 &&
	       mem_iszero(xhdr->rsvd, sizeof(xhdr->rsvd));
}

void uk_plat_native_ectx_assert_equal(struct uk_plat_native_ectx *state)
{
	__u8 ectxbuf[ectx_size + ectx_align];
	struct uk_plat_native_ectx *current;

	/* Store the current state */
	current = (struct uk_plat_native_ectx *)ALIGN_UP((__uptr)ectxbuf,
							 ectx_align);
	uk_plat_native_ectx_init(current);

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
	{
		/* According to Intel SDM, XSAVE does not use bytes 511:416 */
		struct x86_fxsave_ctx *fxsave1 = (struct x86_fxsave_ctx *)state;
		struct x86_fxsave_ctx *fxsave2 =
			(struct x86_fxsave_ctx *)current;

		if (unlikely(memcmp_isr(fxsave1->state, fxsave2->state,
					sizeof(fxsave1->state))))
			goto ectx_corrupted;
		break;
	}
	case X86_SAVE_XSAVE:
	case X86_SAVE_XSAVEOPT:
	{
		struct x86_xsave_ctx *xsave1 = (struct x86_xsave_ctx *)state;
		struct x86_xsave_ctx *xsave2 = (struct x86_xsave_ctx *)current;
		const __u64 xbvf[] = {
			X86_XSAVE_HDR_XSTATE_BV_X87F,
			X86_XSAVE_HDR_XSTATE_BV_SSEF,
			X86_XSAVE_HDR_XSTATE_BV_AVXF,
			X86_XSAVE_HDR_XSTATE_BV_AVX512_OPMASKF,
			X86_XSAVE_HDR_XSTATE_BV_AVX512_ZMM_HI256F,
			X86_XSAVE_HDR_XSTATE_BV_AVX512_HI16_ZMMF,
			X86_XSAVE_HDR_XSTATE_BV_MPX_BNDREGSF,
			X86_XSAVE_HDR_XSTATE_BV_MPX_BNDCSRF,
		};
		__sz nflags = 6; /* xbvf flags not including MPX slots */

		if (have_mpx)
			nflags = 8;

		if (unlikely(!x86_xsave_hdr_isvalid(xsave2)))
			UK_CRASH("Error in saving current ectx\n");

		if (unlikely(!x86_xsave_hdr_isvalid(xsave1) ||
			     !x86_xsave_mxcsr_iseq(xsave1, xsave2)))
			goto ectx_corrupted;

		for (__sz i = 0; i < nflags; i++)
			if (unlikely(!x86_xsave_substate_iseq(xsave1, xsave2,
							      xbvf[i])))
				goto ectx_corrupted;
		break;
	}
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
