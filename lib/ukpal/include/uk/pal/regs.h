/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_REGS_H__
#define __UK_PAL_REGS_H__

/**
 * CPU Register Context - Platform Abstraction Layer
 *
 * Provides access to CPU register state captured during interrupts,
 * exceptions, or context switches. Platform and architecture specific.
 */

#include <uk/arch/types.h>
#include <uk/pal/arch/regs.h>

/*
 * Platform must define offsets for stack pointer (SP) and program counter
 * (PC). These are the only architecture-neutral register aliases required
 * across all platforms.
 */

#ifndef UK_PAL_REGS_OFFSETOF_SP
#error "UK_PAL_REGS_OFFSETOF_SP undefined"
#endif

#ifndef UK_PAL_REGS_OFFSETOF_PC
#error "UK_PAL_REGS_OFFSETOF_PC undefined"
#endif

#ifndef UK_PAL_REGS_SIZE
#error "UK_PAL_REGS_SIZE undefined"
#endif

/*
 * Register context must be 16-byte aligned for proper stack alignment.
 */
#if UK_PAL_REGS_SIZE & 0xf
#error "uk_pal_regs structure size should be multiple of 16."
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * CPU register context (opaque).
 * Platform and architecture specific register state.
 */
struct uk_pal_regs;

/**
 * Read a register value by offset.
 *
 * @param regs    Register context
 * @param offset  Byte offset of register
 * @return Register value
 */
__isr __u64 uk_pal_regs_get(const struct uk_pal_regs *regs, __sz offset);

/**
 * Write a register value by offset.
 *
 * @param regs    Register context
 * @param offset  Byte offset of register
 * @param val     New register value
 */
__isr void uk_pal_regs_set(struct uk_pal_regs *regs, __sz offset, __u64 val);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_REGS_H__ */
