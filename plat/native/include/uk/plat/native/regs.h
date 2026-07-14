/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_REGS_H__
#define __UK_PLAT_NATIVE_REGS_H__

/**
 * CPU Register Context - Architecture-Independent Interface
 *
 * Provides access to CPU general-purpose register state captured during
 * interrupts, exceptions, or context switches. Architecture-specific
 * implementations define the exact register layout.
 */

#include <uk/plat/native/arch/regs.h>

/*
 * Verify that architecture-specific header defined required constants.
 * SP and PC are architecture-neutral names for stack pointer and program
 * counter, which must be accessible across all architectures.
 */

#ifndef UK_PLAT_NATIVE_REGS_OFFSETOF_SP
#error "UK_PLAT_NATIVE_REGS_OFFSETOF_SP undefined"
#endif

#ifndef UK_PLAT_NATIVE_REGS_OFFSETOF_PC
#error "UK_PLAT_NATIVE_REGS_OFFSETOF_PC undefined"
#endif

#ifndef UK_PLAT_NATIVE_REGS_SIZE
#error "UK_PLAT_NATIVE_REGS_SIZE undefined"
#endif

/*
 * Register context size must be 16-byte aligned for proper stack alignment
 * and compatibility with vector save/restore instructions.
 */
#if UK_PLAT_NATIVE_REGS_SIZE & 0xf
#error "uk_plat_native_regs structure size should be multiple of 16."
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read a register value by byte offset.
 * Allows generic access when specific register is not known at compile time.
 *
 * @param regs    Register context to read from
 * @param offset  Byte offset of register (use UK_PLAT_NATIVE_REGS_OFFSETOF_*)
 * @return Register value as 64-bit unsigned integer
 */
__u64 uk_plat_native_regs_get(const struct uk_plat_native_regs *regs,
			      __sz offset);

/**
 * Write a register value by byte offset.
 * Allows generic modification when specific register is not known at compile
 * time.
 *
 * @param regs    Register context to modify
 * @param offset  Byte offset of register (use UK_PLAT_NATIVE_REGS_OFFSETOF_*)
 * @param val     New 64-bit value to write
 */
void uk_plat_native_regs_set(struct uk_plat_native_regs *regs,
			     __sz offset, __u64 val);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_REGS_H__ */
