/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_LCPU_H__
#define __UK_PAL_LCPU_H__

/**
 * Logical CPU (LCPU) Management - Platform Abstraction Layer
 *
 * Provides initialization, control, and inter-processor operations for
 * logical CPUs across all platforms and architectures.
 */

#include <uk/arch/types.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_HAVE_SMP
/**
 * Detect and populate LCPU structures for all processors.
 * Enumerates available CPUs and creates LCPU structures.
 *
 * @param arg  Platform-specific argument (may be unused)
 * @return 0 on success, negative errno on failure
 */
int uk_pal_lcpu_mp_init(void *arg);

/**
 * Send an Interprocessor Interrupt.
 *
 * @param id The CPU ID to send the IPI to
 * @param irq The IRQ to send to the CPU
 * @return 0 on success, negative errno on failure
 */
int uk_pal_send_ipi(__u64 id, unsigned long irq);
#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PAL_LCPU_H__ */
