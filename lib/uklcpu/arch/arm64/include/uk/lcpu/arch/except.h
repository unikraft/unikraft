/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_H__
#error "Do not include this header directly"
#endif

#define UK_LCPU_ARM64_EXCEPT_ID_INVALID_OP			\
	UK_PAL_ARM64_EXCEPT_ID_INVALID_OP
#define UK_LCPU_ARM64_EXCEPT_ID_DEBUG				\
	UK_PAL_ARM64_EXCEPT_ID_DEBUG
#define UK_LCPU_ARM64_EXCEPT_ID_PAGE_FAULT			\
	UK_PAL_ARM64_EXCEPT_ID_PAGE_FAULT
#define UK_LCPU_ARM64_EXCEPT_ID_BUS_ERROR			\
	UK_PAL_ARM64_EXCEPT_ID_BUS_ERROR
#define UK_LCPU_ARM64_EXCEPT_ID_MATH				\
	UK_PAL_ARM64_EXCEPT_ID_MATH
#define UK_LCPU_ARM64_EXCEPT_ID_SECURITY			\
	UK_PAL_ARM64_EXCEPT_ID_SECURITY
#define UK_LCPU_ARM64_EXCEPT_ID_SYSCALL				\
	UK_PAL_ARM64_EXCEPT_ID_SYSCALL

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_lcpu_except_err_ctx;

__isr static inline int
uk_lcpu_arm64_except_err_ctx_get_eid(const struct uk_lcpu_except_err_ctx *ctx)
{
	return uk_pal_arm64_except_err_ctx_get_eid(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_arm64_except_err_ctx_set_eid(struct uk_lcpu_except_err_ctx *ctx,
				     int eid)
{
	uk_pal_arm64_except_err_ctx_set_eid(
		(struct uk_pal_except_err_ctx *)ctx, eid);
}

__isr static inline __u64
uk_lcpu_arm64_except_err_ctx_get_esr(const struct uk_lcpu_except_err_ctx *ctx)
{
	return uk_pal_arm64_except_err_ctx_get_esr(
		(const struct uk_pal_except_err_ctx *)ctx);
}

__isr static inline void
uk_lcpu_arm64_except_err_ctx_set_esr(struct uk_lcpu_except_err_ctx *ctx,
				     __u64 esr)
{
	uk_pal_arm64_except_err_ctx_set_esr(
		(struct uk_pal_except_err_ctx *)ctx, esr);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
