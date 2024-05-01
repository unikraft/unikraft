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
#include <uk/vma_ops.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#endif /* CONFIG_HAVE_PAGING */
#include <vfscore/file.h>
#include <vfscore/vnode.h>
#include <vfscore/uio.h>
#include <uk/isr/string.h>

struct uk_vma_file {
	struct uk_vma base;

	/** File mapped in this VMA */
	struct vfscore_file *f;

	/** Start offset describing what position in the file is mapped */
	__off offset;
};

#ifdef CONFIG_LIBUKVMEM_FILE_BASE
static __vaddr_t vma_op_file_get_base(struct uk_vas *vas __unused,
				      void *data __unused,
				      unsigned long flags __unused)
{
	return CONFIG_LIBUKVMEM_FILE_BASE;
}
#endif /* CONFIG_LIBUKVMEM_FILE_BASE */

int vma_op_file_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		    __sz len __unused, void *data, unsigned long attr,
		    unsigned long *flags, struct uk_vma **vma)
{
	struct uk_vma_file_args *args = (struct uk_vma_file_args *)data;
	struct uk_vma_file *vma_file;

	UK_ASSERT(data);
	UK_ASSERT(args->fd >= 0);
	UK_ASSERT(args->offset >= 0);
	UK_ASSERT(PAGE_ALIGNED(args->offset));

	/* Writable shared mappings are not supported.
	 * Read-only shared mappings are partially supported.
	 *
	 * We treat read-only shared mappings as private. Note that any writes
	 * to the underlying file while the mapping is established will not be
	 * reflected in memory.
	 */
	if ((*flags & UK_VMA_FILE_SHARED) && (attr & PAGE_ATTR_PROT_WRITE))
		return -ENOTSUP;

	/* Since we cannot do ISR-safe file accesses in the fault handler,
	 * we enforce full load at mapping time for now.
	 *
	 * TODO: Remove this restriction if possible.
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

	UK_ASSERT(vma_file->f);
	fdrop(vma_file->f);
}

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

static int vma_op_file_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct uk_vma_file *vma_file = (struct uk_vma_file *)vma;
	struct uk_pagetable * const pt = vma->vas->pt;
	unsigned long pages = fault->len / PAGE_SIZE;
	__paddr_t paddr = __PADDR_ANY;
	__vaddr_t vaddr;
	__sz bytes;
	__off off;
	int rc;

	UK_ASSERT(PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->len == PAGE_Lx_SIZE(fault->level));
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);

	rc = pt->fa->falloc(pt->fa, &paddr, pages, FALLOC_FLAG_ALIGNED);
	if (unlikely(rc))
		return rc;

	if (!(vma->flags & UK_VMA_FLAG_UNINITIALIZED)) {
		vaddr = ukplat_page_kmap(pt, paddr, pages, 0);
		if (unlikely(vaddr == __VADDR_INV)) {
			pt->fa->ffree(pt->fa, paddr, pages);
			return -ENOMEM;
		}

		off = (fault->vbase - vma->start) + vma_file->offset;

		rc = vma_file_read(vma_file->f, vaddr, fault->len, off, &bytes);
		if (unlikely(rc)) {
			ukplat_page_kunmap(pt, vaddr, pages, 0);
			pt->fa->ffree(pt->fa, paddr, pages);

			return rc;
		}

		/* Fill the remaining space with zeros */
		UK_ASSERT(fault->len >= bytes);

		memset_isr((void *)(vaddr + bytes), 0, fault->len - bytes);
		ukplat_page_kunmap(pt, vaddr, pages, 0);
	}

	fault->paddr = paddr;
	return 0;
}

static int vma_op_file_split(struct uk_vma *vma, __vaddr_t vaddr,
			     struct uk_vma **new_vma)
{
	struct uk_vma_file *vma_file = (struct uk_vma_file *)vma;
	struct uk_vma_file *v;
	__off off;

	v = uk_malloc(vma->vas->a, sizeof(struct uk_vma_file));
	if (unlikely(!v))
		return -ENOMEM;

	UK_ASSERT(vaddr >= vma->start);
	off = vaddr - vma->start;

	UK_ASSERT(vma_file->offset <= __OFF_MAX - off);
	v->offset = vma_file->offset + off;

	fhold(vma_file->f);
	v->f = vma_file->f;

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
	if ((vma->flags & UK_VMA_FILE_SHARED) && (attr & PAGE_ATTR_PROT_WRITE))
		return -EPERM;

	/* Default handler */
	return uk_vma_op_set_attr(vma, attr);
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
	.advise		= uk_vma_op_advise,	/* default */
};
