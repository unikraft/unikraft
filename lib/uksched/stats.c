/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE /* asprintf */
#include <stdio.h>

#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/init.h>
#include <uk/store.h>
#include <uk/wait.h>
#include <uk/sched_impl.h>
#include <uk/sched/store.h>

/* cpu total time */
static int get_cpu_total_time(void *cookie __unused, __u64 *out)
{
	struct uk_sched *sched = (struct uk_sched *)cookie;

	UK_ASSERT(sched);

	*out = ukplat_monotonic_clock() - sched->start_time;
	return 0;
}

/* cpu idle time */
static int get_cpu_idle_time(void *cookie, __u64 *out)
{
	struct uk_sched *sched = (struct uk_sched *)cookie;
	const struct uk_thread *idle_thread;

	UK_ASSERT(sched);

	idle_thread = uk_sched_idle_thread(sched, 0);

	UK_ASSERT(idle_thread);

	*out = idle_thread->exec_time;

	return 0;
}

/* total number of schedules */
static int get_sched_count(void *cookie __unused, __u64 *out)
{
	struct uk_sched *sched = (struct uk_sched *)cookie;

	UK_ASSERT(sched);

	*out = sched->sched_count;

	return 0;
}

/* total number of context switches to the next thread */
static int get_next_count(void *cookie __unused, __u64 *out)
{
	struct uk_sched *sched = (struct uk_sched *)cookie;

	UK_ASSERT(sched);

	*out = sched->next_count;

	return 0;
}

/* total number of yields */
static int get_yield_count(void *cookie __unused, __u64 *out)
{
	struct uk_sched *sched = (struct uk_sched *)cookie;

	UK_ASSERT(sched);

	*out = sched->yield_count;

	return 0;
}

/* total number of context switches to the idle thread */
static int get_idle_count(void *cookie __unused, __u64 *out)
{
	struct uk_sched *sched = (struct uk_sched *)cookie;

	UK_ASSERT(sched);

	*out = sched->idle_count;

	return 0;
}

static const struct uk_store_entry *dyn_entries[] = {
	UK_STORE_ENTRY(UK_SCHED_STATS_CPU_TIME_TOTAL, "cpu_time_total",
		       u64, get_cpu_total_time, NULL),
	UK_STORE_ENTRY(UK_SCHED_STATS_CPU_TIME_IDLE, "cpu_time_idle",
		       u64, get_cpu_idle_time, NULL),
	UK_STORE_ENTRY(UK_SCHED_STATS_SCHED_COUNT, "sched_count",
		       u64, get_sched_count, NULL),
	UK_STORE_ENTRY(UK_SCHED_STATS_YIELD_COUNT, "yield_count",
		       u64, get_yield_count, NULL),
	UK_STORE_ENTRY(UK_SCHED_STATS_NEXT_COUNT, "next_count",
		       u64, get_next_count, NULL),
	UK_STORE_ENTRY(UK_SCHED_STATS_IDLE_COUNT, "idle_count",
		       u64, get_idle_count, NULL),
	NULL,
};

int uk_sched_stats_init(struct uk_init_ctx *ctx __unused)
{
	struct uk_store_object *obj;
	struct uk_thread *t;
	struct uk_alloc *a;
	char *obj_name;
	__u64 obj_id;
	int rc;

	a = uk_alloc_get_default();
	UK_ASSERT(a);

	t = uk_thread_current();
	UK_ASSERT(t);

	/* Create stats object */
	obj_id = ukplat_lcpu_idx();
	rc = asprintf(&obj_name, "cpu%ld", obj_id);
	if (unlikely(rc == -1)) {
		uk_pr_err("Could not allocate object name\n");
		return -ENOMEM;
	}

	obj = uk_store_obj_alloc(a, obj_id, obj_name, dyn_entries, t->sched);
	if (unlikely(PTRISERR(obj))) {
		rc = PTR2ERR(obj);
		uk_pr_err("Could not allocate object (%d)\n", rc);
		goto err_obj_alloc;
	}

	rc = uk_store_obj_add(obj);
	if (unlikely(rc)) {
		uk_pr_err("Could not add object (%d)\n", rc);
		goto err_obj_add;
	}

	return 0;

err_obj_add:
	uk_free(a, obj);

err_obj_alloc:
	uk_free(a, obj_name);

	return rc;
}

/* The scheduler starts before init, so if we register stats
 * through uk_scheduler_init() consumers that initialize at
 * early_init won't have the chance to register.
 */
uk_lib_initcall(uk_sched_stats_init, UK_PRIO_EARLIEST);
