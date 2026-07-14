/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/assert.h>
#include <uk/ctors.h>
#include <uk/essentials.h>
#include <uk/isr/string.h> /* memset_isr */
#include <uk/lcpu.h>
#include <uk/print.h>
#include <uk/print/hexdump.h>

#if CONFIG_FPSIMD

struct fpsimd_state {
	__u64		regs[32 * 2];
	__u32		fpsr;
	__u32		fpcr;
};

static void fpsimd_save_state(__uptr ptr)
{
	__u32 fpcr, fpsr;

	__asm__ __volatile__(
		"mrs	%0, fpcr\n"
		"mrs	%1, fpsr\n"
		"stp	q0,  q1,  [%2, #16 *  0]\n"
		"stp	q2,  q3,  [%2, #16 *  2]\n"
		"stp	q4,  q5,  [%2, #16 *  4]\n"
		"stp	q6,  q7,  [%2, #16 *  6]\n"
		"stp	q8,  q9,  [%2, #16 *  8]\n"
		"stp	q10, q11, [%2, #16 * 10]\n"
		"stp	q12, q13, [%2, #16 * 12]\n"
		"stp	q14, q15, [%2, #16 * 14]\n"
		"stp	q16, q17, [%2, #16 * 16]\n"
		"stp	q18, q19, [%2, #16 * 18]\n"
		"stp	q20, q21, [%2, #16 * 20]\n"
		"stp	q22, q23, [%2, #16 * 22]\n"
		"stp	q24, q25, [%2, #16 * 24]\n"
		"stp	q26, q27, [%2, #16 * 26]\n"
		"stp	q28, q29, [%2, #16 * 28]\n"
		"stp	q30, q31, [%2, #16 * 30]\n"
		: "=&r"(fpcr), "=&r"(fpsr) : "r"(ptr));

	((struct fpsimd_state *)ptr)->fpcr = fpcr;
	((struct fpsimd_state *)ptr)->fpsr = fpsr;
}

static void fpsimd_restore_state(__uptr ptr)
{
	__u32 fpcr, fpsr;

	fpcr = ((struct fpsimd_state *)ptr)->fpcr;
	fpsr = ((struct fpsimd_state *)ptr)->fpsr;

	__asm__ __volatile__(
		"ldp	q0,  q1,  [%2, #16 *  0]\n"
		"ldp	q2,  q3,  [%2, #16 *  2]\n"
		"ldp	q4,  q5,  [%2, #16 *  4]\n"
		"ldp	q6,  q7,  [%2, #16 *  6]\n"
		"ldp	q8,  q9,  [%2, #16 *  8]\n"
		"ldp	q10, q11, [%2, #16 * 10]\n"
		"ldp	q12, q13, [%2, #16 * 12]\n"
		"ldp	q14, q15, [%2, #16 * 14]\n"
		"ldp	q16, q17, [%2, #16 * 16]\n"
		"ldp	q18, q19, [%2, #16 * 18]\n"
		"ldp	q20, q21, [%2, #16 * 20]\n"
		"ldp	q22, q23, [%2, #16 * 22]\n"
		"ldp	q24, q25, [%2, #16 * 24]\n"
		"ldp	q26, q27, [%2, #16 * 26]\n"
		"ldp	q28, q29, [%2, #16 * 28]\n"
		"ldp	q30, q31, [%2, #16 * 30]\n"
		"msr	fpcr, %0\n"
		"msr	fpsr, %1\n"
		: : "r"(fpcr), "r"(fpsr), "r"(ptr));
}

static inline
void save_extregs(void *ectx)
{
	fpsimd_save_state((__uptr)ectx);

	/* make sure sysreg writing takes effects */
	uk_arch_arm64_isb();
}

static inline
void restore_extregs(void *ectx)
{
	fpsimd_restore_state((__uptr)ectx);

	/* make sure sysreg writing takes effects */
	uk_arch_arm64_isb();
}

#else /* !CONFIG_FPSIMD */

struct fpsimd_state { };

static inline void save_extregs(void *ectx __unused)
{
}

static inline void restore_extregs(void *ectx __unused)
{
}

#endif /* !CONFIG_FPSIMD */

__isr void uk_plat_native_ectx_sanitize(struct uk_plat_native_ectx *state __maybe_unused)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr)state, UK_PLAT_NATIVE_ECTX_ALIGN));
}

__isr void uk_plat_native_ectx_store(struct uk_plat_native_ectx *state)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr)state, UK_PLAT_NATIVE_ECTX_ALIGN));

	save_extregs(state);
}

__isr void uk_plat_native_ectx_load(struct uk_plat_native_ectx *state)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr)state, UK_PLAT_NATIVE_ECTX_ALIGN));

	restore_extregs(state);
}

__isr void uk_plat_native_ectx_init(struct uk_plat_native_ectx *state)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr)state, UK_PLAT_NATIVE_ECTX_ALIGN));

	/* Initialize extregs area:
	 * Zero out and then save a valid layout to it.
	 */
	memset_isr(state, 0, sizeof(struct fpsimd_state));
	uk_plat_native_ectx_store(state);
}

__isr void uk_plat_native_ectx_assert_equal(struct uk_plat_native_ectx *state)
{
	const __sz ectx_size = sizeof(struct fpsimd_state);
	__u8 ectxbuf[ectx_size + UK_PLAT_NATIVE_ECTX_ALIGN];
	struct uk_plat_native_ectx *current;

	/* Store the current state */
	current = (struct uk_plat_native_ectx *)
		  ALIGN_UP((__uptr)ectxbuf, UK_PLAT_NATIVE_ECTX_ALIGN);
	uk_plat_native_ectx_init(current);

	if (memcmp_isr(current, state, ectx_size) != 0) {
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
}
