/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_EXCEPT_H__
#error "Do not include this header directly"
#endif

#ifndef UK_PAL_ARM64_EXCEPT_ID_INVALID_OP
#error "UK_PAL_ARM64_EXCEPT_ID_INVALID_OP undefined"
#endif
#ifndef UK_PAL_ARM64_EXCEPT_ID_DEBUG
#error "UK_PAL_ARM64_EXCEPT_ID_DEBUG undefined"
#endif
#ifndef UK_PAL_ARM64_EXCEPT_ID_PAGE_FAULT
#error "UK_PAL_ARM64_EXCEPT_ID_PAGE_FAULT undefined"
#endif
#ifndef UK_PAL_ARM64_EXCEPT_ID_BUS_ERROR
#error "UK_PAL_ARM64_EXCEPT_ID_BUS_ERROR undefined"
#endif
#ifndef UK_PAL_ARM64_EXCEPT_ID_MATH
#error "UK_PAL_ARM64_EXCEPT_ID_MATH undefined"
#endif
#ifndef UK_PAL_ARM64_EXCEPT_ID_SECURITY
#error "UK_PAL_ARM64_EXCEPT_ID_SECURITY undefined"
#endif
#ifndef UK_PAL_ARM64_EXCEPT_ID_SYSCALL
#error "UK_PAL_ARM64_EXCEPT_ID_SYSCALL undefined"
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

__isr int uk_pal_arm64_except_err_ctx_get_eid(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_arm64_except_err_ctx_set_eid(
	struct uk_pal_except_err_ctx *ctx, int eid);

__isr __u64 uk_pal_arm64_except_err_ctx_get_esr(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_arm64_except_err_ctx_set_esr(
	struct uk_pal_except_err_ctx *ctx, __u64 esr);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
