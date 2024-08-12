/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_SCHED_STATS__
#define __UK_SCHED_STATS__

/* uksched stats store OIDs
 * ========================
 *
 * By convention, for the stats object ID we use the index
 * of the cpu this scheduler is running on. Yet we cannot
 * export that as a macro that expands to ukplat_lcpu_idx()
 * as the consumer may be executing on a different core.
 *
 * Exported metrics
 * ----------------
 *                                         ____________
 * uk_sched_yield()--[YIELD_COUNT++]-+    |            |
 *                                   |--->| schedule() | [SCHED_COUNT++]
 * preempt --------------------------+    |____________|
 *                                        /   |    |
 * resume current thread <---------------+    |    |
 *                                            |    |
 *                             [IDLE_COUNT++] |    | [NEXT_COUNT++]
 *                                            v    v
 *                                         idle    next
 *                                       thread    thread
 *
 * YIELD_COUNT: Number of calls to yield().
 * SCHED_COUNT: Number of calls to schedule().
 * NEXT_COUNT:  Number of context switches.
 * IDLE_COUNT:  Number of switches to the idle.
 *
 * Given the above it is possible to derive other metrics:
 * - preempt_count = sched_count - yield_count
 * - resume_count = sched_count - switch_count - idle_count
 */
#define UK_SCHED_STATS_CPU_TIME_TOTAL		0x01
#define UK_SCHED_STATS_CPU_TIME_IDLE		0x02
#define UK_SCHED_STATS_SCHED_COUNT		0x03
#define UK_SCHED_STATS_YIELD_COUNT		0x04
#define UK_SCHED_STATS_NEXT_COUNT		0x05
#define UK_SCHED_STATS_IDLE_COUNT		0x06

#endif /* __UK_SCHED_STATS__ */
