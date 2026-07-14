/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_START_H__
#define __UK_LCPU_START_H__

#include <uk/arch/types.h>
#include <uk/plat/pal/start.h>
#include <uk/pal/start.h>

#define UK_LCPU_SENTRY_SYM						\
	UK_PAL_SENTRY_SYM
#define UK_LCPU_SSTACKP_SYM						\
	UK_PAL_SSTACKP_SYM
#define UK_LCPU_SARG_SYM						\
	UK_PAL_SARG_SYM

#endif /* __UK_LCPU_START_H__ */
