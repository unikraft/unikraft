/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ARCH_H__
#define __UK_ARCH_H__

#include <uk/arch/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME: Hardcode for now. In practice this can be
 *        variable, define per platform instead.
 */
#define UK_ARCH_CACHE_LINE_SIZE		64

#if !__ASSEMBLY__

/**
 * Read/Write memory barrier
 */
void uk_arch_mb(void);

/**
 * Read memory barrier
 */
void uk_arch_rmb(void);

/**
 * Write memory barrier
 */
void uk_arch_wmb(void);

/**
 * Read stack pointer
 *
 * @return Current stack pointer value
 */
__u64 uk_arch_read_sp(void);

/**
 * Spin-wait hint
 */
void uk_arch_spinwait(void);

/**
 * Switch stack pointer to given value and jump to address.
 *
 * This function does not return.
 *
 * @param sp Stack pointer to switch to before jumping
 * @param addr The address to jump to
 */
void __noreturn uk_arch_jump_to(__u64 sp, __u64 addr);

/**
 * Switch stack pointer to given value, jump to address,
 * and pass a single argument to the target function.
 *
 * This function does not return.
 *
 * @param sp  Stack pointer to switch to before jumping
 * @param ip  The address to jump to
 * @param arg The argument to pass to the target function
 */
void __noreturn uk_arch_jump_to_with_arg(__u64 sp, __u64 ip, __u64 arg);

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_ARCH_H__ */
