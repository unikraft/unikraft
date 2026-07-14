/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Michalis Pappas <mpappas@fastmail.fm>.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKPLAT_MTE_H__
#define __UKPLAT_MTE_H__

#include <uk/arch/util.h>
#include <uk/config.h>

#if defined(CONFIG_ARM64_FEAT_MTE_TCF_ASYNC) || \
    defined(CONFIG_ARM64_FEAT_MTE_TCF_ASYMMETRIC)

#define mte_async_fault()	UK_ARCH_ARM64_SYSREG_READ(TFSR_EL1)
#else /* !CONFIG_ARM64_FEAT_MTE_TCF_[ASYNC, ASYMMETRIC] */
#define mte_async_fault() 0
#endif /* !CONFIG_ARM64_FEAT_MTE_TCF_[ASYNC, ASYMMETRIC] */

#endif /* __UKPLAT_MTE_H__ */
