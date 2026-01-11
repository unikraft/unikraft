/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_H__
#error "Do not include this header directly"
#endif

#define UK_LCPU_X86_64_EXCEPT_EVENT_ERR_GP_FAULT			\
	UK_PAL_X86_64_EXCEPT_EVENT_ERR_GP_FAULT
#define UK_LCPU_X86_64_EXCEPT_EVENT_NMI					\
	UK_PAL_X86_64_EXCEPT_EVENT_NMI

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_lcpu_except_err_ctx;

__isr static inline __u32
uk_lcpu_x86_64_except_err_ctx_get_trapnr(
		const struct uk_lcpu_except_err_ctx *ctx)
{
	return uk_pal_x86_64_except_err_ctx_get_trapnr(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_x86_64_except_err_ctx_set_trapnr(struct uk_lcpu_except_err_ctx *ctx,
					 __u32 trapnr)
{
	uk_pal_x86_64_except_err_ctx_set_trapnr(
		(struct uk_pal_except_err_ctx *)ctx, trapnr);
}

__isr static inline int
uk_lcpu_x86_64_except_err_ctx_get_error_code(
		const struct uk_lcpu_except_err_ctx *ctx)
{
	return uk_pal_x86_64_except_err_ctx_get_error_code(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_x86_64_except_err_ctx_set_error_code(struct uk_lcpu_except_err_ctx *ctx,
					     int error_code)
{
	uk_pal_x86_64_except_err_ctx_set_error_code(
		(struct uk_pal_except_err_ctx *)ctx, error_code);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
