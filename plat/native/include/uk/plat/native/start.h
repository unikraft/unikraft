/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_START_H__
#define __UK_PLAT_NATIVE_START_H__

#include <uk/plat/native/arch/start.h>

/* Platform bring-up definitions/helpers */

#if CONFIG_LIBUKPLAT_NATIVE_START
#ifndef UK_PLAT_NATIVE_SENTRY_SYM
#error "UK_PLAT_NATIVE_SENTRY_SYM undefined"
#endif

#ifndef UK_PLAT_NATIVE_SSTACKP_SYM
#error "UK_PLAT_NATIVE_SSTACKP_SYM undefined"
#endif

#ifndef UK_PLAT_NATIVE_SARG_SYM
#error "UK_PLAT_NATIVE_SARG_SYM undefined"
#endif
#endif /* CONFIG_LIBUKPLAT_NATIVE_START */

#endif /* __UK_PLAT_NATIVE_START_H__ */
