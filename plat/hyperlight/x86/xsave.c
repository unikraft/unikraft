/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Enable AVX on first use.
 *
 * lcpu_start64 leaves CR4.OSXSAVE clear, so CPUID reports OSXSAVE clear
 * and programs that check for AVX properly keep to SSE, and the extended
 * context store uses FXSAVE.  On a nested
 * AMD host, a guest with OSXSAVE set makes every VM exit slower, which
 * programs that would never use AVX should not pay for.
 *
 * Programs built for x86-64-v3 (e.g. glibc's libc.a on Ubuntu 26.04)
 * use AVX2 without checking CPUID and fault with #UD.  The first #UD on
 * a VEX instruction enables XSAVE and AVX, and on an EVEX one (or any
 * later one, for the VEX opmask instructions) AVX-512 as well when all
 * its components are there; the context store switches to XSAVE and the
 * instruction runs again.  Anything else is left to the
 * next handler, which raises SIGILL.
 */

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/arch/x86_64.h>
#include <uk/essentials.h>
#include <uk/event.h>
#include <uk/lcpu.h>
#include <uk/plat/native/ectx.h>

#include <hyperlight/dispatch.h>

#define AVX512_XCR0	(UK_ARCH_X86_64_XCR0_AVX512_OPMASK |		\
			 UK_ARCH_X86_64_XCR0_AVX512_ZMM_HI256 |		\
			 UK_ARCH_X86_64_XCR0_AVX512_HI16_ZMM)

/* Longest run of prefixes an instruction can carry (15 - 1) */
#define MAX_PREFIXES	14

#define VEX3		0xc4	/* LES in 32-bit mode only */
#define VEX2		0xc5	/* LDS in 32-bit mode only */
#define EVEX		0x62	/* BOUND in 32-bit mode only */

/*
 * The VEX or EVEX byte the instruction at @ip starts with, or 0.  Only
 * segment overrides and the address-size prefix may come before it:
 * after 66, F2, F3, F0 or REX the instruction is #UD whatever XCR0
 * says, so enabling AVX would not make it run.
 */
static __u8 vex_or_evex(const __u8 *ip)
{
	int i;

	for (i = 0; i < MAX_PREFIXES; i++) {
		switch (ip[i]) {
		case 0x26: case 0x2e: case 0x36: case 0x3e:
		case 0x64: case 0x65: case 0x67:
			continue;
		case VEX3: case VEX2: case EVEX:
			return ip[i];
		default:
			return 0;
		}
	}
	return 0;
}

static int hyperlight_xsave_ud_handler(void *arg)
{
	struct uk_lcpu_except_err_ctx *ctx = arg;
	__u32 eax, ebx, ecx, edx, xcr0_supported;
	__bool want_avx512;
	__u64 cr4, xcr0;
	__u8 kind;

	/* Before CPUID: on a nested host each one is a costly VM exit */
	kind = vex_or_evex((const __u8 *)uk_lcpu_regs_get(
		uk_lcpu_except_err_ctx_get_regs(ctx), PC));
	if (!kind)
		return UK_EVENT_NOT_HANDLED;

	/*
	 * A VEX instruction enables AVX; AVX-512 waits for an EVEX one, as
	 * its larger state makes every system call's save dearer.  The
	 * AVX-512 opmask instructions (kmovw, kxnorb, ...) are VEX, so any
	 * #UD once AVX is on may want AVX-512 too.
	 */
	__asm__ volatile("movq %%cr4, %0" : "=r"(cr4));
	want_avx512 = kind == EVEX;
	if (g_xcr0 & UK_ARCH_X86_64_XCR0_AVX) {
		if (g_xcr0 & AVX512_XCR0)
			return UK_EVENT_NOT_HANDLED;
		want_avx512 = __true;
	}

	uk_arch_x86_64_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (!(ecx & UK_ARCH_X86_64_CPUID1_ECX_XSAVE) ||
	    !(ecx & UK_ARCH_X86_64_CPUID1_ECX_AVX))
		return UK_EVENT_NOT_HANDLED;

	/* Only ask XSETBV for components CPUID 0xD says it supports */
	uk_arch_x86_64_cpuid(0xd, 0, &xcr0_supported, &ebx, &ecx, &edx);
	if (!(xcr0_supported & UK_ARCH_X86_64_XCR0_AVX))
		return UK_EVENT_NOT_HANDLED;

	xcr0 = UK_ARCH_X86_64_XCR0_X87 | UK_ARCH_X86_64_XCR0_SSE |
	       UK_ARCH_X86_64_XCR0_AVX;
	if (want_avx512) {
		uk_arch_x86_64_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
		if (!(ebx & UK_ARCH_X86_64_CPUID7_EBX_AVX512F) ||
		    (xcr0_supported & AVX512_XCR0) != AVX512_XCR0)
			return UK_EVENT_NOT_HANDLED;
		xcr0 |= AVX512_XCR0;
	}

	__asm__ volatile("movq %0, %%cr4"
			 :: "r"(cr4 | UK_ARCH_X86_64_CR4_OSXSAVE));
	__asm__ volatile("xsetbv"
			 :: "a"((__u32)xcr0), "d"((__u32)(xcr0 >> 32)),
			    "c"(0));
	g_xcr0 = xcr0;

	uk_plat_native_ectx_reinit();

	return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_INVALID_OP,
		      hyperlight_xsave_ud_handler, UK_PRIO_EARLIEST);
