/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_FSGSBASE_H__
#define __UKARCH_FSGSBASE_H__

#if !__ASSEMBLY__

#include <uk/essentials.h>
#include <uk/print.h>

extern void (*wrgsbasefn)(__uptr gsbase);
extern __uptr (*rdgsbasefn)(void);

static inline void wrgsbase(__uptr gsbase)
{
	wrgsbasefn(gsbase);
}

static inline __uptr rdgsbase(void)
{
	return rdgsbasefn();
}

extern void (*wrkgsbasefn)(__uptr kgsbase);
extern __uptr (*rdkgsbasefn)(void);

static inline void wrkgsbase(__uptr kgsbase)
{
	wrkgsbasefn(kgsbase);
}

static inline __uptr rdkgsbase(void)
{
	return rdkgsbasefn();
}

static inline void wrgsbase8(__u8 val, __off offset)
{
	__asm__ __volatile__(
		"movb	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline void wrgsbase16(__u16 val, __off offset)
{
	__asm__ __volatile__(
		"movw	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline void wrgsbase32(__u32 val, __off offset)
{
	__asm__ __volatile__(
		"movl	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline void wrgsbase64(__u64 val, __off offset)
{
	__asm__ __volatile__(
		"movq	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline __u8 rdgsbase8(__off offset)
{
	__u8 val;

	__asm__ __volatile__(
		"movb	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

static inline __u16 rdgsbase16(__off offset)
{
	__u16 val;

	__asm__ __volatile__(
		"movw	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

static inline __u32 rdgsbase32(__off offset)
{
	__u32 val;

	__asm__ __volatile__(
		"movl	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

static inline __u64 rdgsbase64(__off offset)
{
	__u64 val;

	__asm__ __volatile__(
		"movq	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

extern void (*wrfsbasefn)(__uptr fsbase);
extern __uptr (*rdfsbasefn)(void);

static inline void wrfsbase(__uptr fsbase)
{
	wrfsbasefn(fsbase);
}

static inline __uptr rdfsbase(void)
{
	return rdfsbasefn();
}

static inline void wrfsbase8(__u8 val, __off offset)
{
	__asm__ __volatile__(
		"movb	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline void wrfsbase16(__u16 val, __off offset)
{
	__asm__ __volatile__(
		"movw	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline void wrfsbase32(__u32 val, __off offset)
{
	__asm__ __volatile__(
		"movl	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline void wrfsbase64(__u64 val, __off offset)
{
	__asm__ __volatile__(
		"movq	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

static inline __u8 rdfsbase8(__off offset)
{
	__u8 val;

	__asm__ __volatile__(
		"movb	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

static inline __u16 rdfsbase16(__off offset)
{
	__u16 val;

	__asm__ __volatile__(
		"movw	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

static inline __u32 rdfsbase32(__off offset)
{
	__u32 val;

	__asm__ __volatile__(
		"movl	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

static inline __u64 rdfsbase64(__off offset)
{
	__u64 val;

	__asm__ __volatile__(
		"movq	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}
#endif /* !__ASSEMBLY__ */
#endif /* __UKARCH_FSGSBASE_H__ */
