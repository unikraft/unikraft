/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/init.h>
#include <uk/process.h>

UK_EVENT(POSIX_PROCESS_CLONE_EVENT);
UK_EVENT(POSIX_PROCESS_EXECVE_EVENT);
UK_EVENT(POSIX_PROCESS_PPEXIT_EVENT); /* pprocess */
UK_EVENT(POSIX_PROCESS_PTEXIT_EVENT); /* pthread */
UK_EVENT(POSIX_PROCESS_RELEASE_EVENT);

extern const struct posix_process_clonetab_entry _posix_process_clonetab_start[];
extern const struct posix_process_clonetab_entry _posix_process_clonetab_end;

#define posix_process_clonetab_foreach(itr)				\
	for ((itr) = _posix_process_clonetab_start;			\
	     (itr) < &(_posix_process_clonetab_end);			\
	     (itr)++)

static __uk_tls struct {
	bool is_cloned;
	__u64 cl_flags;
} cl_status = { false, 0x0 };

/* clone() flags we can handle. We initialize these to the flags
 * we manage internally in libposix-process and during init append
 * the flags handlers have registered for.
 */
static __u64 cl_flags_handled = CLONE_THREAD | CLONE_VFORK | CLONE_VM |
				CLONE_CHILD_SETTID | CLONE_PARENT_SETTID |
				CLONE_SETTLS | CLONE_DETACHED;

/*
 * Raise clone event
 */
int pprocess_raise_clone_event(struct posix_process_clone_event_data *event_data)
{
	const struct clone_args *cl_args;
	struct uk_thread *child;
	int ret;

	UK_ASSERT(event_data);
	UK_ASSERT(event_data->cl_args);
	UK_ASSERT(event_data->child);

	cl_args = event_data->cl_args;
	child = event_data->child;

	/* Test if we can handle all requested clone flags */
	if (unlikely(cl_args->flags & ~cl_flags_handled)) {
		uk_pr_err("posix_clone %p (%s): Unsupported clone flags requested: 0x%" __PRIx64 "\n",
			  child, child->name ? child->name : "<unnamed>",
			  cl_args->flags);
		return -ENOTSUP;
	}

	/* Raise clone event */
	ret = uk_raise_event(POSIX_PROCESS_CLONE_EVENT, event_data);
	if (unlikely(ret < 0)) {
		uk_pr_err("POSIX_PROCESS_CLONE_EVENT handler error (%d)\n",
			  ret);
		return ret;
	}

	/* Make sure no handler returns UK_EVENT_HANDLED as that
	 * would prevent propagating the event to the rest of
	 * the handlers.
	 */
	UK_ASSERT(ret != UK_EVENT_HANDLED);

	/* Set child TLS variables */
	uk_thread_uktls_var(child, cl_status.is_cloned) = true;
	uk_thread_uktls_var(child, cl_status.cl_flags)  = cl_args->flags;

	return 0;
}

int cl_flags_handled_set(struct uk_init_ctx *ctx __unused)
{
	const struct posix_process_clonetab_entry *itr;

	posix_process_clonetab_foreach(itr)
		cl_flags_handled |= itr->flags_mask;

	return 0;
}

uk_lib_initcall(cl_flags_handled_set, 0);

/*
 * Clear child TLS variables
 */
static void pprocess_clear_clone_tls(struct uk_thread *child __maybe_unused)
{
	UK_ASSERT(child);
	UK_ASSERT(uk_lcpu_tlsp_get() == child->uktlsp);

	if (!cl_status.is_cloned)
		return;

	cl_status.is_cloned = false;
	cl_status.cl_flags = 0x0;
}

UK_THREAD_INIT_PRIO(0x0, pprocess_clear_clone_tls, UK_PRIO_LATEST);

#if CONFIG_LIBPOSIX_PROCESS_EXECVE
/*
 * Raise execve event
 */
int pprocess_raise_execve_event(struct posix_process_execve_event_data *data)
{
	int ret;

	ret = uk_raise_event(POSIX_PROCESS_EXECVE_EVENT, data);

	/* Make sure no handler returns UK_EVENT_HANDLED as that
	 * would prevent propagating the event to the rest of
	 * the handlers.
	 */
	UK_ASSERT(ret != UK_EVENT_HANDLED);

	return ret;
}
#endif /* CONFIG_LIBPOSIX_PROCESS_EXECVE */

/*
 * Raise pthread exit event
 */
void pprocess_raise_ptexit_event(struct posix_process_ptexit_event_data *data)
{
	int ret __maybe_unused;

	ret = uk_raise_event(POSIX_PROCESS_PTEXIT_EVENT, data);

	/* Make sure no handler returns UK_EVENT_HANDLED as that
	 * would prevent propagating the event to the rest of
	 * the handlers.
	 */
	UK_ASSERT(ret != UK_EVENT_HANDLED);
}

/*
 * Raise pprocess exit event
 */
void pprocess_raise_ppexit_event(struct posix_process_ppexit_event_data *data)
{
	int ret __maybe_unused;

	ret = uk_raise_event(POSIX_PROCESS_PPEXIT_EVENT, data);

	/* Make sure no handler returns UK_EVENT_HANDLED as that
	 * would prevent propagating the event to the rest of
	 * the handlers.
	 */
	UK_ASSERT(ret != UK_EVENT_HANDLED);
}
