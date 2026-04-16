/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <uk/alloc.h>
#include <uk/event.h>
#include <uk/paging.h>
#include <uk/print.h>
#include <uk/prio.h>
#include <uk/process.h>
#include <uk/syscall.h>
#if CONFIG_LIBUKVMEM
#include <uk/vmem.h>
#endif /* CONFIG_LIBUKVMEM */
#include <uk/process/events.h>
#include "process.h"

#define HEAP_PAGES							\
	(1UL << CONFIG_LIBPOSIX_PROCESS_BRK_PAGE_ORDER)
#define HEAP_LEN							\
	(UK_PAGING_PAGE_SIZE * HEAP_PAGES)

#if CONFIG_LIBUKVMEM
static inline int pprocess_brk_init_zone(struct posix_process *pprocess)
{
	__vaddr_t va = UK_PAGING_VADDR_ANY;
	int rc;

	rc = uk_vma_map_anon(uk_vas_get_active(), &va, HEAP_LEN,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, "brk VMA");
	if (unlikely(rc))
		return rc;

	pprocess->brk_ctx.base = (void *)va;

	return 0;
}
#else /* !CONFIG_LIBUKVMEM */
static inline int pprocess_brk_init_zone(struct posix_process *pprocess)
{
	/* Take a big enough region from the global allocator. */
	pprocess->brk_ctx.base = uk_palloc(pprocess->_a, HEAP_PAGES);
	if (unlikely(!pprocess->brk_ctx.base))
		return -ENOMEM;

	return 0;
}
#endif /* !CONFIG_LIBUKVMEM */

static int pprocess_brk_init(void *arg)
{
	struct posix_process_clone_event_data *event_data;
	struct posix_process *pprocess;
	int rc;

	UK_ASSERT(arg);

	event_data = (struct posix_process_clone_event_data *)arg;
	pprocess = pid2pprocess(event_data->pid);

	UK_ASSERT(!pprocess->brk_ctx.base);

	rc = pprocess_brk_init_zone(pprocess);
	if (unlikely(rc)) {
		uk_pr_err("Failed to create brk space (%lu KiB).\n",
			  (__u64)HEAP_LEN / 1024);
		return rc;
	}

	pprocess->brk_ctx.pos = 0;

	/* Needs to be recursive because sbrk() also needs early access. */
	uk_mutex_init_config(&pprocess->brk_ctx.mtx,
			     UK_MUTEX_CONFIG_RECURSE);

	uk_pr_debug("New brk heap region: %p-%p\n",
		    pprocess->brk_ctx.base,
		    (__u8 *)pprocess->brk_ctx.base + HEAP_LEN);

	return 0;
}

POSIX_PROCESS_CLONE_HANDLER(CLONE_VFORK, pprocess_brk_init);

#if CONFIG_LIBUKVMEM
static inline int pprocess_brk_free_zone(struct posix_process *pprocess)
{
	return uk_vma_unmap(uk_vas_get_active(),
			    (__vaddr_t)pprocess->brk_ctx.base, HEAP_LEN, 0);
}
#else /* !CONFIG_LIBUKVMEM */
static inline int pprocess_brk_free_zone(struct posix_process *pprocess)
{
	uk_pfree(pprocess->_a, pprocess->brk_ctx.base, HEAP_PAGES);
	return 0;
}
#endif /* !CONFIG_LIBUKVMEM */

static int pprocess_brk_free(void *arg)
{
	struct posix_process_ppexit_event_data *event_data;
	struct posix_process *pprocess;
	int rc;

	UK_ASSERT(arg);

	event_data = (struct posix_process_ppexit_event_data *)arg;
	pprocess = pid2pprocess(event_data->pid);

	UK_ASSERT(pprocess->brk_ctx.base);

	/**
	 * The process is exiting and if there is any thread currently holding
	 * the brk mutex, we don't really care if we are releasing the brk area,
	 * but in the ukvmem DONTNEED case there might be some issues.
	 * Technically, in the ukvmem case, the unmapping might become
	 * blocking in the future in combination with that other thread
	 * attempting a DONTNEED, causing this thread to also block on it.
	 * However, when the other thread wakes up to finish its DONTNEED,
	 * its POSIX resources would have supposedly already died, as this
	 * handler can only run after POSIX thread resources have been released,
	 * possibly resulting in an invalid state.
	 * At this point in time, ukvmem is not synchronized yet though, so
	 * it's something for later.
	 *
	 * TODO: Reconsider synchronization approach when ukvmem API might lead
	 * to thread yielding.
	 */
	uk_pr_debug("Freeing brk heap region: %p-%p\n",
		    pprocess->brk_ctx.base,
		    (__u8 *)pprocess->brk_ctx.base + HEAP_LEN);

	rc = pprocess_brk_free_zone(pprocess);
	if (unlikely(rc))
		UK_CRASH("Failed to free brk space (%lu KiB).\n",
			 (__u64)HEAP_LEN / 1024);

	pprocess->brk_ctx.pos = 0;
	pprocess->brk_ctx.base = __NULL;

	return 0;
}

POSIX_PROCESS_PPEXIT_HANDLER(pprocess_brk_free);

#if CONFIG_LIBPOSIX_PROCESS_BRK_INC_MEMZERO
#if CONFIG_LIBUKVMEM && CONFIG_LIBPOSIX_PROCESS_BRK_DEC_DONTNEED
/**
 * According to POSIX:
 * The newly-allocated space is set to 0. However, if the application first
 * decrements and then increments the break value, the contents of the
 * reallocated space are unspecified.
 *
 * Therefore, we technically afford to free up pages that were decremented.
 * However, Linux does not do this and upon reincrements the pages maintain
 * old data. So this feature is optional.
 */

static inline int memzero_brk_inrange(__uptr cur_brk, __uptr addr)
{
	__vaddr_t free_start, free_end;
	__sz zlen;
	int rc;

	/*
	 * Increment: Zero out the difference within the same page.
	 * We don't even need to zero the bump on page boundary crossings as
	 * the backing VMA does not have the UK_VMA_MAP_UNINITIALIZED flag which
	 * means that every new page we fault on will be zeroed out on demand
	 * instead of us having to explicitly memset the difference every
	 * time. This wouldn't be achievable if we didn't free the decrements
	 * on page boundaries.
	 *
	 * See glibc's MORECORE_CLEARS. We go with MORECORE_CLEARS == 2 to
	 * be on the safe side.
	 */
	if (addr > cur_brk) {
		/*
		 * Zero out the part of the bump that is still wthin the same
		 * page.
		 */
		if (addr > UK_PAGING_PAGE_ALIGN_UP(cur_brk)) {
			/*
			 * If addr is beyond the page cur_brk is in then zero
			 * out starting from cur_brk to the start of the next
			 * page.
			 */
			zlen = UK_PAGING_PAGE_ALIGN_UP(cur_brk) - cur_brk;
		} else {
			/* If addr is within the page cur_brk is in then zero
			 * out starting from cur_brk to addr.
			 */
			zlen = addr - cur_brk;
		}

		memset((void *)cur_brk, 0, zlen);

		return 0;
	}

	/*
	 * Decrement: Free up whole pages between previous cur_brk and addr.
	 */

	/* Align boundaries to identify full pages */
	free_start = UK_PAGING_PAGE_ALIGN_UP(addr);
	free_end = UK_PAGING_PAGE_ALIGN_UP(cur_brk);

	/* Check if we have at least one full page to free */
	if (!(free_start < free_end))
		return 0;

	uk_pr_debug("Freeing up brk range 0x%lx-0x%lx (%lu pages)\n",
		    free_start, free_end,
		    (free_end - free_start) >> UK_PAGING_PAGE_SHIFT);

	rc = uk_vma_advise(uk_vas_get_active(), free_start,
			   free_end - free_start, UK_VMA_ADV_DONTNEED, 0);
	if (unlikely(rc)) {
		uk_pr_err("Failed to free up pages between 0x%lx-0x%lx\n",
			  free_start, free_end);
		return rc;
	}

	return 0;
}
#else /* !(CONFIG_LIBUKVMEM && CONFIG_LIBPOSIX_PROCESS_BRK_DEC_DONTNEED) */
static inline int memzero_brk_inrange(__uptr cur_brk, __uptr addr)
{
	/*
	 * Increment: We must zero out the bumped region as some applications
	 * might expect.
	 *
	 * See glibc's MORECORE_CLEARS. We go with MORECORE_CLEARS == 2 to be
	 * on the safe side.
	 */
	if (addr > cur_brk) {
		uk_pr_debug("Zeroing 0x%lx-0x%lx...\n", cur_brk, addr);
		memset((void *)cur_brk, 0x0, addr - cur_brk);
	}

	/*
	 * Decrement: Do nothing as we are in one single allocated region
	 * whose individual pages we cannot arbitrarily free.
	 */

	return 0;
}
#endif /* !(CONFIG_LIBUKVMEM && CONFIG_LIBPOSIX_PROCESS_BRK_DEC_DONTNEED) */
#endif /* CONFIG_LIBPOSIX_PROCESS_BRK_INC_MEMZERO */

UK_LLSYSCALL_R_DEFINE(void *, brk, void *, addr)
{
	struct posix_process *pprocess;
	__uptr base, cur_brk, addr_arg;
	int rc __maybe_unused;

	pprocess = uk_pprocess_current();
	UK_ASSERT(pprocess);

	uk_mutex_lock(&pprocess->brk_ctx.mtx);

	base = (__uptr)pprocess->brk_ctx.base;
	cur_brk = base + pprocess->brk_ctx.pos;
	addr_arg = (__uptr)addr;

	if (addr_arg == cur_brk) {
		uk_mutex_unlock(&pprocess->brk_ctx.mtx);
		return addr;
	}

	if (!IN_RANGE(addr_arg, base, HEAP_LEN)) {
		uk_pr_debug("0x%lx outside PID %d's brk range 0x%lx-0x%lx, keep brk @ 0x%lx\n",
			    addr_arg, pprocess->pid,
			    base, base + HEAP_LEN, cur_brk);
		uk_mutex_unlock(&pprocess->brk_ctx.mtx);
		return (void *)cur_brk;
	}

#if CONFIG_LIBPOSIX_PROCESS_BRK_INC_MEMZERO
	rc = memzero_brk_inrange(cur_brk, addr_arg);
	if (unlikely(rc)) {
		uk_mutex_unlock(&pprocess->brk_ctx.mtx);
		return (void *)cur_brk;
	}
#endif /* CONFIG_LIBPOSIX_PROCESS_BRK_INC_MEMZERO */

	uk_pr_debug("PID %d brk @ 0x%lx (brk heap region: 0x%lx-0x%lx)\n",
		    pprocess->pid, addr_arg, base, base + HEAP_LEN);

	pprocess->brk_ctx.pos = addr_arg - base;

	uk_mutex_unlock(&pprocess->brk_ctx.mtx);

	return addr;
}

#if UK_LIBC_SYSCALLS
int brk(void *addr)
{
	void *new_brk;

	/**
	 * In practice glibc seems to return success even when addr is below
	 * current brk position. Since we need to be compatible with other
	 * libcs, mimic this behavior.
	 */
	new_brk = (void *)uk_syscall_r_brk((long)addr);
	if (unlikely(new_brk < addr)) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

#if CONFIG_LIBNOLIBC
void *sbrk(intptr_t inc)
{
	struct posix_process *pprocess;
	__uptr req_brk, prev_brk;
	void *new_brk;

	pprocess = uk_pprocess_current();
	UK_ASSERT(pprocess);

	uk_mutex_lock(&pprocess->brk_ctx.mtx);

	/* We are increasing or reducing our range */
	prev_brk = (__uptr)pprocess->brk_ctx.base + pprocess->brk_ctx.pos;

	/* Check for 0 and overflow/underflow */
	if (!inc) {
		uk_mutex_unlock(&pprocess->brk_ctx.mtx);
		return (void *)prev_brk;
	} else if (inc > 0) {
		if (unlikely(prev_brk + (__uptr)inc < prev_brk))
			goto err;
	} else {
		if (unlikely((__uptr)prev_brk < (__uptr)-inc))
			goto err;
	}

	req_brk = (__uptr)prev_brk + inc;
	new_brk = (void *)uk_syscall_r_brk((long)req_brk);
	if (unlikely((__uptr)new_brk < req_brk))
		goto err;

	uk_mutex_unlock(&pprocess->brk_ctx.mtx);
	return (void *)prev_brk;

err:
	uk_mutex_unlock(&pprocess->brk_ctx.mtx);
	errno = ENOMEM;
	return (void *)-1;
}
#endif /* CONFIG_LIBNOLIBC */
#endif /* UK_LIBC_SYSCALLS */
