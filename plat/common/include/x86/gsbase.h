/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __PLAT_COMMON_X86_GSBASE_H__
#define __PLAT_COMMON_X86_GSBASE_H__

#include <x86/cpu.h>
#include <uk/essentials.h>

/* TODO: Use wrgsbase/rdgsbase instead, as they are faster */
static inline void wrgsbase(__uptr gsbase)
{
	wrmsrl(X86_MSR_GS_BASE, gsbase);
}

static inline __uptr rdgsbase(void)
{
	return rdmsrl(X86_MSR_GS_BASE);
}

static inline void wrkgsbase(__uptr kgsbase)
{
	wrmsrl(X86_MSR_KERNEL_GS_BASE, kgsbase);
}

static inline __uptr rdkgsbase(void)
{
	return rdmsrl(X86_MSR_KERNEL_GS_BASE);
}

static inline void wrgsbase8(__u8 val, __off offset)
{
	__asm__ __volatile__(
		"movb	%1, %%gs:%0\n"
		: "=m"(*(__off *)offset)
		: "r"(val)
	);
}

static inline void wrgsbase16(__u16 val, __off offset)
{
	__asm__ __volatile__(
		"movw	%1, %%gs:%0\n"
		: "=m"(*(__off *)offset)
		: "r"(val)
	);
}

static inline void wrgsbase32(__u32 val, __off offset)
{
	__asm__ __volatile__(
		"movl	%1, %%gs:%0\n"
		: "=m"(*(__off *)offset)
		: "r"(val)
	);
}

static inline void wrgsbase64(__u64 val, __off offset)
{
	__asm__ __volatile__(
		"movq	%1, %%gs:%0\n"
		: "=m"(*(__off *)offset)
		: "r"(val)
	);
}

static inline __u8 rdgsbase8(__off offset)
{
	__u8 val;

	__asm__ __volatile__(
		"movb	%%gs:%1, %0\n"
		: "=r"(val)
		: "m"(*(__off *)offset)
	);

	return val;
}

static inline __u16 rdgsbase16(__off offset)
{
	__u16 val;

	__asm__ __volatile__(
		"movw	%%gs:%1, %0\n"
		: "=r"(val)
		: "m"(*(__off *)offset)
	);

	return val;
}

static inline __u32 rdgsbase32(__off offset)
{
	__u32 val;

	__asm__ __volatile__(
		"movl	%%gs:%1, %0\n"
		: "=r"(val)
		: "m"(*(__off *)offset)
	);

	return val;
}

static inline __u64 rdgsbase64(__off offset)
{
	__u64 val;

	__asm__ __volatile__(
		"movq	%%gs:%1, %0\n"
		: "=r"(val)
		: "m"(*(__off *)offset)
	);

	return val;
}

#endif /* __PLAT_COMMON_X86_GSBASE_H__ */
