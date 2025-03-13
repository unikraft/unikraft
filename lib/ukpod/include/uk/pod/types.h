/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Common PoD types & callback API */

#ifndef __UK_POD_TYPES_H__
#define __UK_POD_TYPES_H__

#include <uk/arch/paging.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>

/**
 * Page in `npages` starting from addr, as part of an allocation registered with
 * `arg`, with `pgoff` set to `base` + page offset in allocation.
 *
 * May process fewer pages than requested, but must always make progress or
 * error out. It is undefined behavior to return 0.
 *
 * If `pa` is set (ukarch_paddr_isvalid) the buffer is guaranteed to be
 * physically contiguous.
 * Specific backends may or may not provide `pa`.
 *
 * @param addr Virtual address of buffer to page in
 * @param npages Number of pages to process
 * @param pa Physical address of target buffer, if valid
 * @param arg Argument passed when registering the PoD allocation
 * @param pgoff Set to `base` + page offset in allocation
 * @return
 *   > 0: number of pages read into memory
 *   < 0: negative error code
 */
typedef __ssz (*uk_pod_pgin_func)(void *addr, __sz npages, __paddr_t pa,
				  void *arg, __sz pgoff);

/**
 * Write back `npages` starting from addr, as part of an allocation registered
 * with `arg`, with `pgoff` set to `base` + page offset in allocation.
 *
 * May process fewer pages than requested, but must always make progress or
 * error out. It is undefined behavior to return 0.
 *
 * @param addr Virtual address of buffer to page out
 * @param npages Number of pages to process
 * @param arg Argument passed when registering the PoD allocation
 * @param pgoff Set to `base` + page offset in allocation
 * @return
 *   > 0: number of pages written back from memory
 *   < 0: negative error code
 */
typedef __ssz (*uk_pod_pgwb_func)(const void *addr, __sz npages,
				  void *arg, __sz pgoff);

/**
 * Called by supporting backends just before `npages` starting from `addr` are
 * to be deallocated and no longer backed the physical pages starting at `pa`.
 *
 * This operation is called only paired with a `pagein` call that supplies a
 * valid physical address.
 *
 * Memory is part of an allocation registered with `arg`, with `pgoff` set to
 * `base` + page offset in allocation.
 *
 * Cannot fail.
 *
 * @param addr Virtual address of buffer to be deallocated
 * @param npages Number of pages to process
 * @param pa Physical address of target buffer
 * @param arg Argument passed when registering the PoD allocation
 * @param pgoff Set to `base` + page offset in allocation
 */
typedef void (*uk_pod_pgout_func)(const void *addr, __sz npages, __paddr_t pa,
				  void *arg, __sz pgoff);

/**
 * Page I/O operations used by PoD, provided to it by callers.
 *
 * When pages are mapped in, `pagein` is used to initialize their contents.
 * Before pages are mapped out, as well as on explicit request, `writeback` is
 * called to ensure persistence if so desired.
 *
 * If `pagein` is supplied with a valid physical address, a corresponding call
 * to `pageout` will be made just prior to unmapping that address.
 *
 * Mapping in or out may occur at any time to an arbitrary number of pages,
 * potentially resulting in multiple calls to `readin`/`writeback` over the
 * course of a single allocation's lifetime.
 *
 * All operations may be called concurrently any number of times, with all
 * calls guaranteed to target non-overlapping buffers.
 */
struct uk_pod_pgio {
	uk_pod_pgin_func pagein;
	uk_pod_pgwb_func writeback;
	uk_pod_pgout_func pageout;
};

#endif /* __UK_POD_TYPES_H__ */
