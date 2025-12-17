/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PAL_PAGING_H__
#define __UK_PAL_PAGING_H__

#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#if CONFIG_HAVE_PAGING

/**
 * Performs platform-specific initialization
 *
 * @return zero on success, negative value on error
 */
int uk_pal_paging_init(void);

#endif /* CONFIG_HAVE_PAGING */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PAL_PAGING_H__ */
