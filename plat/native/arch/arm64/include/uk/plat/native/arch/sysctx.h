/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_SYSCTX_H__
#define __UK_PLAT_NATIVE_ARCH_SYSCTX_H__

#include <uk/arch.h>
#include <uk/arch/types.h>
#include <uk/config.h>

#define UK_PLAT_NATIVE_ARM64_SYSCTX_OFFSETOF_TPIDR_EL0		0

#define UK_PLAT_NATIVE_SYSCTX_SIZE				16
#define UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP			\
	UK_PLAT_NATIVE_ARM64_SYSCTX_OFFSETOF_TPIDR_EL0

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
/* Assembly-visible function symbols */
#define UK_PLAT_NATIVE_SYSCTX_LOAD_FNSYM			\
	uk_plat_native_sysctx_load
#define UK_PLAT_NATIVE_SYSCTX_STORE_FNSYM			\
	uk_plat_native_sysctx_store
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* --- System Context --- */
struct uk_plat_native_sysctx {
	__uptr tpidr_el0; /* TLS pointer (AAPCS) */
	__u8 pad[8]; /* 16-byte stack alignment */
};

UK_CTASSERT(sizeof(struct uk_plat_native_sysctx) == UK_PLAT_NATIVE_SYSCTX_SIZE);
UK_CTASSERT(__offsetof(struct uk_plat_native_sysctx, tpidr_el0) ==
	    UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP);

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
/**
 * Get current thread-local storage pointer.
 *
 * @return Current TLS pointer value
 */
__isr static inline __uptr uk_plat_native_tlsp_get(void)
{
	return UK_ARCH_ARM64_SYSREG_READ(tpidr_el0);
}

/**
 * Set thread-local storage pointer.
 *
 * @param tlsp  New TLS pointer value (typically base of TLS area)
 */
__isr static inline void uk_plat_native_tlsp_set(__uptr tlsp)
{
	UK_ARCH_ARM64_SYSREG_WRITE(tpidr_el0, tlsp);
}

/**
 * Save current CPU's system context to memory.
 *
 * @param sysctx  Destination for saved context
 */
__isr void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx);

/**
 * Restore system context from memory to CPU.
 *
 * @param sysctx  System context to restore
 */
__isr void uk_plat_native_sysctx_load(struct uk_plat_native_sysctx *sysctx);
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

/**
 * Read a system context register by byte offset.
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

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_SYSCTX_H__ */
