/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/assert.h>
#include <uk/binfmt.h>
#include <uk/list.h>

static UK_LIST_HEAD(uk_binfmt_loader_list);

#define uk_binfmt_loader_foreach(_loader, _type)			     \
	uk_list_for_each_entry((_loader), &uk_binfmt_loader_list, list_head) \
		if ((_loader)->type == (_type))

int uk_binfmt_register(struct uk_binfmt_loader *loader)
{
	UK_ASSERT(loader);
	UK_ASSERT(loader->name);
	UK_ASSERT(loader->ops.load);
	UK_ASSERT(loader->ops.unload);
	UK_ASSERT(loader->type != UK_BINFMT_LOADER_TYPE_NONE);

	uk_pr_debug("Register loader: %s\n", loader->name);
	uk_list_add(&loader->list_head, &uk_binfmt_loader_list);

	return 0;
}

int uk_binfmt_load(struct uk_binfmt_loader_args *args)
{
	struct uk_binfmt_loader *ldr;
	int rc;

	UK_ASSERT(args);
	UK_ASSERT(args->pathname);
	UK_ASSERT(args->alloc);
	UK_ASSERT(args->ctx.sp);
	UK_ASSERT(args->stack_size);
	UK_ASSERT(!args->user);
	UK_ASSERT(!args->loader);

	/* Try INTERP types first. These should update args and
	 * fall back to EXEC types.
	 */
	uk_binfmt_loader_foreach(ldr, UK_BINFMT_LOADER_TYPE_INTERP) {
		UK_ASSERT(ldr->ops.load);
		rc = ldr->ops.load(args);
		if (unlikely(rc < 0)) {
			uk_pr_err("binfmt loader error (%d)\n", rc);
			return rc;
		}
		if (rc == UK_BINFMT_HANDLED_CONT) {
			/* For now restrict the use of private data
			 * to the final loader.
			 */
			UK_ASSERT(!args->user);
			break; /* Assume one interpreter max */
		}
		if (rc == UK_BINFMT_HANDLED)
			return 0;
	}

	uk_binfmt_loader_foreach(ldr, UK_BINFMT_LOADER_TYPE_EXEC) {
		rc = ldr->ops.load(args);
		if (unlikely(rc < 0)) {
			uk_pr_err("binfmt loader error (%d)\n", rc);
			return rc;
		}
		/* Only one loader of TYPE_EXEC is expected
		 * to handle a binary.
		 */
		UK_ASSERT(rc != UK_BINFMT_HANDLED_CONT);
		if (rc == UK_BINFMT_HANDLED) {
			uk_pr_debug("Load handled by %s\n", ldr->name);
			args->loader = ldr;
			return 0;
		}
	}

	uk_pr_err("%s: Loader not found\n", args->pathname);

	return -ENOEXEC;
}

int uk_binfmt_unload(struct uk_binfmt_loader_args *args)
{
	int rc;

	UK_ASSERT(args);
	UK_ASSERT(args->loader);

	uk_pr_debug("Unload handled by %s\n", args->loader->name);
	rc = args->loader->ops.unload(args);
	if (unlikely(rc < 0)) {
		uk_pr_err("binfmt unload error (%d)\n", rc);
		return rc;
	}

	UK_ASSERT(rc != UK_BINFMT_HANDLED_CONT);

	return 0;
}
