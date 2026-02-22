/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_LCPU_H__
#define __UK_PLAT_NATIVE_ARCH_LCPU_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_LCPU
extern __vaddr_t uk_plat_native_x86_64_start16_addr; /* target address */

#if CONFIG_HAVE_SMP
/**
 * Send an Interprocessor Interrupt.
 *
 * @param id The CPU ID to send the IPI to
 * @param irq The IRQ to send to the CPU
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_send_ipi(__u64 id, unsigned long irq);

#endif /* CONFIG_HAVE_SMP */
#endif /* CONFIG_LIBUKPLAT_NATIVE_LCPU */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_LCPU_H__ */
