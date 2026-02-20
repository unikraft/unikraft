/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_START_H__
#define __UK_PAL_START_H__

#include <uk/arch/types.h>

/**
 * Start elements - Platform Abstraction Layer
 *
 * Provides primitives or helpers that may allow the configuration of
 * platform-specific boot/bring-up execution path at runtime.
 */

/**
 * The platform must define per-CPU variables for the CPU start entry, stack
 * and argument and export their symbols through the PAL by aliasing these
 * macros to them.
 *
 * These are all of type __uptr and must be accessed through the per-CPU
 * variables API as such.
 */
#ifndef UK_PAL_SENTRY_SYM
#error "UK_PAL_SENTRY_SYM undefined"
#endif

#ifndef UK_PAL_SSTACKP_SYM
#error "UK_PAL_SSTACKP_SYM undefined"
#endif

#ifndef UK_PAL_SARG_SYM
#error "UK_PAL_SARG_SYM undefined"
#endif

#endif /* __UK_PAL_START_H__ */
