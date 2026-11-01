/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <string.h>

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/falloc.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-file-vma.h>
#include <uk/vmem/vma_ops.h>
#include <uk/arch/limits.h>
#include <uk/paging.h>
#include <uk/refcount.h>
#include <uk/print.h>

#include "fdio-impl.h"

/* A split VMA shares this one pin acquisition. Its last VMA releases it.
 * Unmapped subranges remain pinned until that last reference; no disk I/O or
 * frame allocation is permitted in the page-fault callback.
 */
struct file_backing {
	__atomic refs;
	__off offset;
	__sz acquired;
	__paddr_t pages[];
};

struct file_vma {
	struct uk_vma base;

	/** Open file description mapped by this VMA */
	struct uk_ofile *of;
	/** Start offset describing what position in the file is mapped */
	__off offset;
	struct file_backing *shared;
};

#ifdef CONFIG_LIBUKVMEM_FILE_BASE
static
__vaddr_t file_vma_op_get_base(struct uk_vas *vas __unused,
			       void *data __unused,
			       unsigned long flags __unused)
{
	return CONFIG_LIBUKVMEM_FILE_BASE;
}
#else /* !CONFIG_LIBUKVMEM_FILE_BASE */
#define file_vma_op_get_base __NULL
#endif /* !CONFIG_LIBUKVMEM_FILE_BASE */

static
int file_backing_new(struct uk_vas *vas, const struct uk_file *f,
		     __off offset, __sz len, struct file_backing **out)
{
	const __sz count = len / UK_PAGING_PAGE_SIZE;
	struct file_backing *b;
	struct iovec iov;
	__sz done;
	__ssz r;
	int rc = 0;

	if (count > (__SZ_MAX - sizeof(*b)) / sizeof(b->pages[0]))
		return -ENOMEM;
	b = uk_malloc(vas->a, sizeof(*b) + count * sizeof(b->pages[0]));
	if (!b)
		return -ENOMEM;
	uk_refcount_init(&b->refs, 1);
	b->offset = offset;
	b->acquired = 0;
	uk_file_wlock(f);
	while (b->acquired < len) {
		r = uk_file_mem(f, UKFILE_MEM_ACQUIRE_PINNED,
				offset + b->acquired, len - b->acquired,
				__NULL, 0);
		if (r <= 0) {
			rc = r ? r : -ENXIO;
			goto fail;
		}
		if (!UK_PAGING_PAGE_ALIGNED(r) || (__sz)r > len - b->acquired)
			UK_CRASH("Invalid pinned file memory span: %zd", r);
		b->acquired += r;
	}
	for (done = 0; done < len; done += UK_PAGING_PAGE_SIZE) {
		r = uk_file_mem(f, UKFILE_MEM_RETRIEVE, offset + done,
				UK_PAGING_PAGE_SIZE, &iov, 1);
		if (r != 1 || !iov.iov_base || !iov.iov_len ||
		    !UK_PAGING_PAGE_ALIGNED((__vaddr_t)iov.iov_base)) {
			rc = r < 0 ? r : -EIO;
			goto fail;
		}
		/* PINNED guarantees resident, physically stable pages. The
		 * last partial file page is still a complete backing page.
		 */
		b->pages[done / UK_PAGING_PAGE_SIZE] =
			uk_paging_virt_to_phys((__vaddr_t)iov.iov_base);
		if (b->pages[done / UK_PAGING_PAGE_SIZE] ==
		    UK_PAGING_PADDR_INV) {
			rc = -EIO;
			goto fail;
		}
	}
	uk_file_wunlock(f);
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	uk_pr_info("Pinned shared file offset=%lld bytes=%zu pages=%zu\n",
		   (long long)offset, len, count);
#endif
	*out = b;
	return 0;
fail:
	if (b->acquired)
		uk_file_mem(f, UKFILE_MEM_RELEASE, offset, b->acquired,
			    __NULL, 0);
	uk_file_wunlock(f);
	uk_free(vas->a, b);
	return rc;
}

static
int file_vma_op_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		    __sz len, void *data, unsigned long attr,
		    unsigned long *flags, struct uk_vma **vma)
{
	struct uk_fdio_vma_args *args = data;
	struct file_vma *fvma;
	unsigned int mode;

	UK_ASSERT(data);
	UK_ASSERT(args->of);
	UK_ASSERT(args->offset >= 0);
	UK_ASSERT(UK_PAGING_PAGE_ALIGNED(args->offset));
	UK_ASSERT(vma);

	/* File I/O and pin acquisition run in syscall/thread context. Only
	 * populated, already pinned physical pages reach the fault callback.
	 */
	*flags |= UK_VMA_MAP_POPULATE;
	mode = args->of->mode;
	if (!_CAN_READ(mode))
		return -EACCES;
	if ((*flags & UK_VMA_FILE_SHARED) &&
	    (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE) && !_CAN_WRITE(mode))
		return -EACCES;

	fvma = uk_malloc(vas->a, sizeof(*fvma));
	if (unlikely(!fvma))
		return -ENOMEM;
	fvma->shared = __NULL;
	if (*flags & UK_VMA_FILE_SHARED) {
		int rc = file_backing_new(vas, args->of->file,
					 args->offset, len, &fvma->shared);

		if (rc) {
			uk_free(vas->a, fvma);
			return rc;
		}
	}
	uk_ofile_acquire(args->of);
	fvma->of = args->of;
	fvma->offset = args->offset;
	fvma->base.name = uk_ofile_name(args->of, "file");

	*vma = &fvma->base;
	return 0;
}

static
void file_vma_op_destroy(struct uk_vma *vma)
{
	struct file_vma *fvma = (struct file_vma *)vma;

	UK_ASSERT(fvma->of);
	if (fvma->shared && uk_refcount_release(&fvma->shared->refs)) {
		const struct uk_file *f = fvma->of->file;
		struct file_backing *b = fvma->shared;

		uk_file_wlock(f);
		uk_file_mem(f, UKFILE_MEM_RELEASE, b->offset, b->acquired,
			    __NULL, 0);
		uk_file_wunlock(f);
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
		uk_pr_info("Released shared file offset=%lld bytes=%zu\n",
			   (long long)b->offset, b->acquired);
#endif
		uk_free(vma->vas->a, b);
	}
	uk_ofile_release(fvma->of);
}

static inline
__ssz file_vma_readin(const struct uk_file *f, __vaddr_t buf, __sz len,
		      __off offset)
{
	__ssz read = 0;
	__ssz r;

	uk_file_rlock(f);
	/* Read up until done, error, or confirmed EOF */
	do {
		r = uk_file_read(f, &(struct iovec){ (void *)buf, len }, 1,
				 offset, 0);
		if (unlikely(r <= 0))
			break;
		buf += r;
		len -= r;
		offset += r;
		read += r;
	} while (len);
	uk_file_runlock(f);
	/* Only return success if done reading or EOF */
	if (!len || !r)
		return read;
	/* We should only get here on I/O error */
	UK_ASSERT(r < 0);
	return r;
}

static
int file_vma_op_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct file_vma *fvma = (struct file_vma *)vma;
	struct uk_pagetable *const pt = vma->vas->pt;
	unsigned long pages = fault->len / UK_PAGING_PAGE_SIZE;
	__paddr_t paddr = UK_PAGING_PADDR_ANY;
	__vaddr_t vaddr;
	__off off;
	__ssz nbytes;
	int rc;

	UK_ASSERT(UK_PAGING_PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->len == UK_PAGING_PAGE_Lx_SIZE(fault->level));
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);

	if (fvma->shared) {
		__sz pos;

		/* mapx retries at base-page granularity when a larger physical
		 * run cannot be supplied. There is no requirement that file
		 * backing pages be contiguous.
		 */
		if (fault->level != UK_PAGING_PAGE_LEVEL)
			return -ENOMEM;
		pos = fvma->offset - fvma->shared->offset +
			fault->vbase - vma->start;
		if (pos >= fvma->shared->acquired)
			return -ENXIO;
		fault->paddr = fvma->shared->pages[pos / UK_PAGING_PAGE_SIZE];
		return 0;
	}

	rc = pt->fa->falloc(pt->fa, &paddr, pages, FALLOC_FLAG_ALIGNED);
	if (unlikely(rc))
		return rc;

	vaddr = uk_paging_page_kmap(pt, paddr, pages, 0);
	if (unlikely(vaddr == UK_PAGING_VADDR_INV)) {
		pt->fa->ffree(pt->fa, paddr, pages);
		return -ENOMEM;
	}

	off = (fault->vbase - vma->start) + fvma->offset;

	nbytes = file_vma_readin(fvma->of->file, vaddr, fault->len, off);
	if (unlikely(nbytes < 0)) {
		uk_paging_page_kunmap(pt, vaddr, pages, 0);
		pt->fa->ffree(pt->fa, paddr, pages);
		return nbytes;
	}

	/* Fill the remaining space with zeros */
	UK_ASSERT(fault->len >= (__sz)nbytes);

	memset((void *)(vaddr + nbytes), 0, fault->len - nbytes);
	uk_paging_page_kunmap(pt, vaddr, pages, 0);

	fault->paddr = paddr;
	return 0;
}

static
int file_vma_op_split(struct uk_vma *vma, __vaddr_t vaddr,
		      struct uk_vma **new_vma)
{
	struct file_vma *fvma = (struct file_vma *)vma;
	struct file_vma *v;
	__off off = vaddr - vma->start;

	UK_ASSERT(vaddr >= vma->start);
	UK_ASSERT(fvma->offset <= __OFF_MAX - off);
	UK_ASSERT(new_vma);

	v = uk_malloc(vma->vas->a, sizeof(*v));
	if (unlikely(!v))
		return -ENOMEM;

	v->offset = fvma->offset + off;
	uk_ofile_acquire(fvma->of);
	v->of = fvma->of;
	v->shared = fvma->shared;
	if (v->shared)
		uk_refcount_acquire(&v->shared->refs);

	*new_vma = &v->base;
	return 0;
}

static
int file_vma_op_merge(struct uk_vma *vma, struct uk_vma *next)
{
	struct file_vma *fvma = (struct file_vma *)vma;
	struct file_vma *nvma = (struct file_vma *)next;
	__off off;

	UK_ASSERT(next->start == vma->end);
	UK_ASSERT(next->start > vma->start);

	/* Only merge if this is the same open file description ... */
	if (fvma->of != nvma->of || fvma->shared != nvma->shared)
		return -EPERM;
	/* ... and the VMAs map contiguous file ranges */
	off = next->start - vma->start;
	if (nvma->offset != fvma->offset + off)
		return -EPERM;
	/* We release the file in the destructor */
	return 0;
}

static
int file_vma_op_set_attr(struct uk_vma *vma, unsigned long attr)
{
	struct file_vma *fvma = (struct file_vma *)vma;

	if (fvma->shared && (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE) &&
	    !_CAN_WRITE(fvma->of->mode))
		return -EACCES;
	if ((attr & UK_PAGING_PAGE_ATTR_PROT_READ) &&
	    !_CAN_READ(fvma->of->mode))
		return -EACCES;
	/* Default handler */
	return uk_vma_op_set_attr(vma, attr);
}

static
int file_vma_op_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	struct file_vma *fvma = (struct file_vma *)vma;

	if (!fvma->shared)
		return uk_vma_op_unmap(vma, vaddr, len);
	/* The file owns these physical pages. Only remove our MMU aliases;
	 * destroy releases the pin after the final split VMA is gone.
	 */
	return uk_paging_page_unmap(vma->vas->pt, vaddr,
		len / UK_PAGING_PAGE_SIZE, UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
}

static
int file_vma_op_sync(struct uk_vma *vma, __vaddr_t vaddr __unused,
		     __sz len __unused)
{
	struct file_vma *fvma = (struct file_vma *)vma;
	const struct uk_file *f = fvma->of->file;
	int rc;

	if (!fvma->shared)
		return 0;
	/* The existing file contract flushes the whole inode, which includes
	 * the requested range. Shared aliases use this same authoritative
	 * memory, not independent copies requiring invalidation.
	 */
	uk_file_wlock(f);
	rc = uk_file_ctl(f, UKFILE_CTL_FILE, UKFILE_CTL_FILE_SYNC, 1, 0, 0);
	uk_file_wunlock(f);
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	uk_pr_info("Shared file sync offset=%lld bytes=%zu result=%d\n",
		   (long long)fvma->offset, uk_vma_len(vma), rc);
#endif
	return rc;
}

static
int file_vma_op_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
		       unsigned long advice)
{
	struct file_vma *fvma = (struct file_vma *)vma;

	/* Retain pinned backing and its aliases. The generic anonymous
	 * DONTNEED path frees frames and is unsuitable for file-owned memory.
	 */
	if (fvma->shared)
		return 0;
	return uk_vma_op_advise(vma, vaddr, len, advice);
}

const struct uk_vma_ops uk_fdio_vma_ops = {
	.get_base	= file_vma_op_get_base,
	.new		= file_vma_op_new,
	.destroy	= file_vma_op_destroy,
	.fault		= file_vma_op_fault,
	.unmap		= file_vma_op_unmap,
	.sync		= file_vma_op_sync,
	.split		= file_vma_op_split,
	.merge		= file_vma_op_merge,
	.set_attr	= file_vma_op_set_attr,
	.advise		= file_vma_op_advise,
};
