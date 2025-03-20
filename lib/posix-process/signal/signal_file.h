/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PROCESS_SIGNAL_H__
#error Do not include this header directly
#endif

#include <stdbool.h>
#include <uk/file.h>
#include <uk/posix-fd.h>

/* Signal file structure combining file operations with signal handling */
struct uk_signal_file {
	/* Allocator used to create this signal file */
	struct uk_alloc *a;
	/* Signal file refcount */
	uk_file_refcnt frefcnt;
	/* Signal file state */
	struct uk_file_state fstate;
	/* Base file structure */
	struct uk_file f;
	/* Signal mask for this file */
	uk_sigset_t mask;
	/* List head for tracking in signal context */
	struct uk_list_head sigf_head;
};

/* Context for managing multiple signal files */
struct uk_signal_files_ctx {
	/* Combined signal mask of all tracked files */
	uk_sigset_t allmask;
	/* List head for tracking signal files */
	struct uk_list_head sigfiles;
};

/*
 * Initialize signal files context
 *
 * param ctx
 *   Context structure to initialize
 */
static inline
void uk_signal_files_ctx_init(struct uk_signal_files_ctx *ctx)
{
	UK_INIT_LIST_HEAD(&ctx->sigfiles);
	uk_sigemptyset(&ctx->allmask);
}

/*
 * Add signal file to context and update combined mask
 *
 * param ctx
 *   Context to modify
 * param sigf
 *   Signal file to add
 */
static inline
void uk_signal_files_ctx_add(struct uk_signal_files_ctx *ctx,
			     struct uk_signal_file *sigf)
{
	uk_sigorset(&ctx->allmask, &sigf->mask);
	uk_list_add_tail(&sigf->sigf_head, &ctx->sigfiles);
}

/*
 * Remove signal file from context. Mask is not recalculated but instead the
 * recalculation is deferred to the next time the signal files list is
 * iterated upon.
 *
 * param ctx
 *   Context to modify
 * param sigf
 *   Signal file to remove
 */
static inline
void uk_signal_files_ctx_del(struct uk_signal_files_ctx *ctx __unused,
			     struct uk_signal_file *sigf)
{
	/*
	 * NOTE: We do not recalculate the mask here. Instead, as an
	 * optimization, we defer this operation until we need to iterate
	 * through the registered signal files which, at this moment,
	 * is when we want to notify them against a given signal number.
	 */
	uk_list_del_init(&sigf->sigf_head);
}

/*
 * Check if a signal is present in the combined mask
 *
 * param ctx
 *   Context to check
 * param signum
 *   Signal number to check
 *
 * @return
 *   True if signal is in mask, false otherwise
 */
static inline
bool uk_signal_files_ctx_is_set(struct uk_signal_files_ctx *ctx, int signum)
{
	return (bool)uk_sigismember(&ctx->allmask, signum);
}

/*
 * Notify all signal files that monitor a given signal number. At the same time,
 * as an optimization, also recalculate the mask since we are iterating over
 * all signal files anyway.
 *
 * param ctx
 *   Context whose signal files to notify
 * param signum
 *   Signal number to check against for monitoring signal files
 */
static inline
void uk_signal_files_ctx_notify(struct uk_signal_files_ctx *ctx,
				int signum)
{
	struct uk_signal_file *it;
	uk_sigset_t allmask;

	uk_sigemptyset(&allmask);

	uk_list_for_each_entry(it, &ctx->sigfiles, sigf_head) {
		uk_sigorset(&allmask, &it->mask);

		if (!uk_sigismember(&it->mask, signum))
			continue;

		uk_file_event_set(&it->f, UKFD_POLLIN);
	}

	uk_sigcopyset(&ctx->allmask, &allmask);
}

/*
 * Create a signal file with a given mask
 *
 * param a
 *   Allocator to use for signal file creation
 * param mask
 *   Signal mask to be monitored by the signal file
 *
 * @return
 *   Pointer to the created signal file or ERR2PTR on creation failure
 */
const struct uk_file *signal_file_create(struct uk_alloc *a,
					 const uk_sigset_t *mask);
