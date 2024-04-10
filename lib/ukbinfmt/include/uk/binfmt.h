/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_BINFMT_H__
#define __UK_BINFMT_H__

#include <uk/alloc.h>
#include <uk/arch/ctx.h>
#include <uk/essentials.h>
#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_BINFMT_NOT_HANDLED	0  /* Not handled. Try next loader. */
#define UK_BINFMT_HANDLED	1  /* Handled. Stop passing to loaders. */
#define UK_BINFMT_HANDLED_CONT	2  /* Handled. Pass to next loader. */

/** Loader type */
enum uk_binfmt_type {
	UK_BINFMT_LOADER_TYPE_NONE,
	UK_BINFMT_LOADER_TYPE_INTERP,
	UK_BINFMT_LOADER_TYPE_EXEC,
};

struct uk_binfmt_loader_args {
	const char *pathname; /* fully qualified */
	char *progname;
	int argc, envc;
	const char **argv;
	const char **envp;
	struct uk_alloc *alloc;
	struct ukarch_ctx ctx;
	__sz stack_size;
	struct uk_binfmt_loader *loader;
	void *user; /* private loader data */
};

/* Loader load function.
 *
 * @param args[in|out] Loader args. May be updated by the loader.
 * @return
 *         - UK_BINFMT_NOT_HANDLED if this loader does not handle this file type
 *         - UK_BINFMT_HANDLED if handled (final)
 *         - UK_BINFMT_HANDLED_CONT if handled and should be passed to the next
 *                                  loader. Can only be returned by loaders of
 *                                  TYPE_INTERP. Upon returning this value the
 *                                  next loader is searched among loaders of
 *                                  TYPE_EXEC.
 *         - Negative value on load error
 */
typedef	int (*uk_binfmt_load_fn)(struct uk_binfmt_loader_args *args);

/* Loader unload function.
 *
 * @param args Loader args.
 * @return zero on success or negative value on error.
 */
typedef	int (*uk_binfmt_unload_fn)(struct uk_binfmt_loader_args *args);

struct uk_binfmt_loader {
	/** loader name */
	const char *name;
	/** loader type (see ::uk_binfmt_type) */
	enum uk_binfmt_type type;
	/** loader ops */
	struct {
		uk_binfmt_load_fn load;
		uk_binfmt_load_fn unload;
	} ops;
	struct uk_list_head list_head;
};

/**
 * Register an interpreter
 *
 * @param loader pointer to loader descriptor.
 * @return zero on success, negative value on error.
 */
int uk_binfmt_register(struct uk_binfmt_loader *loader);

/**
 * Load an executable using a suitable loader
 *
 * @param args[in|out] Loader args. May be updated by the loader
 * @return zero on success, negative value on error
 */
int uk_binfmt_load(struct uk_binfmt_loader_args *args);

/**
 * Unload executable
 *
 * @param args[in|out] Loader args. May be updated by the loader
 * @return zero on success, negative value on error
 */
int uk_binfmt_unload(struct uk_binfmt_loader_args *args);

#ifdef __cplusplus
}
#endif

#endif /* __UK_BINFMT_H__ */
