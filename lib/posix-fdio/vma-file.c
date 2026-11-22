/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <uk/config.h>
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
#include <uk/print.h>
#endif
#if CONFIG_LIBUKVMEM_COW_VMA
#include <uk/alloc.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/list.h>
#include <uk/spinlock.h>
#endif
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

#if CONFIG_LIBUKVMEM_COW_VMA
/* Canonical identity across independent descriptions of the same file.
 * Holding its first file reference prevents node identity reuse while a
 * mapping or a futex key still names that node. No registry lock reaches
 * file I/O or destruction.
 */
struct file_identity {
	struct uk_list_head entry;
	__atomic refs;
	struct uk_alloc *a;
	const struct uk_file *file;
	const void *vol;
	void *node;
	const struct uk_file *unique;
};

static UK_LIST_HEAD(file_identities);
static uk_spinlock file_identity_lock = UK_SPINLOCK_INITIALIZER();

static struct file_identity *file_identity_find(const struct uk_file *f)
{
	const struct uk_file *unique = f->vol || f->node ? NULL : f;
	struct file_identity *id;

	uk_list_for_each_entry(id, &file_identities, entry)
		if (id->vol == f->vol && id->node == f->node &&
		    id->unique == unique)
			return id;
	return NULL;
}

static void file_identity_acquire(const void *object)
{
	struct file_identity *id = (struct file_identity *)object;

	if (id)
		uk_refcount_acquire(&id->refs);
}

static void file_identity_release(const void *object)
{
	struct file_identity *id = (struct file_identity *)object;
	unsigned long irqf;
	int last;

	if (!id || uk_refcount_release_if_not_last(&id->refs))
		return;
	irqf = uk_lcpu_save_irqf();
	uk_spin_lock(&file_identity_lock);
	last = uk_refcount_release(&id->refs);
	if (last)
		uk_list_del_init(&id->entry);
	uk_spin_unlock(&file_identity_lock);
	uk_lcpu_restore_irqf(irqf);
	if (last) {
		UK_ASSERT(!uk_lcpu_irqs_disabled());
		uk_file_release(id->file);
		uk_free(id->a, id);
	}
}

static struct file_identity *file_identity_for(const struct uk_file *f)
{
	struct uk_alloc *a = uk_vas_get_kernel()->a;
	struct file_identity *id, *fresh;
	unsigned long irqf;

	irqf = uk_lcpu_save_irqf();
	uk_spin_lock(&file_identity_lock);
	id = file_identity_find(f);
	file_identity_acquire(id);
	uk_spin_unlock(&file_identity_lock);
	uk_lcpu_restore_irqf(irqf);
	if (id)
		return id;
	fresh = uk_malloc(a, sizeof(*fresh));
	if (!fresh)
		return NULL;
	UK_INIT_LIST_HEAD(&fresh->entry);
	uk_refcount_init(&fresh->refs, 1);
	fresh->a = a;
	fresh->file = f;
	fresh->vol = f->vol;
	fresh->node = f->node;
	fresh->unique = f->vol || f->node ? NULL : f;
	uk_file_acquire(f);
	irqf = uk_lcpu_save_irqf();
	uk_spin_lock(&file_identity_lock);
	id = file_identity_find(f);
	if (id) {
		file_identity_acquire(id);
	} else {
		uk_list_add_tail(&fresh->entry, &file_identities);
		id = fresh;
	}
	uk_spin_unlock(&file_identity_lock);
	uk_lcpu_restore_irqf(irqf);
	if (id != fresh) {
		uk_file_release(fresh->file);
		uk_free(a, fresh);
	}
	return id;
}
#endif

struct file_vma {
	struct uk_vma base;

	/** Open file description mapped by this VMA */
	struct uk_ofile *of;
	/** Start offset describing what position in the file is mapped */
	__off offset;
	struct file_backing *shared;
	__paddr_t *priv_pages;
	__sz priv_npages;
	__sz priv_taken;
#if CONFIG_LIBUKVMEM_COW_VMA
	struct uk_vma_cow_source *source;
	__sz source_first;
	struct file_identity *identity;
#endif
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
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
			uk_pr_err("SMP_VM_IO stage=file_pin rc=%d raw=%ld off=%lld done=%zu len=%zu mem=%p\n",
				  rc, (long)r, (long long)offset, b->acquired,
				  len, (const void *)f->ops->mem);
#endif
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
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
			uk_pr_err("SMP_VM_IO stage=file_retrieve rc=%d raw=%ld off=%lld done=%zu len=%zu\n",
				  rc, (long)r, (long long)offset, done, len);
#endif
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

static inline
__ssz file_vma_readin_locked(const struct uk_file *f, __vaddr_t buf, __sz len,
			     __off offset)
{
	__ssz read = 0;
	__ssz r;

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
	/* Only return success if done reading or EOF */
	if (!len || !r)
		return read;
	/* We should only get here on I/O error */
	UK_ASSERT(r < 0);
	return r;
}

static
void file_priv_pages_free(struct uk_vas *vas, __paddr_t *pages, __sz npages)
{
	struct uk_pagetable *pt = vas->pt;
	__sz i;

	if (!pages)
		return;
	for (i = 0; i < npages; i++) {
		if (pages[i] != UK_PAGING_PADDR_INV)
			pt->fa->ffree(pt->fa, pages[i], 1);
	}
	uk_free(vas->a, pages);
}

static
int file_priv_pages_new(struct uk_vas *vas, const struct uk_file *f,
			__off offset, __sz len, unsigned long flags,
			__paddr_t **out_pages, __sz *out_npages)
{
	const __sz count = len / UK_PAGING_PAGE_SIZE;
	struct uk_pagetable *pt = vas->pt;
	__paddr_t *pages;
	__sz i;
	int rc = 0;

	if (!count || count > __SZ_MAX / sizeof(*pages))
		return -ENOMEM;
	pages = uk_malloc(vas->a, count * sizeof(*pages));
	if (!pages)
		return -ENOMEM;
	for (i = 0; i < count; i++)
		pages[i] = UK_PAGING_PADDR_INV;

	if (!(flags & UK_VMA_MAP_UNINITIALIZED))
		uk_file_rlock(f);

	for (i = 0; i < count; i++) {
		__paddr_t paddr = UK_PAGING_PADDR_ANY;

		rc = pt->fa->falloc(pt->fa, &paddr, 1, FALLOC_FLAG_ALIGNED);
		if (unlikely(rc))
			goto fail;

		if (!(flags & UK_VMA_MAP_UNINITIALIZED)) {
			__vaddr_t kvaddr;
			__ssz nbytes;
			__off off = offset + (__off)(i * UK_PAGING_PAGE_SIZE);

			kvaddr = uk_paging_page_kmap(pt, paddr, 1, 0);
			if (unlikely(kvaddr == UK_PAGING_VADDR_INV)) {
				pt->fa->ffree(pt->fa, paddr, 1);
				rc = -ENOMEM;
				goto fail;
			}
			nbytes = file_vma_readin_locked(f, kvaddr,
							UK_PAGING_PAGE_SIZE,
							off);
			if (unlikely(nbytes < 0)) {
				uk_paging_page_kunmap(pt, kvaddr, 1, 0);
				pt->fa->ffree(pt->fa, paddr, 1);
				rc = (int)nbytes;
				goto fail;
			}
			UK_ASSERT(UK_PAGING_PAGE_SIZE >= (__sz)nbytes);
			memset((void *)(kvaddr + nbytes), 0,
			       UK_PAGING_PAGE_SIZE - (__sz)nbytes);
			uk_paging_page_kunmap(pt, kvaddr, 1, 0);
		}
		pages[i] = paddr;
	}

	if (!(flags & UK_VMA_MAP_UNINITIALIZED))
		uk_file_runlock(f);

	*out_pages = pages;
	*out_npages = count;
	return 0;

fail:
	if (!(flags & UK_VMA_MAP_UNINITIALIZED))
		uk_file_runlock(f);
	file_priv_pages_free(vas, pages, count);
	return rc;
}

static
int file_vma_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		 __sz len, void *data, unsigned long attr,
		 unsigned long *flags, struct uk_vma **vma,
		 bool cow __maybe_unused)
{
	struct uk_fdio_vma_args *args = data;
	struct file_vma *fvma;
	unsigned int mode;
	int rc;

	UK_ASSERT(data);
	UK_ASSERT(args->of);
	UK_ASSERT(args->offset >= 0);
	UK_ASSERT(UK_PAGING_PAGE_ALIGNED(args->offset));
	UK_ASSERT(vma);

#if CONFIG_LIBUKVMEM_COW_VMA
	if (cow && (!vas->cow || (*flags & UK_VMA_FILE_SHARED)))
		return -EINVAL;
	if (cow && UK_VMA_MAP_SIZE_TO_ORDER(*flags) &&
	    UK_VMA_MAP_SIZE_TO_ORDER(*flags) != UK_PAGING_PAGE_SHIFT)
		return -ENOTSUP;
#endif
	/* File I/O and pin acquisition run in syscall/thread context. Only
	 * populated, already resident physical pages reach the fault callback.
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
	fvma->priv_pages = __NULL;
	fvma->priv_npages = 0;
	fvma->priv_taken = 0;
#if CONFIG_LIBUKVMEM_COW_VMA
	fvma->source = NULL;
	fvma->source_first = 0;
	fvma->identity = NULL;
#endif
	if (*flags & UK_VMA_FILE_SHARED) {
		rc = file_backing_new(vas, args->of->file,
				      args->offset, len, &fvma->shared);
		if (rc) {
			uk_free(vas->a, fvma);
			return rc;
		}
	} else {
		rc = file_priv_pages_new(vas, args->of->file, args->offset, len,
					 *flags, &fvma->priv_pages,
					 &fvma->priv_npages);
		if (rc) {
			uk_free(vas->a, fvma);
			return rc;
		}
#if CONFIG_LIBUKVMEM_COW_VMA
		if (cow) {
			rc = uk_vma_cow_source_take(vas,
						    fvma->priv_pages,
						    fvma->priv_npages,
						    &fvma->source);
			/* Free unconsumed frames and the vector. */
			file_priv_pages_free(vas, fvma->priv_pages,
					     fvma->priv_npages);
			fvma->priv_pages = NULL;
			fvma->priv_npages = 0;
			if (rc) {
				uk_free(vas->a, fvma);
				return rc;
			}
		}
#endif
	}
	uk_ofile_acquire(args->of);
	fvma->of = args->of;
	fvma->offset = args->offset;
	fvma->base.name = uk_ofile_name(args->of, "file");

	*vma = &fvma->base;
	return 0;
}

static int file_vma_op_new(struct uk_vas *vas, __vaddr_t addr, __sz len,
			   void *data, unsigned long attr,
			   unsigned long *flags, struct uk_vma **out)
{
	return file_vma_new(vas, addr, len, data, attr, flags, out, false);
}

#if CONFIG_LIBUKVMEM_COW_VMA
static int file_cow_new(struct uk_vas *vas, __vaddr_t addr, __sz len,
			void *data, unsigned long attr,
			unsigned long *flags, struct uk_vma **out)
{
	return file_vma_new(vas, addr, len, data, attr, flags, out, true);
}
#endif

static
void file_vma_op_destroy(struct uk_vma *vma)
{
	struct file_vma *fvma = (struct file_vma *)vma;

	UK_ASSERT(fvma->of);
#if CONFIG_LIBUKVMEM_COW_VMA
	uk_vma_cow_source_put(fvma->source);
#endif
	if (fvma->priv_pages) {
		file_priv_pages_free(vma->vas, fvma->priv_pages,
				     fvma->priv_npages);
		fvma->priv_pages = __NULL;
		fvma->priv_npages = 0;
		fvma->priv_taken = 0;
	}
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
#if CONFIG_LIBUKVMEM_COW_VMA
	file_identity_release(fvma->identity);
#endif
}

static
int file_vma_op_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct file_vma *fvma = (struct file_vma *)vma;
	__sz idx;

	UK_ASSERT(UK_PAGING_PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->len == UK_PAGING_PAGE_Lx_SIZE(fault->level));
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);

	/* mapx retries at base-page granularity when a larger physical run
	 * cannot be supplied. File-backed frames are tracked per 4 KiB page.
	 */
	if (fault->level != UK_PAGING_PAGE_LEVEL)
		return -ENOMEM;

	if (fvma->shared) {
		__sz pos = fvma->offset - fvma->shared->offset +
			   fault->vbase - vma->start;

		if (pos >= fvma->shared->acquired)
			return -ENXIO;
		fault->paddr = fvma->shared->pages[pos / UK_PAGING_PAGE_SIZE];
		return 0;
	}

	idx = (fault->vbase - vma->start) / UK_PAGING_PAGE_SIZE;
	if (unlikely(!fvma->priv_pages || idx >= fvma->priv_npages ||
		     fvma->priv_pages[idx] == UK_PAGING_PADDR_INV))
		return -EIO;

	fault->paddr = fvma->priv_pages[idx];
	fvma->priv_pages[idx] = UK_PAGING_PADDR_INV;
	fvma->priv_taken++;
	if (fvma->priv_taken == fvma->priv_npages) {
		uk_free(vma->vas->a, fvma->priv_pages);
		fvma->priv_pages = __NULL;
		fvma->priv_npages = 0;
		fvma->priv_taken = 0;
	}
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
	UK_ASSERT(!fvma->priv_pages);

	v = uk_malloc(vma->vas->a, sizeof(*v));
	if (unlikely(!v))
		return -ENOMEM;

	v->offset = fvma->offset + off;
	uk_ofile_acquire(fvma->of);
	v->of = fvma->of;
	v->shared = fvma->shared;
	v->priv_pages = __NULL;
	v->priv_npages = 0;
	v->priv_taken = 0;
#if CONFIG_LIBUKVMEM_COW_VMA
	v->source = uk_vma_cow_source_get(fvma->source);
	v->source_first = fvma->source_first + off / UK_PAGING_PAGE_SIZE;
	v->identity = fvma->identity;
	file_identity_acquire(v->identity);
#endif
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

	if (fvma->priv_pages || nvma->priv_pages)
		return -EPERM;
	/* Only merge if this is the same open file description ... */
	if (fvma->of != nvma->of || fvma->shared != nvma->shared)
		return -EPERM;
#if CONFIG_LIBUKVMEM_COW_VMA
	if (fvma->source != nvma->source ||
	    (fvma->source && fvma->source_first +
	     uk_vma_len(vma) / UK_PAGING_PAGE_SIZE != nvma->source_first))
		return -EPERM;
#endif
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

	/* Retain pinned backing and pre-populated file pages so hardware #PF
	 * never attempts blocking file I/O with interrupts disabled.
	 */
	if (fvma->shared || (advice & UK_VMA_ADV_DONTNEED))
		return 0;
	return uk_vma_op_advise(vma, vaddr, len, advice);
}

#if CONFIG_LIBUKVMEM_COW_VMA
static int file_cow_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct file_vma *fvma = (struct file_vma *)vma;
	__sz index;

	if (fault->level != UK_PAGING_PAGE_LEVEL)
		return -ENOMEM;
	index = fvma->source_first +
		(fault->vbase - vma->start) / UK_PAGING_PAGE_SIZE;
	return uk_vma_cow_source_fault(vma, fault, fvma->source, index);
}

static int file_cow_clone(struct uk_vma *vma, struct uk_alloc *a,
			  struct uk_vma **out)
{
	struct file_vma *fvma = (struct file_vma *)vma;
	struct file_vma *copy;

	if (!fvma->source && (!fvma->shared || !fvma->identity))
		return -ENOTSUP;
	copy = uk_malloc(a, sizeof(*copy));
	if (!copy)
		return -ENOMEM;
	memcpy_isr(copy, fvma, sizeof(*copy));
	uk_ofile_acquire(copy->of);
	uk_vma_cow_source_get(copy->source);
	if (copy->shared)
		uk_refcount_acquire(&copy->shared->refs);
	file_identity_acquire(copy->identity);
	*out = &copy->base;
	return 0;
}

static int file_cow_unmap(struct uk_vma *vma, __vaddr_t addr, __sz len)
{
	return uk_vma_cow_anon_ops.unmap(vma, addr, len);
}

static int file_cow_set_attr(struct uk_vma *vma, unsigned long attr)
{
	struct file_vma *fvma = (struct file_vma *)vma;

	if ((attr & UK_PAGING_PAGE_ATTR_PROT_READ) &&
	    !_CAN_READ(fvma->of->mode))
		return -EACCES;
	return uk_vma_cow_anon_ops.set_attr(vma, attr);
}

static int file_cow_advise(struct uk_vma *vma, __vaddr_t addr, __sz len,
			   unsigned long advice)
{
	return uk_vma_cow_anon_ops.advise(vma, addr, len, advice);
}

const struct uk_vma_ops uk_fdio_cow_vma_ops = {
	.get_base = file_vma_op_get_base,
	.new = file_cow_new,
	.destroy = file_vma_op_destroy,
	.clone_cow = file_cow_clone,
	.fault = file_cow_fault,
	.unmap = file_cow_unmap,
	.sync = file_vma_op_sync,
	.split = file_vma_op_split,
	.merge = file_vma_op_merge,
	.set_attr = file_cow_set_attr,
	.advise = file_cow_advise
};

static int file_shared_new(struct uk_vas *vas, __vaddr_t addr, __sz len,
			   void *data, unsigned long attr,
			   unsigned long *flags, struct uk_vma **out)
{
	struct file_vma *fvma;
	int rc;

	if (!(*flags & UK_VMA_FILE_SHARED))
		return -EINVAL;
	rc = file_vma_op_new(vas, addr, len, data, attr, flags, out);
	if (rc)
		return rc;
	fvma = (struct file_vma *)*out;
	fvma->identity = file_identity_for(fvma->of->file);
	if (!fvma->identity) {
		/* Generic VMA initialization has not run on this error path. */
		fvma->base.vas = vas;
		file_vma_op_destroy(&fvma->base);
		uk_free(vas->a, fvma);
		*out = NULL;
		return -ENOMEM;
	}
	return 0;
}

static void file_shared_ref(struct uk_vma *vma, __vaddr_t addr,
			    struct uk_vma_shared_ref *out)
{
	struct file_vma *fvma = (struct file_vma *)vma;

	UK_ASSERT(fvma->shared && fvma->identity);
	out->object = fvma->identity;
	out->offset = (__sz)fvma->offset + addr - vma->start;
	out->get = file_identity_acquire;
	out->put = file_identity_release;
	file_identity_acquire(out->object);
}

const struct uk_vma_ops uk_fdio_shared_vma_ops = {
	.get_base = file_vma_op_get_base,
	.new = file_shared_new,
	.destroy = file_vma_op_destroy,
	.clone_cow = file_cow_clone,
	.shared_ref = file_shared_ref,
	.fault = file_vma_op_fault,
	.unmap = file_vma_op_unmap,
	.sync = file_vma_op_sync,
	.split = file_vma_op_split,
	.merge = file_vma_op_merge,
	.set_attr = file_vma_op_set_attr,
	.advise = file_vma_op_advise
};
#endif

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
