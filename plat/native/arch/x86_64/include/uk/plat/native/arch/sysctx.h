/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_SYSCTX_H__
#define __UK_PLAT_NATIVE_ARCH_SYSCTX_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>

/**
 * x86_64 system register context layout.
 * Contains segment base addresses (FS_BASE, GS_BASE) used for per-thread
 * and per-CPU data access. Offsets used by assembly code.
 */

/* Total size: 16 bytes (2 x 64-bit base addresses) */
#define UK_PLAT_NATIVE_SYSCTX_SIZE			16

/* MSR_GS_BASE offset (used for per-CPU data) */
#define UK_PLAT_NATIVE_X86_64_SYSCTX_OFFSETOF_GSBASE	0

/* MSR_FS_BASE offset (used for TLS pointer per AMD64 System V ABI) */
#define UK_PLAT_NATIVE_X86_64_SYSCTX_OFFSETOF_FSBASE	8

/* Architecture-neutral TLS pointer alias */
#define UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP				\
	UK_PLAT_NATIVE_X86_64_SYSCTX_OFFSETOF_FSBASE

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
/* Assembly-visible function symbols */
#define UK_PLAT_NATIVE_SYSCTX_LOAD_FNSYM				\
	uk_plat_native_sysctx_load
#define UK_PLAT_NATIVE_SYSCTX_STORE_FNSYM				\
	uk_plat_native_sysctx_store
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * x86_64 system register context.
 * Stores the base addresses of FS and GS segment registers, which are
 * used for efficient per-thread (TLS) and per-CPU data access without
 * requiring memory indirection.
 *
 * GS_BASE: Typically points to per-CPU data structure (kernel use).
 * FS_BASE: Typically points to thread-local storage (TLS per AMD64 ABI).
 *
 * Saved/restored on context switches, syscalls, and exceptions.
 */
struct uk_plat_native_sysctx {
	/*
	 * MSR_GS_BASE value.
	 * On syscall entry, holds the application's GS_BASE. Kernel typically
	 * uses GS for per-CPU data via SWAPGS instruction.
	 */
	__u64 gsbase;
	/*
	 * MSR_FS_BASE value (TLS pointer per AMD64 System V ABI).
	 * Points to thread-local storage area.
	 */
	__u64 fsbase;
};

/* Compile-time verification of sysctx layout */
UK_CTASSERT(sizeof(struct uk_plat_native_sysctx) == UK_PLAT_NATIVE_SYSCTX_SIZE);
UK_CTASSERT(__offsetof(struct uk_plat_native_sysctx, gsbase) ==
	    UK_PLAT_NATIVE_X86_64_SYSCTX_OFFSETOF_GSBASE);
UK_CTASSERT(__offsetof(struct uk_plat_native_sysctx, fsbase) ==
	    UK_PLAT_NATIVE_X86_64_SYSCTX_OFFSETOF_FSBASE);
UK_CTASSERT(__offsetof(struct uk_plat_native_sysctx, fsbase) ==
	    UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP);

/**
 * Read a system context register by byte offset.
 * Allows generic access to FS_BASE or GS_BASE when the specific register
 * is not known at compile time.
 *
 * @param sc      System context to read from
 * @param offset  Byte offset (use UK_PLAT_NATIVE_SYSCTX_OFFSETOF_*)
 * @return Register value as 64-bit unsigned integer
 */
__isr static inline __u64
uk_plat_native_sysctx_get(const struct uk_plat_native_sysctx *sc, __sz offset)
{
	return *(__u64 *)((const char *)sc + offset);
}

/**
 * Write a system context register by byte offset.
 * Allows generic modification of FS_BASE or GS_BASE.
 *
 * @param sc      System context to modify
 * @param offset  Byte offset (use UK_PLAT_NATIVE_SYSCTX_OFFSETOF_*)
 * @param val     New 64-bit value to write
 */
__isr static inline void
uk_plat_native_sysctx_set(struct uk_plat_native_sysctx *sc, __sz offset,
			  __u64 val)
{
	*(__u64 *)((char *)sc + offset) = val;
}

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
/**
 * Function pointers for FS_BASE register access.
 * Dynamically selected at boot based on CPU capabilities:
 *   - FSGSBASE instructions (RDFSBASE/WRFSBASE) if supported
 *   - MSR access (RDMSR/WRMSR) as fallback for older CPUs
 *
 * Using function pointers avoids runtime CPU feature checks on every access.
 */
extern void (*uk_plat_native_wrfsbasefn)(__u64);
extern __u64 (*uk_plat_native_rdfsbasefn)(void);

/**
 * Get current thread-local storage pointer.
 * Reads MSR_FS_BASE, which holds the TLS pointer per AMD64 System V ABI.
 *
 * @return Current TLS pointer value
 */
__isr static inline __uptr uk_plat_native_tlsp_get(void)
{
	return uk_plat_native_rdfsbasefn();
}

/**
 * Set thread-local storage pointer.
 * Writes MSR_FS_BASE to configure TLS for the current thread.
 *
 * @param tlsp  New TLS pointer value (typically base of TLS area)
 */
__isr static inline void uk_plat_native_tlsp_set(__uptr tlsp)
{
	uk_plat_native_wrfsbasefn(tlsp);
}

/**
 * Save current CPU's system context (FS_BASE and GS_BASE) to memory.
 * Reads both MSRs and stores them to the provided structure.
 * ISR-safe.
 *
 * @param sysctx  Destination for saved context
 */
__isr void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx);

/**
 * Restore system context (FS_BASE and GS_BASE) from memory to CPU.
 * Writes both MSRs from the provided structure.
 * ISR-safe.
 *
 * @param sysctx  System context to restore
 */
__isr void uk_plat_native_sysctx_load(struct uk_plat_native_sysctx *sysctx);
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_SYSCTX_H__ */
