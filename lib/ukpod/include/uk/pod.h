/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Pages-on-Demand (PoD) library
 *
 * PoD allows callers to obtain contiguous read-write virtual memory that:
 * - may or may not be mapped by physical RAM at any time
 * - behaves consistently with it always being mapped
 * - has this consistency ensured by caller-provided callbacks
 *
 * PoD is an abstraction over the concept of on-demand paging that aims to
 * provide behavior guarantees independent of implementation, the latter being
 * delegated to PoD backends. These backends can then be tailored to the use
 * case at hand (e.g., builds with or without ukvmem).
 * Notably, this allows consumers of PoD to benefit from true demand-paging when
 * available, without directly depending on virtual memory support.
 * This transparent behavior is provided by the default system PoD.
 *
 * A PoD backend provides the following operations:
 * - alloc -- create a PoD area
 * - free -- free (destroy) a PoD area
 * - writeback -- explicitly request writeback of pages in a PoD area
 * - drop -- explicitly request discarding pages within a PoD area
 * - lock -- ensure pages in a PoD area remain resident in memory
 * - unlock -- reverse the effects of lock
 * - populate -- ensure pages in a PoD area are memory resident and populated
 * The operations are presented in more detail in <uk/pod/default.h>, consult
 * that file for more information.
 *
 * In addition to the system-wide PoD backend, and in order to satisfy bespoke
 * requirements, callers can explicitly create private PoD instances of any
 * backend independent of the system PoD.
 */

/* PoD functions work with lengths and offsets in units of "pages".
 *
 * In the context of PoD a "page" is defined to be exactly PAGE_SIZE bytes,
 * regardless of the actual hardware page sizes in use (e.g., hugepages).
 */

/* PoD functions all take as arguments `ops` -- page I/O operations -- as well
 * as `arg` and `base`, which are used when calling the page-in/writeback ops.
 *
 * During calls to the page-in/writeback ops:
 * - `arg` is passed verbatim to every call
 * - `base` is adjusted upwards to reflect the number of pages that the target
 *   buffer is offset from the start of the returned allocation
 *
 * When operating on an allocated buffer, `ops` and `arg` must match those
 * provided in the call to `uk_pod_alloc`.
 * If the target address of a PoD function is a whole buffer, `base` must match
 * the value used when calling `uk_pod_alloc`. If the target is in the middle of
 * an allocation, `base` must be adjusted upwards with the number of pages
 * relative to the start value.
 * It is undefined behavior if any of the previous do not hold true.
 */

/* PoD functions that operate on existing allocations all take the arguments
 * `addr` and `npages` to describe the buffer being operated on.
 *
 * It is undefined behavior if:
 * - `addr` is not page-aligned
 * - `addr` + `npages` describe a buffer that is not entirely backed by
 *   allocations returned from previous calls to `uk_pod_*_alloc`
 */

#ifndef __UK_POD_H__
#define __UK_POD_H__

/* This main header includes the following, consult them for details:
 * - <uk/pod/types.h> basic types & callback API
 * - <uk/pod/default.h> system default PoD API
 * - <uk/pod/...> backend-specific APIs
 *
 * In addition:
 * - <uk/pod/external.h> API for providing the system default PoD externally
 * - <uk/pod/anon.h> Utility ops for managing private anonymous memory with PoD
 *
 * A caller into PoD may include this header directly, or just the subset of
 * <uk/pod/...> that provides the required functionality.
 */

#include <uk/pod/types.h>

#include <uk/pod/eager.h>

#include <uk/pod/default.h>

#endif /* __UK_POD_H__ */
