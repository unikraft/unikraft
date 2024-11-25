/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_CFI_H__
#define __UK_CFI_H__

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKCFI_PAUTH
#if CONFIG_LIBUKCFI_BTI
#define __no_pauth __attribute__((target("branch-protection=bti")))
#else
#define __no_pauth __attribute__((target("branch-protection=none")))
#endif /* CONFIG_LIBUKCFI_BTI */
#else
#define __no_pauth
#endif /* CONFIG_LIBUKCFI_PAUTH */

#if CONFIG_LIBUKCFI_BTI
#if CONFIG_LIBUKCFI_PAUTH
#define __no_bti __attribute__((target("branch-protection=pac-ret+leaf")))
#else
#define __no_bti __attribute__((target("branch-protection=none")))
#endif /* CONFIG_LIBUKCFI_PAUTH */
#else
#define __no_bti
#endif /* CONFIG_LIBUKCFI_BTI */

#if defined(CONFIG_LIBUKCFI_PAUTH) || defined(CONFIG_LIBUKCFI_BTI)
#define __no_branch_protection __attribute__((target("branch-protection=none")))
#else
#define __no_branch_protection
#endif /* defined(CONFIG_LIBUKCFI_PAUTH) || defined(CONFIG_LIBUKCFI_BTI) */

#ifdef __cplusplus
}
#endif

#endif /* __UK_CFI_H__ */
