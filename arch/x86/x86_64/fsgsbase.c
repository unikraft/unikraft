/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/lcpu.h>
#include <uk/ctors.h>
#include <x86/cpu.h>

static void wrmsrgsbase(__uptr gsbase)
{
	wrmsrl(X86_MSR_GS_BASE, gsbase);
}

static __uptr rdmsrgsbase(void)
{
	return rdmsrl(X86_MSR_GS_BASE);
}

static __uptr rdgsbase_cr4fsgsbase(void)
{
	__uptr gsbase;

	__asm__ __volatile__(
		"rdgsbase	%0"
		: "=r" (gsbase)
		:
		: "memory"
	);

	return gsbase;
}

static void wrgsbase_cr4fsgsbase(__uptr gsbase)
{
	__asm__ __volatile__(
		"wrgsbase	%0"
		:
		: "r" (gsbase)
		: "memory"
	);
}

static void wrmsrkgsbase(__uptr kgsbase)
{
	wrmsrl(X86_MSR_KERNEL_GS_BASE, kgsbase);
}

static __uptr rdmsrkgsbase(void)
{
	return rdmsrl(X86_MSR_KERNEL_GS_BASE);
}

static void wrmsrfsbase(__uptr fsbase)
{
	wrmsrl(X86_MSR_FS_BASE, fsbase);
}

static __uptr rdmsrfsbase(void)
{
	return rdmsrl(X86_MSR_FS_BASE);
}

static __uptr rdfsbase_cr4fsgsbase(void)
{
	__uptr fsbase;

	__asm__ __volatile__(
		"rdfsbase	%0"
		: "=r" (fsbase)
		:
		: "memory"
	);

	return fsbase;
}

static void wrfsbase_cr4fsgsbase(__uptr fsbase)
{
	__asm__ __volatile__(
		"wrfsbase	%0"
		:
		: "r" (fsbase)
		: "memory"
	);
}

void (*wrgsbasefn)(__uptr) = &wrmsrgsbase;
__uptr (*rdgsbasefn)(void) = &rdmsrgsbase;
void (*wrfsbasefn)(__uptr) = &wrmsrfsbase;
__uptr (*rdfsbasefn)(void) = &rdmsrfsbase;
void (*wrkgsbasefn)(__uptr) = &wrmsrkgsbase;
__uptr (*rdkgsbasefn)(void) = &rdmsrkgsbase;

static void init_fsgsbasefns(void)
{
	__u32 eax, ebx, ecx, edx;

	ukarch_x86_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
	if (ebx & X86_CPUID7_EBX_FSGSBASE) {
		wrgsbasefn = wrgsbase_cr4fsgsbase;
		rdgsbasefn = rdgsbase_cr4fsgsbase;
		wrfsbasefn = wrfsbase_cr4fsgsbase;
		rdfsbasefn = rdfsbase_cr4fsgsbase;
	}
}
UK_CTOR_PRIO(init_fsgsbasefns, 0);
