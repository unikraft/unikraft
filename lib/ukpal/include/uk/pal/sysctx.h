/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_SYSCTX_H__
#define __UK_PAL_SYSCTX_H__

/**
 * System Register Context - Platform Abstraction Layer
 *
 * Manages system registers not part of general-purpose register state,
 * including the thread-local storage pointer and platform-specific control
 * registers. Platform and architecture specific.
 */

#include <uk/arch/types.h>
#include <uk/pal/arch/sysctx.h>

/*
 * Platform must define size, TLS pointer offset, and function symbols.
 * TLS pointer is the only universally required system register.
 */

#ifndef UK_PAL_SYSCTX_SIZE
#error "UK_PAL_SYSCTX_SIZE undefined"
#endif

#ifndef UK_PAL_SYSCTX_OFFSETOF_TLSP
#error "UK_PAL_SYSCTX_OFFSETOF_TLSP undefined"
#endif

#ifndef UK_PAL_SYSCTX_LOAD_FNSYM
#error "UK_PAL_SYSCTX_LOAD_FNSYM undefined"
#endif

#ifndef UK_PAL_SYSCTX_STORE_FNSYM
#error "UK_PAL_SYSCTX_STORE_FNSYM undefined"
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * System register context (opaque).
 * Platform and architecture specific system register state.
 */
struct uk_pal_sysctx;

/**
 * Read a system register by offset.
 *
 * @param sc      System context
 * @param offset  Byte offset of register
 * @return Register value
 */
__isr __u64 uk_pal_sysctx_get(const struct uk_pal_sysctx *sc, __sz offset);

/**
 * Write a system register by offset.
 *
 * @param sc      System context
 * @param offset  Byte offset of register
 * @param val     New register value
 */
__isr void uk_pal_sysctx_set(struct uk_pal_sysctx *sc, __sz offset, __u64 val);

/**
 * Save current system register state to memory.
 *
 * @param sysctx  Destination for saved context
 */
__isr void uk_pal_sysctx_store(struct uk_pal_sysctx *sysctx);

/**
 * Restore system register state from memory.
 *
 * @param sysctx  System context to restore
 */
__isr void uk_pal_sysctx_load(struct uk_pal_sysctx *sysctx);

/**
 * Get current thread-local storage pointer.
 *
 * @return TLS pointer value
 */
__isr __uptr uk_pal_tlsp_get(void);

/**
 * Set thread-local storage pointer.
 *
 * @param tlsp  New TLS pointer value
 */
__isr void uk_pal_tlsp_set(__uptr tlsp);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_SYSCTX_H__ */
