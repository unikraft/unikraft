/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Interface for externally-implemented system PoD.
 *
 * An external library that wishes to implement the system PoD must include this
 * file and provide the following:
 * - definition of `uk_pod_external_ctx` -- runtime context used by the PoD
 * - storage of `uk_pod_external_init_ctx` -- actual runtime state of system PoD
 * - `uk_pod_external_init()` -- init function
 * - PoD API -- alloc, free, lock, unlock, populate, writeback, drop
 */

#ifndef __UK_POD_EXTERNAL_H__
#define __UK_POD_EXTERNAL_H__

#include <uk/pod/types.h>

/* Implementation-specific PoD state; defined in external lib */
struct uk_pod_external_ctx;

extern struct uk_pod_external_ctx uk_pod_external_init_ctx;

/**
 * Ensure the external system PoD instance is initialized with all state stored
 * in `uk_pod_external_init_ctx`.
 *
 * Will be called before any `uk_pod_external_*`.
 * Multiple calls are idempotent.
 *
 * @return
 *   == 0: Success
 *    < 0: Negative error code
 */
int uk_pod_external_init(void);

void *uk_pod_external_alloc(struct uk_pod_external_ctx *ctx,
			    __sz npages, int locked,
			   const struct uk_pod_pgio *ops, void *arg, __sz base);

int uk_pod_external_free(struct uk_pod_external_ctx *ctx,
			 void *addr, __sz npages,
			 const struct uk_pod_pgio *ops, void *arg, __sz base);

int uk_pod_external_lock(struct uk_pod_external_ctx *ctx,
			 void *addr, __sz npages,
			 const struct uk_pod_pgio *ops, void *arg, __sz base);

int uk_pod_external_unlock(struct uk_pod_external_ctx *ctx,
			   void *addr, __sz npages,
			   const struct uk_pod_pgio *ops, void *arg, __sz base);

int uk_pod_external_populate(struct uk_pod_external_ctx *ctx,
			     void *addr, __sz npages,
			     const struct uk_pod_pgio *ops,
			     void *arg, __sz base);

int uk_pod_external_writeback(struct uk_pod_external_ctx *ctx,
			      void *addr, __sz npages,
			      const struct uk_pod_pgio *ops,
			      void *arg, __sz base);

int uk_pod_external_drop(struct uk_pod_external_ctx *ctx,
			 void *addr, __sz npages,
			 const struct uk_pod_pgio *ops, void *arg, __sz base);

#endif /* __UK_POD_EXTERNAL_H__ */
