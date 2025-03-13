/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* System default PoD */

#ifndef __UK_POD_DEFAULT_H__
#define __UK_POD_DEFAULT_H__

#include <uk/config.h>
#include <uk/errptr.h>
#include <uk/pod/types.h>

#if CONFIG_LIBUKPOD_DEFAULT_EAGER

#include <uk/pod/eager.h>

extern struct uk_alloc *uk_pod_eager_init_ctx;

#define UK_POD_DEFAULT_TYPE eager
#define UK_POD_DEFAULT_CTX (uk_pod_eager_init_ctx)

#elif CONFIG_LIBUKPOD_DEFAULT_EXTERNAL

#include <uk/pod/external.h>

#define UK_POD_DEFAULT_TYPE external
#define UK_POD_DEFAULT_CTX (&uk_pod_external_init_ctx)

#else /* !CONFIG_LIBUKPOD_DEFAULT_* */

#error No system PoD backend enabled

#endif /* !CONFIG_LIBUKPOD_DEFAULT_* */

/**
 * Ensure the system PoD is initialized.
 *
 * This function must be called before any other `uk_pod_default_*`.
 * Multiple calls are idempotent.
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code
 */
#if CONFIG_LIBUKPOD_DEFAULT_EXTERNAL
/* Inline to call init function in external lib */
static inline
int uk_pod_default_init(void)
{
	return uk_pod_external_init();
}
#else /* !CONFIG_LIBUKPOD_DEFAULT_EXTERNAL */
/* We provide the implementation within this lib */
int uk_pod_default_init(void);
#endif /* !CONFIG_LIBUKPOD_DEFAULT_EXTERNAL */

/* Macro magic to ensure `uk_pod_default_*` calls are always resolved as direct
 * calls to their backend counterparts, preventing any unwanted indirection.
 */
#define _UK_POD_OP(type, name) uk_pod_ ## type ## _ ## name
#define UK_POD_OP(type, name) _UK_POD_OP(type, name)
#define UK_POD_DEFAULT_OP(name) UK_POD_OP(UK_POD_DEFAULT_TYPE, name)

/**
 * Acquire `npages` pages mapped read+write from the system PoD.
 *
 * If `locked` != 0 these pages, once present in RAM, are guaranteed to remain
 * there (or equivalently consistent) until they are explicitly unlocked or
 * freed, with `ops->writeback` never being called in the meantime.
 * If `locked` == 0, memory accesses to these pages are guaranteed to be
 * consistent, using `ops->readin` and `ops->writeback`, however the pages
 * themselves may or may not be mapped into RAM at any point in time.
 *
 * @param npages Number of pages to acquire
 * @param locked If != 0 lock pages in memory
 * @param ops Page I/O operations
 * @param arg Custom argument to pass to page ops
 * @param base Logical base offset
 *
 * @return
 *  !PTRISERR(ret): Newly allocated buffer
 *   PTRISERR(ret): Negative error code in PTR2ERR(ret)
 */
static inline
void *uk_pod_default_alloc(__sz npages, int locked,
			   const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	return UK_POD_DEFAULT_OP(alloc)(UK_POD_DEFAULT_CTX,
					npages, locked, ops, arg, base);
}

/**
 * Release (part of) a PoD allocation starting at `addr` of `npages` pages.
 *
 * @param addr Starting address
 * @param npages Number of pages to free
 * @param ops Page I/O operations
 * @param arg Custom argument to pass to page ops
 * @param base Logical base offset
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code; buffer is left allocated
 */
static inline
int uk_pod_default_free(void *addr, __sz npages,
			const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	return UK_POD_DEFAULT_OP(free)(UK_POD_DEFAULT_CTX, addr, npages,
				       ops, arg, base);
}

/**
 * Lock a range of `npages` starting at `addr` that are part of a PoD alloc.
 * Locked pages are guaranteed to remain in RAM (or functionally equivalent
 * without the use of any I/O ops) until they are unlocked or explicitly freed.
 *
 * @param addr Starting address
 * @param npages Number of pages to lock
 * @param ops Page I/O operations
 * @param arg Custom argument to pass to page ops
 * @param base Logical base offset
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code; no pages are locked
 */
static inline
int uk_pod_default_lock(void *addr, __sz npages,
			const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	return UK_POD_DEFAULT_OP(lock)(UK_POD_DEFAULT_CTX, addr, npages,
				       ops, arg, base);
}

/**
 * Unlock a range of `npages` starting at `addr` that are part of a PoD alloc.
 *
 * @param addr Starting address
 * @param npages Number of pages to unlock
 * @param ops Page I/O operations
 * @param arg Custom argument to pass to page ops
 * @param base Logical base offset
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code; no pages are unlocked
 */
static inline
int uk_pod_default_unlock(void *addr, __sz npages,
			  const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	return UK_POD_DEFAULT_OP(unlock)(UK_POD_DEFAULT_CTX, addr, npages,
					 ops, arg, base);
}

/**
 * Ensure (part of) a PoD allocation starting at `addr` of `npages` pages is
 * mapped into RAM and populated.
 *
 * @param addr Starting address
 * @param npages Number of pages to populate
 * @param ops Page I/O operations
 * @param arg Custom argument to pass to page ops
 * @param base Logical base offset
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code; range may be left partially populated
 */
static inline
int uk_pod_default_populate(void *addr, __sz npages,
			    const struct uk_pod_pgio *ops,
			    void *arg, __sz base)
{
	return UK_POD_DEFAULT_OP(populate)(UK_POD_DEFAULT_CTX, addr, npages,
					   ops, arg, base);
}

/**
 * Ensure (part of) a PoD allocation starting at `addr` of `npages` pages is
 * written back to persistence without freeing it.
 *
 * @param addr Starting address
 * @param npages Number of pages to write back
 * @param ops Page I/O operations
 * @param arg Custom argument to pass to page ops
 * @param base Logical base offset
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code; range may have been partially written back
 */
static inline
int uk_pod_default_writeback(void *addr, __sz npages,
			     const struct uk_pod_pgio *ops,
			     void *arg, __sz base)
{
	return UK_POD_DEFAULT_OP(writeback)(UK_POD_DEFAULT_CTX, addr, npages,
					    ops, arg, base);
}

/**
 * Immediately discard the contents of (part of) a PoD allocation starting at
 * `addr` of `npages` pages. Any changes previously not written to persistence
 * are lost, and any future accesses will see a freshly populated copy.
 *
 * @param addr Starting address
 * @param npages Number of pages to discard
 * @param ops Page I/O operations
 * @param arg Custom argument to pass to page ops
 * @param base Logical base offset
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code; range may have been partially dropped
 */
static inline
int uk_pod_default_drop(void *addr, __sz npages,
			const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	return UK_POD_DEFAULT_OP(drop)(UK_POD_DEFAULT_CTX, addr, npages,
				       ops, arg, base);
}

#undef _UK_POD_OP
#undef UK_POD_OP
#undef UK_POD_DEFAULT_OP

#endif /* __UK_POD_DEFAULT_H__ */
