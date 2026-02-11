/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PSCI_H__
#define __UK_PSCI_H__

#include <uk/plat/common/bootinfo.h>

int uk_psci_init(struct ukplat_bootinfo *bi);

__isr int uk_psci_cpu_on(__u64 cpuid, __u64 entry_point, __u64 context_id);

__isr int uk_psci_system_off(void);

__isr int uk_psci_reset(void);

#endif /* __UK_PSCI_H__ */
