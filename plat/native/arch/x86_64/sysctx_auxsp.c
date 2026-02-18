/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <string.h>

#include <uk/arch/ctx.h>
#include <uk/ctors.h>
#include <uk/arch.h>

static void uk_arch_wrmsrgsbase(__u64 gsbase)
{
	uk_arch_wrmsrl(UK_ARCH_MSR_GS_BASE, gsbase);
}

static __u64 uk_arch_rdmsrgsbase(void)
{
	return uk_arch_rdmsrl(UK_ARCH_MSR_GS_BASE);
}

static __u64 rdgsbase_cr4fsgsbase(void)
{
	__u64 gsbase;

	__asm__ __volatile__(
		"rdgsbase	%0"
		: "=r" (gsbase)
		:
		: "memory"
	);

	return gsbase;
}

static void wrgsbase_cr4fsgsbase(__u64 gsbase)
{
	__asm__ __volatile__(
		"wrgsbase	%0"
		:
		: "r" (gsbase)
		: "memory"
	);
}

static void uk_arch_wrmsrkgsbase(__u64 kgsbase)
{
	uk_arch_wrmsrl(UK_ARCH_MSR_KERNEL_GS_BASE, kgsbase);
}

static void uk_arch_wrmsrfsbase(__u64 fsbase)
{
	uk_arch_wrmsrl(UK_ARCH_MSR_FS_BASE, fsbase);
}

static __u64 uk_arch_rdmsrfsbase(void)
{
	return uk_arch_rdmsrl(UK_ARCH_MSR_FS_BASE);
}

static __u64 rdfsbase_cr4fsgsbase(void)
{
	__u64 fsbase;

	__asm__ __volatile__(
		"rdfsbase	%0"
		: "=r" (fsbase)
		:
		: "memory"
	);

	return fsbase;
}

static void wrfsbase_cr4fsgsbase(__u64 fsbase)
{
	__asm__ __volatile__(
		"wrfsbase	%0"
		:
		: "r" (fsbase)
		: "memory"
	);
}

void (*wrgsbasefn)(__u64) = &uk_arch_wrmsrgsbase;
__u64 (*rdgsbasefn)(void) = &uk_arch_rdmsrgsbase;
void (*uk_plat_native_wrfsbasefn)(__u64) = &uk_arch_wrmsrfsbase;
__u64 (*uk_plat_native_rdfsbasefn)(void) = &uk_arch_rdmsrfsbase;
static __maybe_unused void (*wrkgsbasefn)(__u64) = &uk_arch_wrmsrkgsbase;

static void init_fsgsbasefns(void)
{
	__u32 eax, ebx, ecx, edx;

	uk_arch_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
	if (ebx & UK_ARCH_CPUID7_EBX_FSGSBASE) {
		wrgsbasefn = wrgsbase_cr4fsgsbase;
		rdgsbasefn = rdgsbase_cr4fsgsbase;
		uk_plat_native_wrfsbasefn = wrfsbase_cr4fsgsbase;
		uk_plat_native_rdfsbasefn = rdfsbase_cr4fsgsbase;
	}
}

UK_CTOR_PRIO(init_fsgsbasefns, 0);

__isr void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	sysctx->gsbase = rdgsbasefn();
	sysctx->fsbase = uk_plat_native_rdfsbasefn();
}

__isr void uk_plat_native_sysctx_load(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	wrgsbasefn(sysctx->gsbase);
	uk_plat_native_wrfsbasefn(sysctx->fsbase);
}

#if CONFIG_LIBUKPLAT_NATIVE_AUXSP
__uk_pcpuvar __uptr uk_plat_native_auxsp;

void uk_plat_native_set_auxsp(__uptr auxsp)
{
	struct uk_plat_native_sysctx *sc;
	struct ukarch_auxspcb *auxspcb;

	uk_pcpuvar_current_set(uk_plat_native_auxsp, auxsp);
	auxspcb = ukarch_auxsp_get_cb(auxsp);
	sc = (struct uk_plat_native_sysctx *)auxspcb->uksysctx;
	sc->gsbase = rdgsbasefn();
}
#endif /* CONFIG_LIBUKPLAT_NATIVE_AUXSP */
