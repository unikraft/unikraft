/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_START_H__
#define __UK_PLAT_PAL_START_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/start.h>

#define UK_PAL_SENTRY_SYM						\
	UK_PLAT_NATIVE_SENTRY_SYM
#define UK_PAL_SSTACKP_SYM						\
	UK_PLAT_NATIVE_SSTACKP_SYM
#define UK_PAL_SARG_SYM							\
	UK_PLAT_NATIVE_SARG_SYM

#endif /* __UK_PLAT_PAL_START_H__ */
