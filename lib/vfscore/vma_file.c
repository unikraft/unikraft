/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <errno.h>

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/falloc.h>
#include <uk/vmem/vma_ops.h>
#include <uk/arch/limits.h>
#ifdef CONFIG_LIBUKPAGING
#include <uk/paging.h>
#endif /* CONFIG_LIBUKPAGING */
#include <vfscore/file.h>
#include <vfscore/vma.h>
#include <vfscore/vnode.h>
#include <vfscore/uio.h>
#include <uk/isr/string.h>

struct uk_vma_file {
	struct uk_vma base;

	/** File mapped in this VMA */
	struct vfscore_file *f;

	/** Start offset describing what position in the file is mapped */
	__off offset;
	__paddr_t *priv_pages;
	__sz priv_npages;
	__sz priv_taken;
};

#ifdef CONFIG_LIBUKVMEM_FILE_BASE
static __vaddr_t vma_op_file_get_base(struct uk_vas *vas __unused,
				      void *data __unused,
				      unsigned long flags __unused)
{
	return CONFIG_LIBUKVMEM_FILE_BASE;
}
#endif /* CONFIG_LIBUKVMEM_FILE_BASE */

static int vma_file_read(struct vfscore_file *fp, __vaddr_t buf, __sz len,
			 __off offset, __sz *bytes)
{
	struct vnode *vp = fp->f_dentry->d_vnode;
	struct iovec iovec = {
		.iov_base = (void *)buf,
		.iov_len = len,
	};
	struct uio uio = {
		.uio_iov = &iovec,
		.uio_iovcnt = 1,
		.uio_offset = offset,
		.uio_resid = len,
		.uio_rw = UIO_READ,
	};
	int rc;

	vn_lock(vp);
	rc = VOP_READ(vp, fp, &uio, 0);
	vn_unlock(vp);

	if (unlikely(rc))
		return -rc;

	UK_ASSERT(bytes);
	*bytes = len - uio.uio_resid;

	return 0;
}

static void vma_file_priv_pages_free(struct uk_vas *vas, __paddr_t *pages,
				     __sz npages)
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

static int vma_file_priv_pages_new(struct uk_vas *vas, struct vfscore_file *fp,
				   __off offset, __sz len, unsigned long flags,
				   __paddr_t **out_pages, __sz *out_npages)
{
	const __sz count = len / UK_PAGING_PAGE_SIZE;
	struct uk_pagetable *pt = vas->pt;
	__paddr_t *pages;
	__sz i;
	int rc;

	if (!count || count > __SZ_MAX / sizeof(*pages))
		return -ENOMEM;
	pages = uk_malloc(vas->a, count * sizeof(*pages));
	if (!pages)
		return -ENOMEM;
	for (i = 0; i < count; i++)
		pages[i] = UK_PAGING_PADDR_INV;

	for (i = 0; i < count; i++) {
		__paddr_t paddr = UK_PAGING_PADDR_ANY;

		rc = pt->fa->falloc(pt->fa, &paddr, 1, FALLOC_FLAG_ALIGNED);
		if (unlikely(rc))
			goto fail;

		if (!(flags & UK_VMA_MAP_UNINITIALIZED)) {
			__vaddr_t kvaddr;
			__sz bytes = 0;
			__off off = offset + (__off)(i * UK_PAGING_PAGE_SIZE);

			kvaddr = uk_paging_page_kmap(pt, paddr, 1, 0);
			if (unlikely(kvaddr == UK_PAGING_VADDR_INV)) {
				pt->fa->ffree(pt->fa, paddr, 1);
				rc = -ENOMEM;
				goto fail;
			}
			rc = vma_file_read(fp, kvaddr, UK_PAGING_PAGE_SIZE,
					   off, &bytes);
			if (unlikely(rc)) {
				uk_paging_page_kunmap(pt, kvaddr, 1, 0);
				pt->fa->ffree(pt->fa, paddr, 1);
				goto fail;
			}
			UK_ASSERT(UK_PAGING_PAGE_SIZE >= bytes);
			memset_isr((void *)(kvaddr + bytes), 0,
				   UK_PAGING_PAGE_SIZE - bytes);
			uk_paging_page_kunmap(pt, kvaddr, 1, 0);
		}
		pages[i] = paddr;
	}

	*out_pages = pages;
	*out_npages = count;
	return 0;

fail:
	vma_file_priv_pages_free(vas, pages, count);
	return rc;
}

int vma_op_file_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		    __sz len, void *data, unsigned long attr,
		    unsigned long *flags, struct uk_vma **vma)
{
	struct uk_vma_file_args *args = (struct uk_vma_file_args *)data;
	struct uk_vma_file *vma_file;
	int rc;

	UK_ASSERT(data);
	UK_ASSERT(args->fd >= 0);
	UK_ASSERT(args->offset >= 0);
	UK_ASSERT(UK_PAGING_PAGE_ALIGNED(args->offset));

	/* Writable shared mappings are not supported.
	 * Read-only shared mappings are partially supported.
	 *
	 * We treat read-only shared mappings as private. Note that any writes
	 * to the underlying file while the mapping is established will not be
	 * reflected in memory.
	 */
	if ((*flags & UK_VMA_FILE_SHARED) &&
	    (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE))
		return -ENOTSUP;

	/* Pre-read file contents in sleepable thread context so the fault
	 * callback remains strictly non-blocking under vas->lock.
	 */
	*flags |= UK_VMA_MAP_POPULATE;

	vma_file = uk_malloc(vas->a, sizeof(struct uk_vma_file));
	if (unlikely(!vma_file))
		return -ENOMEM;

	vma_file->f = vfscore_get_file(args->fd);
	if (unlikely(!vma_file->f)) {
		uk_free(vas->a, vma_file);
		return -EBADF;
	}
	vma_file->offset = args->offset;
	vma_file->priv_pages = __NULL;
	vma_file->priv_npages = 0;
	vma_file->priv_taken = 0;

	rc = vma_file_priv_pages_new(vas, vma_file->f, args->offset, len,
				     *flags, &vma_file->priv_pages,
				     &vma_file->priv_npages);
	if (unlikely(rc)) {
		fdrop(vma_file->f);
		uk_free(vas->a, vma_file);
		return rc;
	}

	/* Use the file name as VMA name. Since the memory management of the
	 * string is tied to the file object, we do not need to care about
	 * freeing it. So it is ok, if the caller should override the name.
	 */
	vma_file->base.name = vma_file->f->f_dentry->d_path;

	UK_ASSERT(vma);
	*vma = &vma_file->base;

	return 0;
}

static void vma_op_file_destroy(struct uk_vma *vma)
{
	struct uk_vma_file *vma_file = (struct uk_vma_file *)vma;

	if (vma_file->priv_pages) {
		vma_file_priv_pages_free(vma->vas, vma_file->priv_pages,
					 vma_file->priv_npages);
		vma_file->priv_pages = __NULL;
		vma_file->priv_npages = 0;
		vma_file->priv_taken = 0;
	}
	UK_ASSERT(vma_file->f);
	fdrop(vma_file->f);
}

static int vma_op_file_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct uk_vma_file *vma_file = (struct uk_vma_file *)vma;
	__sz idx;

	UK_ASSERT(UK_PAGING_PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->len == UK_PAGING_PAGE_Lx_SIZE(fault->level));
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);

	if (fault->level != UK_PAGING_PAGE_LEVEL)
		return -ENOMEM;

	idx = (fault->vbase - vma->start) / UK_PAGING_PAGE_SIZE;
	if (unlikely(!vma_file->priv_pages || idx >= vma_file->priv_npages ||
		     vma_file->priv_pages[idx] == UK_PAGING_PADDR_INV))
		return -EIO;

	fault->paddr = vma_file->priv_pages[idx];
	vma_file->priv_pages[idx] = UK_PAGING_PADDR_INV;
	vma_file->priv_taken++;
	if (vma_file->priv_taken == vma_file->priv_npages) {
		uk_free(vma->vas->a, vma_file->priv_pages);
		vma_file->priv_pages = __NULL;
		vma_file->priv_npages = 0;
		vma_file->priv_taken = 0;
	}
	return 0;
}

static int vma_op_file_split(struct uk_vma *vma, __vaddr_t vaddr,
			     struct uk_vma **new_vma)
{
	struct uk_vma_file *vma_file = (struct uk_vma_file *)vma;
	struct uk_vma_file *v;
	__off off;

	UK_ASSERT(!vma_file->priv_pages);

	v = uk_malloc(vma->vas->a, sizeof(struct uk_vma_file));
	if (unlikely(!v))
		return -ENOMEM;

	UK_ASSERT(vaddr >= vma->start);
	off = vaddr - vma->start;

	UK_ASSERT(vma_file->offset <= __OFF_MAX - off);
	v->offset = vma_file->offset + off;

	fhold(vma_file->f);
	v->f = vma_file->f;
	v->priv_pages = __NULL;
	v->priv_npages = 0;
	v->priv_taken = 0;

	UK_ASSERT(new_vma);
	*new_vma = &v->base;

	return 0;
}

static int vma_op_file_merge(struct uk_vma *vma, struct uk_vma *next)
{
	struct uk_vma_file *vma_file = (struct uk_vma_file *)vma;
	struct uk_vma_file *next_file = (struct uk_vma_file *)next;
	__off off;

	UK_ASSERT(next->start == vma->end);
	UK_ASSERT(next->start > vma->start);

	if (vma_file->priv_pages || next_file->priv_pages)
		return -EPERM;

	/* Only merge if this is the same file... */
	if (vma_file->f->f_dentry != next_file->f->f_dentry)
		return -EPERM;

	/* ...and the VMAs map contiguous file ranges */
	off = next->start - vma->start;
	if (next_file->offset != vma_file->offset + off)
		return -EPERM;

	/* We call fdrop() in the destructor */

	return 0;
}

static int vma_op_file_set_attr(struct uk_vma *vma, unsigned long attr)
{
	/* Writable shared mappings are not supported. */
	if ((vma->flags & UK_VMA_FILE_SHARED) &&
	    (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE))
		return -EPERM;

	/* Default handler */
	return uk_vma_op_set_attr(vma, attr);
}

static int vma_op_file_advise(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
			      unsigned long advice)
{
	if (advice & UK_VMA_ADV_DONTNEED)
		return 0;
	return uk_vma_op_advise(vma, vaddr, len, advice);
}

/* We only support private mappings. Changes are not carried through to the
 * underlying file. So we can just use the default unmap handler that unmaps
 * the memory and forgets about it. Private file mappings can also change their
 * protections without checking for the permissions on the underlying file. We
 * can thus also use the default attribute setter.
 */
const struct uk_vma_ops uk_vma_file_ops = {
#ifdef CONFIG_LIBUKVMEM_FILE_BASE
	.get_base	= vma_op_file_get_base,
#else /* CONFIG_LIBUKVMEM_FILE_BASE */
	.get_base	= __NULL,
#endif /* !CONFIG_LIBUKVMEM_FILE_BASE */
	.new		= vma_op_file_new,
	.destroy	= vma_op_file_destroy,
	.fault		= vma_op_file_fault,
	.unmap		= uk_vma_op_unmap,	/* default */
	.split		= vma_op_file_split,
	.merge		= vma_op_file_merge,
	.set_attr	= vma_op_file_set_attr,
	.advise		= vma_op_file_advise,
};
