/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PRINT_STORE_H__
#define __UK_PRINT_STORE_H__

#include <uk/store.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ukstore entry that controls the verbosity at runtime.
 * For values see UK_PRINT_KLVL_ in print.h
 */
#define UK_PRINT_KLVL_ENTRY_ID		1

#ifdef __cplusplus
}
#endif

#endif /* __UK_PRINT_STORE_H__ */
