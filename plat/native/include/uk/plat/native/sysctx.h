/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_SYSCTX_H__
#define __UK_PLAT_NATIVE_SYSCTX_H__

/**
 * System Register Context - Architecture-Independent Interface
 *
 * Provides save/restore of system registers not part of general-purpose
 * register state, such as segment base registers, special control registers,
 * and the thread-local storage (TLS) pointer. Architecture-specific
 * implementations define which system registers are preserved.
 */

#include <uk/plat/native/arch/sysctx.h>

/*
 * Verify that architecture-specific header defined required constants.
 * TLSP (thread-local storage pointer) must be accessible as it's used
 * across all architectures for per-thread data.
 */

#ifndef UK_PLAT_NATIVE_SYSCTX_SIZE
#error "UK_PLAT_NATIVE_SYSCTX_SIZE undefined"
#endif

#ifndef UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP
#error "UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP undefined"
#endif

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
#ifndef UK_PLAT_NATIVE_SYSCTX_LOAD_FNSYM
#error "UK_PLAT_NATIVE_SYSCTX_LOAD_FNSYM undefined"
#endif

#ifndef UK_PLAT_NATIVE_SYSCTX_STORE_FNSYM
#error "UK_PLAT_NATIVE_SYSCTX_STORE_FNSYM undefined"
#endif
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read a system register value by byte offset.
 * Allows generic access when specific register is not known at compile time.
 *
 * @param sc      System context to read from
 * @param offset  Byte offset (use UK_PLAT_NATIVE_SYSCTX_OFFSETOF_*)
 * @return Register value as 64-bit unsigned integer
 */
__u64 uk_plat_native_sysctx_get(const struct uk_plat_native_sysctx *sc,
				__sz offset);

/**
 * Write a system register value by byte offset.
 * Allows generic modification when specific register is not known at compile
 * time.
 *
 * @param sc      System context to modify
 * @param offset  Byte offset (use UK_PLAT_NATIVE_SYSCTX_OFFSETOF_*)
 * @param val     New 64-bit value to write
 */
void uk_plat_native_sysctx_set(struct uk_plat_native_sysctx *sc,
			       __sz offset, __u64 val);

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
/**
 * Get current thread-local storage pointer.
 * Reads the architecture-specific register used for TLS.
 *
 * @return Current TLS pointer value
 */
__uptr uk_plat_native_tlsp_get(void);

/**
 * Set thread-local storage pointer.
 * Writes the architecture-specific register used for TLS.
 *
 * @param tlsp  New TLS pointer value (typically base of TLS area)
 */
void uk_plat_native_tlsp_set(__uptr tlsp);

/**
 * Save current CPU's system register context to memory.
 * Captures system registers (including TLS pointer) for later restoration.
 * ISR-safe.
 *
 * @param sysctx  Destination for saved context
 */
__isr void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx);

/**
 * Restore system register context from memory to CPU.
 * Loads previously saved system registers (including TLS pointer).
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
#endif /* __UK_PLAT_NATIVE_SYSCTX_H__ */
