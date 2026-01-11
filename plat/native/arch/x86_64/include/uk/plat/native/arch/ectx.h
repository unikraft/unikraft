/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_ECTX_H__
#define __UK_PLAT_NATIVE_ARCH_ECTX_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>

/**
 * x86_64 XSAVE Area Layout
 *
 * Extended context managed by XSAVE/XRSTOR instructions, containing
 * floating-point and vector register state.
 *
 * Component breakdown:
 *   - Legacy x87 FPU:             160 bytes (ST0-ST7, control/status)
 *   - Legacy SSE:                 352 bytes (XMM0-XMM15, MXCSR)
 *   - XSAVE Header:                64 bytes (XSTATE_BV bitmap, must be zeroed!)
 *   - AVX (YMM_H):                256 bytes (upper 128 bits of YMM0-YMM15)
 *   - MPX_BNDREGS/BNDCSR:         128 bytes (not enabled)
 *   - AVX-512 (KMASK/ZMM_H/ZMM): 1600 bytes (not enabled)
 *
 * Current size: 160 + 352 + 64 + 256 = 832 bytes
 *
 * NOTE: Actual size queried via CPUID.0Dh at runtime may be smaller (e.g.,
 * 576 bytes without AVX). Increase UK_PLAT_NATIVE_ECTX_SIZE if enabling
 * additional features.
 */

/* Maximum size for currently enabled XSAVE components */
#define UK_PLAT_NATIVE_ECTX_SIZE			832

/* XSAVE/XRSTOR required alignment */
#define UK_PLAT_NATIVE_ECTX_ALIGN			64

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
/* Assembly-visible function symbols */
#define UK_PLAT_NATIVE_ECTX_LOAD_FNSYM					\
	uk_plat_native_ectx_load
#define UK_PLAT_NATIVE_ECTX_STORE_FNSYM					\
	uk_plat_native_ectx_store
#define UK_PLAT_NATIVE_ECTX_SANITIZE_FNSYM				\
	uk_plat_native_ectx_sanitize
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
/**
 * x86_64 extended context (opaque XSAVE area).
 * Must be UK_PLAT_NATIVE_ECTX_SIZE bytes, 64-byte aligned.
 */
struct uk_plat_native_ectx;

/**
 * Sanitize XSAVE header to prevent #GP on first XRSTOR.
 * Zeroes XSTATE_BV/XCOMP_BV at offset 512. Must be called on newly
 * allocated memory before any save/restore operations.
 *
 * @param state  64-byte aligned ectx memory to sanitize
 */
__isr void uk_plat_native_ectx_sanitize(struct uk_plat_native_ectx *state);

/**
 * Save CPU extended state (FPU/SSE/AVX) to memory.
 * Uses XSAVEOPT/XSAVE/FXSAVE/FSAVE depending on CPU features.
 * RFBM = 0xFFFFFFFF_FFFFFFFF (save all enabled components).
 *
 * @param state  64-byte aligned destination (must be sanitized)
 */
__isr void uk_plat_native_ectx_store(struct uk_plat_native_ectx *state);

/**
 * Restore extended state from memory to CPU.
 * Uses XRSTOR/FXRSTOR/FRSTOR (matches save method).
 * RFBM = 0xFFFFFFFF_FFFFFFFF, components restored per XSTATE_BV bitmap.
 *
 * WARNING: Corrupted XSAVE data (invalid XSTATE_BV) triggers #GP.
 *
 * @param state  64-byte aligned ectx with valid saved data
 */
__isr void uk_plat_native_ectx_load(struct uk_plat_native_ectx *state);

/**
 * Initialize ectx to clean default state.
 * Zeroes memory then saves current (clean) CPU state: x87 FPU reset,
 * MXCSR = 0x1F80, all vector registers zeroed.
 *
 * @param state  64-byte aligned ectx to initialize
 */
__isr void uk_plat_native_ectx_init(struct uk_plat_native_ectx *state);

/**
 * Assert ectx matches current CPU state (debug only).
 * Crashes with hexdump if mismatch detected. Accounts for XSTATE_BV
 * semantics (components in "initial configuration" compared for equivalence).
 *
 * WARNING: Significant performance cost.
 *
 * @param state  ectx to compare against current CPU state
 */
__isr void uk_plat_native_ectx_assert_equal(struct uk_plat_native_ectx *state);
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_ECTX_H__ */
