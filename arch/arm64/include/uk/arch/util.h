/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018, Arm Ltd.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ARCH_UTIL_H__
#define __UK_ARCH_UTIL_H__

#include <uk/arch/arm64.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

/* Macros to access system registers */
#define UK_ARCH_ARM64_SYSREG_READ(reg)				\
({	__u64 val;						\
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg)	\
			: "=r" (val));				\
	val;							\
})

#define UK_ARCH_ARM64_SYSREG_WRITE(reg, val)			\
({	__asm__ __volatile__("msr " __STRINGIFY(reg) ", %0"	\
			: : "r" ((__u64)(val)));		\
})

#define UK_ARCH_ARM64_SYSREG_READ32(reg)			\
({	__u64 val;						\
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg)	\
			: "=r" (val));				\
	val;							\
})

#define UK_ARCH_ARM64_SYSREG_WRITE32(reg, val)			\
({	__asm__ __volatile__("msr " __STRINGIFY(reg) ", %0"	\
			: : "r" ((__u32)(val)));		\
})

#define UK_ARCH_ARM64_SYSREG_READ64(reg)			\
	UK_ARCH_ARM64_SYSREG_READ(reg)

#define UK_ARCH_ARM64_SYSREG_WRITE64(reg, val)			\
	UK_ARCH_ARM64_SYSREG_WRITE(reg, val)

/*
 * we should use inline assembly with volatile constraint to access mmio
 * device memory to avoid compiler use load/store instructions of writeback
 * addressing mode which will cause crash when running in hyper mode
 * unless they will be decoded by hypervisor.
 */
static inline
__u8 uk_arch_arm64_ioreg_read8(const volatile __u8 *address)
{
	__u8 value;

	__asm__ __volatile__("ldrb %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline
__u16 uk_arch_arm64_ioreg_read16(const volatile __u16 *address)
{
	__u16 value;

	__asm__ __volatile__("ldrh %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline
__u32 uk_arch_arm64_ioreg_read32(const volatile __u32 *address)
{
	__u32 value;

	__asm__ __volatile__("ldr %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline
__u64 uk_arch_arm64_ioreg_read64(const volatile __u64 *address)
{
	__u64 value;

	__asm__ __volatile__("ldr %0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline
void uk_arch_arm64_ioreg_write8(const volatile __u8 *address, __u8 value)
{
	__asm__ __volatile__("strb %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline
void uk_arch_arm64_ioreg_write16(const volatile __u16 *address,
				 __u16 value)
{
	__asm__ __volatile__("strh %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline
void uk_arch_arm64_ioreg_write32(const volatile __u32 *address,
				 __u32 value)
{
	__asm__ __volatile__("str %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline
void uk_arch_arm64_ioreg_write64(const volatile __u64 *address,
				 __u64 value)
{
	__asm__ __volatile__("str %0, [%1]" : : "rZ"(value), "r"(address));
}

/*
 * Instruction Synchronization Barrier flushes the pipeline in the
 * processor, so that all instructions following the ISB are fetched
 * from cache or memory, after the instruction has been completed.
 */
static inline void uk_arch_arm64_isb(void)
{
	__asm__ __volatile__("isb" ::: "memory");
}

/*
 * Options for DMB and DSB:
 *	oshld	Outer Shareable, load
 *	oshst	Outer Shareable, store
 *	osh	Outer Shareable, all
 *	nshld	Non-shareable, load
 *	nshst	Non-shareable, store
 *	nsh	Non-shareable, all
 *	ishld	Inner Shareable, load
 *	ishst	Inner Shareable, store
 *	ish	Inner Shareable, all
 *	ld	Full system, load
 *	st	Full system, store
 *	sy	Full system, all
 */
#define uk_arch_arm64_dmb(opt)						\
		__asm__ __volatile__ ("dmb " #opt ::: "memory")

#define uk_arch_arm64_dsb(opt)						\
		__asm__ __volatile__ ("dsb " #opt ::: "memory")

static inline
void uk_arch_arm64_mb(void)
{
	uk_arch_arm64_dsb(sy);
}

static inline
void uk_arch_arm64_rmb(void)
{
	uk_arch_arm64_dsb(ld);
}

static inline
void uk_arch_arm64_wmb(void)
{
	uk_arch_arm64_dsb(st);
}

static inline
__u64 uk_arch_arm64_read_sp(void)
{
	__u64 sp;

	__asm__ __volatile__ ("mov %0, sp" : "=&r"(sp));

	return sp;
}

static inline
void uk_arch_arm64_spinwait(void)
{
	/* Intelligent busy wait not supported on arm64. */
}

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_ARCH_UTIL_H__ */
