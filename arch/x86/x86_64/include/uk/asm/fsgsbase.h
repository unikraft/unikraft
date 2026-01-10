/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_FSGSBASE_H__
#define __UKARCH_FSGSBASE_H__

#if !__ASSEMBLY__

#include <uk/arch/util.h>
#include <uk/essentials.h>
#include <uk/print.h>

extern void (*wrgsbasefn)(__uptr gsbase);
extern __uptr (*rdgsbasefn)(void);

static inline void wrgsbase(__uptr gsbase)
{
	wrgsbasefn(gsbase);
}

static inline __uptr rdgsbase(void)
{
	return rdgsbasefn();
}

extern void (*wrkgsbasefn)(__uptr kgsbase);
extern __uptr (*rdkgsbasefn)(void);

static inline void wrkgsbase(__uptr kgsbase)
{
	wrkgsbasefn(kgsbase);
}

static inline __uptr rdkgsbase(void)
{
	return rdkgsbasefn();
}

extern void (*wrfsbasefn)(__uptr fsbase);
extern __uptr (*rdfsbasefn)(void);

static inline void wrfsbase(__uptr fsbase)
{
	wrfsbasefn(fsbase);
}

static inline __uptr rdfsbase(void)
{
	return rdfsbasefn();
}
#endif /* !__ASSEMBLY__ */
#endif /* __UKARCH_FSGSBASE_H__ */
