/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UKTIMER_TICK_H__
#define __UKTIMER_TICK_H__

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __u64 uk_jiffies;

/**
 * Enable regular updating of uk_jiffies.
 * @param idle a non-zero value if uktimer should not keep uk_jiffies regularly
 * updated, zero if it should keep updating uk_jiffies in regular intervals.
 */
void uktimer_set_idle(int idle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UKTIMER_TICK_H__ */
