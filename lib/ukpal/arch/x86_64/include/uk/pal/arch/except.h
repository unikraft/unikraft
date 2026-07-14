/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_EXCEPT_H__
#error "Do not include this header directly"
#endif

#ifndef UK_PAL_X86_64_EXCEPT_EVENT_ERR_GP_FAULT
#error "UK_PAL_X86_64_EXCEPT_EVENT_ERR_GP_FAULT undefined"
#endif

#ifndef UK_PAL_X86_64_EXCEPT_EVENT_NMI
#error "UK_PAL_X86_64_EXCEPT_EVENT_NMI undefined"
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

__isr __u32 uk_pal_x86_64_except_err_ctx_get_trapnr(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_x86_64_except_err_ctx_set_trapnr(
	struct uk_pal_except_err_ctx *ctx,
	__u32 trapnr);

__isr __u64 uk_pal_except_err_ctx_get_error_code(
	const struct uk_pal_except_err_ctx *ctx);

__isr void uk_pal_except_err_ctx_set_error_code(
	struct uk_pal_except_err_ctx *ctx,
	__u64 error_code);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
