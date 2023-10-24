/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_SCHED_THREAD_ISR_H__
#define __UK_SCHED_THREAD_ISR_H__

#include <uk/thread.h>

/* ISR variant of `uk_thread_wake()` */
void uk_thread_wake_isr(struct uk_thread *thread);

#endif /* __UK_SCHED_THREAD_ISR_H__ */
