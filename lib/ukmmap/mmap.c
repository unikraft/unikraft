/* SPDX-License-Identifier: BSD-3-Clause */
/*
 *
 * Authors: Charalampos Mainas <charalampos.mainas@neclab.eu>
 *
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/mman.h>
#include <uk/alloc.h>
#include <string.h>
#include <uk/syscall.h>
#include <uk/arch/limits.h>
#include <uk/config.h>
#if CONFIG_PLAT_HYPERLIGHT
#include <unistd.h>
#include <uk/plat/time.h>
#include <vfscore/file.h>
#include <vfscore/vnode.h>
#include <vfscore/dentry.h>
struct vfscore_file;
extern int fget(int fd, struct vfscore_file **out_fp);
extern int fdrop(struct vfscore_file *fp);
extern int ramfs_get_file_buffer(struct vnode *vp,
				 const void **out_buf, size_t *out_size);

/* Finer-grained mmap timing: break the Hyperlight mmap hot path into
 * VMA bookkeeping, the per-page demand-map loop, and the pread of the
 * file content. Populated on every mmap call, exported with default
 * visibility so plat/hyperlight/syscall_profile.c can read them out
 * at dump time.
 */
#define MMAP_TIMING_EXPORT \
	__attribute__((visibility("default"))) __attribute__((used))

MMAP_TIMING_EXPORT __u64 mmap_timing_calls;
MMAP_TIMING_EXPORT __u64 mmap_timing_bookkeep_ns;
MMAP_TIMING_EXPORT __u64 mmap_timing_pgloop_ns;
MMAP_TIMING_EXPORT __u64 mmap_timing_pread_ns;
MMAP_TIMING_EXPORT __u64 mmap_timing_pgloop_pages;
MMAP_TIMING_EXPORT __u64 mmap_timing_pread_bytes;
#endif


#if CONFIG_PLAT_HYPERLIGHT
/*
 * On Hyperlight, mmap allocations use virtual addresses above the guest
 * heap, backed on demand by scratch memory pages.  This avoids consuming
 * buddy allocator memory for huge PROT_NONE reservations (Go's runtime
 * reserves GBs of virtual address space for page-allocator summary tables)
 * and eliminates the partial-backing bug where MAP_FIXED commits within a
 * partially-backed PROT_NONE range returned an address with no page table
 * entry, causing an unhandled "not present" page fault.
 */
static __u64 mmap_virt_next = 0x800000000ULL; /* 32 GiB — above any heap */

extern int cow_demand_map_page(__u64 gva);
extern int cow_demand_map_page_ex(__u64 gva, int zero_data);
extern int cow_map_contiguous(__u64 gva_base, __sz n_pages, int zero_data);
#endif

struct mmap_addr {
	void *begin;
	void *end;
	unsigned long num_pages; /* pages passed to uk_palloc; used by munmap */
	struct mmap_addr *next;
};

static struct mmap_addr *mmap_addr;

/**
 * This is not a correct implementation of mmap. It is just a trick that works
 * for Go but it needs to be revisited. Instead of mapping, it allocates len
 * bytes of memory and stores the beginninig and the end of that memory chunk
 * in struct mmap_addr. At first it checks if addr belongs to one of the memory
 * chunks that have been allocated in a previous call of mmap. If that is the
 * case addr is the return value. Otherwise a new memory block is allocated and
 * the return value is a pointer to the beginninig of that block.
 *
 * Go uses mmap always with:
 * @prot   =	either PROT_NONE or PROT_READ|PROT_WRITE,
 * @flags  =	as MAP_ANON|MAP_PRIVATE, or MAP_FIXED|MAP_ANON|MAP_PRIVATE
 *		or MAP_NORESERVE|MAP_ANON|MAP_PRIVATE
 * @fildes =	-1
 * @off    =	0
 *
 */

UK_SYSCALL_DEFINE(void*, mmap, void*, addr, size_t, len, int, prot,
		int, flags, int, fildes, off_t, off)
{
	struct mmap_addr *tmp = mmap_addr, *last = NULL, *new = NULL;

	if (!len) {
		errno = EINVAL;
		return (void *) -1;
	}

#if CONFIG_PLAT_HYPERLIGHT
	/* On Hyperlight, accept file-backed mappings (fildes != -1) for the
	 * dynamic linker to load shared libraries.  Accept both MAP_PRIVATE
	 * and MAP_SHARED — in a unikernel there is only one address space,
	 * so SHARED semantics are identical to PRIVATE.
	 */
	if (!(flags & (MAP_PRIVATE | MAP_SHARED)))
		return MAP_FAILED;
#else
	/* Check if parameters match the ones that go use
	 * Otherwise return 0 (unimplemented mmap)
	 */
	if (fildes != -1 || off)
		return MAP_FAILED;
	if (!(prot & (PROT_READ|PROT_WRITE)) && (prot != 0))
		return MAP_FAILED;
	if (!(flags & (MAP_ANON|MAP_PRIVATE)) &&
			!(flags & (MAP_FIXED|MAP_ANON|MAP_PRIVATE)) &&
			!(flags & (MAP_NORESERVE|MAP_ANON|MAP_PRIVATE)))
		return MAP_FAILED;
#endif

	while (tmp) {
		if (addr) {
			if (addr >= tmp->begin && addr < tmp->end) {
#if CONFIG_PLAT_HYPERLIGHT
				/* MAP_FIXED commit within an existing reservation:
				 * demand-map each page from scratch so the virtual
				 * address is backed by a real, zeroed page.
				 */
				if ((flags & MAP_FIXED) && (prot != PROT_NONE)) {
					size_t pg_off;
					for (pg_off = 0; pg_off < len;
					     pg_off += __PAGE_SIZE)
						cow_demand_map_page(
							(__u64)addr + pg_off);
					/* File-backed: read content into the
					 * newly mapped pages.
					 * Anonymous: zero-fill BSS pages.
					 */
					if (fildes != -1)
						pread(fildes, addr, len, off);
					else
						memset(addr, 0, len);
				}
#endif
				return addr;
			}
		}
		last = tmp;
		tmp = tmp->next;
	}

	/* MAP_FIXED requires mapping at exactly addr. If addr is not in any
	 * existing allocation (checked above), we cannot satisfy this request.
	 * Return MAP_FAILED so callers can fall back to a hint-based allocation.
	 */
	if ((flags & MAP_FIXED) && addr) {
		errno = ENOMEM;
		return MAP_FAILED;
	}

	/* For hint-based (non-MAP_FIXED) calls with a non-NULL addr,
	 * ignore the hint and fall through to a fresh allocation.
	 * V8 passes address hints for PROT_NONE reservations that it
	 * later commits with MAP_FIXED; returning ENOMEM here would
	 * prevent V8 from reserving MemoryChunks.
	 */

#if CONFIG_PLAT_HYPERLIGHT
	/*
	 * On Hyperlight, all mmap allocations use virtual addresses above the
	 * guest heap.  Physical pages are demand-mapped from scratch memory:
	 *   - PROT_NONE: address space only; pages created on MAP_FIXED commit
	 *   - PROT_READ|PROT_WRITE: eagerly demand-mapped and zeroed now
	 */
	{
		size_t aligned_len = (len + __PAGE_SIZE - 1) & ~(__PAGE_SIZE - 1);
		void *mem = (void *)mmap_virt_next;
		mmap_virt_next += aligned_len;

		new = uk_malloc(uk_alloc_get_default(),
				sizeof(struct mmap_addr));
		if (!new) {
			mmap_virt_next -= aligned_len;
			errno = ENOMEM;
			return MAP_FAILED;
		}

		__u64 _t0 = ukplat_monotonic_clock();
		new->begin = mem;
		new->end = mem + len;
		new->num_pages = 0; /* virtual-only: no buddy pages to free */
		new->next = NULL;
		if (!mmap_addr)
			mmap_addr = new;
		else
			last->next = new;
		__u64 _t1 = ukplat_monotonic_clock();
		mmap_timing_bookkeep_ns += (_t1 - _t0);
		mmap_timing_calls++;

		/* For non-PROT_NONE, eagerly demand-map all pages. For
		 * file-backed mappings, skip the per-page zero-fill — the
		 * pread below overwrites every byte anyway, so the memset
		 * is pure memory-bandwidth waste. Saves ~0.4 us/page on a
		 * Python-import hot path that averages 40+ pages/mmap and
		 * 700+ mmaps per `import pandas`.
		 */
		if (prot != PROT_NONE) {
			/* Always zero the freshly-mapped pages. The earlier
			 * "skip zero-fill for file-backed mmaps" optimisation
			 * turned out to have two holes:
			 *   1) len past EOF had to be zeroed per POSIX (fixed
			 *      with a tail memset below), and
			 *   2) physical pages recycled from recently-munmapped
			 *      mappings still had the previous mapping's data,
			 *      producing non-zero bytes in the middle of the
			 *      new mapping where pread() never wrote.
			 * (2) was the root cause of powershell crashing during
			 * startup. Reverting to always-zero costs ~3-5 ms per
			 * `import pandas` (measured) which is cheap compared
			 * to the bugs it prevents. A future optimisation could
			 * zero only the physical pages not sourced from a clean
			 * pool, but for now correctness wins.
			 */
			int zero_data = 1;
			/* One contiguous physical allocation + single
			 * memset (for anonymous) + per-page PTE writes —
			 * replaces the N-per-page (alloc + memset + PTE
			 * write) that the naive loop was doing. Giant
			 * win on anonymous mmaps where the memset alone
			 * was eating 120+ ms per Python startup.
			 */
			cow_map_contiguous((__u64)mem,
					   aligned_len / __PAGE_SIZE,
					   zero_data);
			__u64 _t2 = ukplat_monotonic_clock();
			mmap_timing_pgloop_ns += (_t2 - _t1);
			mmap_timing_pgloop_pages +=
				aligned_len / __PAGE_SIZE;
			/* File-backed: read content into the mapped pages.
			 * Fast path: if the backing is a ramfs regular
			 * file (the overwhelming case for initrd-extracted
			 * .so files on Hyperlight), grab the raw rn_buf
			 * pointer and memcpy directly, skipping the
			 * vfscore pread → preadv → do_preadv → sys_read →
			 * ramfs_read → vfscore_uiomove chain. Falls back
			 * to plain pread() for anything that isn't a
			 * ramfs vnode.
			 *
			 * Tried non-temporal stores (movnti) to avoid
			 * write-allocate on the fresh scratch pages —
			 * didn't help; the copy is bandwidth-limited on
			 * memcpy's own throughput, not write-allocate.
			 * The remaining speedup in this path would come
			 * from zero-copy mapping of ramfs pages directly
			 * into the process VA, which needs page-aligned
			 * ramfs buffers.
			 */
			if (fildes != -1) {
				struct vfscore_file *fp = NULL;
				/* Bytes actually filled from the file. Any
				 * remaining [written, len) MUST be zeroed to
				 * satisfy POSIX — mmap regions past a file's
				 * EOF must read back as zero. We skipped the
				 * up-front per-page zero-fill for files (to
				 * save memory bandwidth on the common Python-
				 * import hot path), so the tail needs an
				 * explicit memset here. Missing this was the
				 * root cause of powershell and other heavy
				 * runtimes crashing with bogus pointer
				 * dereferences after mmap'ing a .so whose
				 * request length exceeds the file size.
				 */
				size_t written = 0;
#if CONFIG_LIBRAMFS
				const void *src = NULL;
				size_t src_size = 0;
				if (fget(fildes, &fp) == 0 && fp
				    && fp->f_dentry
				    && fp->f_dentry->d_vnode
				    && ramfs_get_file_buffer(
					    fp->f_dentry->d_vnode,
					    &src, &src_size) == 0) {
					size_t avail = (off >= (off_t)src_size)
						? 0
						: src_size - off;
					size_t n = len < avail ? len : avail;
					if (n)
						memcpy(mem,
						       (const char *)src + off,
						       n);
					written = n;
					fdrop(fp);
				} else {
					if (fp)
						fdrop(fp);
					ssize_t r = pread(fildes, mem, len, off);
					written = r > 0 ? (size_t)r : 0;
				}
#else
				/* No ramfs direct-buffer fast path available;
				 * fall back to pread() through whatever vfs
				 * (cpiovfs, hostfs, …) the image enables.
				 */
				(void)fp;
				{
					ssize_t r = pread(fildes, mem, len, off);
					written = r > 0 ? (size_t)r : 0;
				}
#endif /* CONFIG_LIBRAMFS */
				if (written < len)
					memset((char *)mem + written, 0,
					       len - written);
				mmap_timing_pread_bytes += len;
			}
			__u64 _t3 = ukplat_monotonic_clock();
			mmap_timing_pread_ns += (_t3 - _t2);
		}

		return mem;
	}
#else /* !CONFIG_PLAT_HYPERLIGHT */
	{
		/* Original buddy-based allocation for non-Hyperlight platforms */
		unsigned long num_pages =
			(len + __PAGE_SIZE - 1) >> __PAGE_SHIFT;
		void *mem = uk_palloc(uk_alloc_get_default(), num_pages);
		if (!mem) {
			errno = ENOMEM;
			return MAP_FAILED;
		}

		new = uk_malloc(uk_alloc_get_default(),
				sizeof(struct mmap_addr));
		if (!new) {
			uk_pfree(uk_alloc_get_default(), mem, num_pages);
			errno = ENOMEM;
			return MAP_FAILED;
		}

		memset(mem, 0, len);

		new->begin = mem;
		new->end = mem + len;
		new->num_pages = num_pages;
		new->next = NULL;
		if (!mmap_addr)
			mmap_addr = new;
		else
			last->next = new;
		return mem;
	}
#endif /* !CONFIG_PLAT_HYPERLIGHT */
}

/*
 * munmap frees len bytes os memory starting from addr.
 * addr needs to be a pointer to a memory block that has been allocated from
 * mmap. If len has the same value with the size of the memory block that has
 * been allocated from mmap the struct mmap_addr counterpart is destroyed.
 * Otherwise, we do nothing and unfortunately have to leak the memory. This is
 * inherent to the way this library works as it implements mmap on top of
 * ukalloc which does not provide any means to partially free memory or do a
 * realloc with the guarantee of not moving the existing allocation.
 */

UK_SYSCALL_DEFINE(int, munmap, void*, addr, size_t, len)
{
	struct mmap_addr *tmp = mmap_addr, *prev = NULL;

	if (!len) {
		errno = EINVAL;
		return -1;
	}

	if (!addr)
		return 0;

	while (tmp) {
		if (addr >= tmp->begin && addr < tmp->end) {
			/* We cannot release only some part of the allocation.
			 * In that case, pretend we have done it and hope
			 * everything will be fine
			 */
			if (len != (__uptr)tmp->end - (__uptr)tmp->begin)
				return 0;

			/* Caller wants to unmap the whole region. Easy! */
			if (!prev)
				mmap_addr = tmp->next;
			else
				prev->next = tmp->next;

			unsigned long np = tmp->num_pages;
			uk_free(uk_alloc_get_default(), tmp);
			if (np > 0)
				uk_pfree(uk_alloc_get_default(), addr, np);
			/* np == 0: virtual-only region, no buddy pages to free */
			return 0;
		}

		prev = tmp;
		tmp = tmp->next;
	}

	/* No matching region found. But it is ok anyway */
	return 0;
}

UK_LLSYSCALL_R_DEFINE(int, mremap, void*, old_address, size_t, old_size,
		size_t, new_size, int, flags, unsigned long, arg)
{
	(void)arg;

#if CONFIG_PLAT_HYPERLIGHT
	/* Without paging we cannot actually grow or move regions.
	 *
	 * When MREMAP_MAYMOVE is set and the size changes this is a
	 * real resize request (e.g. glibc malloc's mremap_chunk).
	 * Returning -ENOMEM makes glibc fall back to mmap+memcpy+munmap.
	 *
	 * When MREMAP_MAYMOVE is *not* set the caller is probing
	 * whether pages are mapped (.NET CLR / musl stack detection).
	 * Returning the old address lets the probe succeed and terminate.
	 */
	if (new_size != old_size && (flags & 1))  /* MREMAP_MAYMOVE = 1 */
		return -ENOMEM;
	return (long)old_address;
#else
	(void)flags;
	/*
	 * musl uses mremap(addr, 4K, 8K, 0) to probe whether pages
	 * are mapped (stack boundary detection).  Return success for
	 * addresses in the kernel/heap range or the mmap virtual range
	 * so the probe terminates at the correct boundary.
	 */
	__u64 addr = (__u64)old_address;
	if (addr < 0x400b1000 || addr >= 0x800000000ULL)
		return (long)old_address;
	return -ENOMEM;
#endif
}

#if UK_LIBC_SYSCALLS
void *mremap(void *old_address, size_t old_size,
	     size_t new_size, int flags, ...)
{
#if CONFIG_PLAT_HYPERLIGHT
	if (new_size != old_size && (flags & 1)) {  /* MREMAP_MAYMOVE = 1 */
		errno = ENOMEM;
		return MAP_FAILED;
	}
	return old_address;
#else
	(void)old_size;
	(void)new_size;
	(void)flags;
	__u64 addr = (__u64)old_address;
	if (addr < 0x400b1000 || addr >= 0x800000000ULL)
		return old_address;
	errno = ENOMEM;
	return MAP_FAILED;
#endif
}
#endif

UK_SYSCALL_R_DEFINE(int, madvise, void*, addr, size_t, length, int, advice)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, mprotect, void*, addr, size_t, len, int, prot)
{
#if CONFIG_PLAT_HYPERLIGHT
	/* When transitioning from PROT_NONE to readable/writable, back the
	 * virtual pages with scratch memory.  This is needed because V8
	 * uses mmap(PROT_NONE) + mprotect(PROT_READ|PROT_WRITE) to commit
	 * memory regions.
	 */
	if (prot != PROT_NONE) {
		size_t pg_off;
		size_t aligned_len = (len + __PAGE_SIZE - 1) & ~(__PAGE_SIZE - 1);
		__u64 base = (__u64)addr & ~(__PAGE_SIZE - 1);
		for (pg_off = 0; pg_off < aligned_len; pg_off += __PAGE_SIZE)
			cow_demand_map_page(base + pg_off);
	}
#endif
	return 0;
}
