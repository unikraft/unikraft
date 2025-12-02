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

#include "fdio-impl.h"

struct file_vma {
	struct uk_vma base;

	/** Open file description mapped by this VMA */
	struct uk_ofile *of;
	/** Start offset describing what position in the file is mapped */
	__off offset;
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
int file_vma_op_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
		    __sz len __unused, void *data, unsigned long attr,
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

	/* Since we cannot do ISR-safe file accesses in the fault handler,
	 * we enforce full load at mapping time for now.
	 *
	 * TODO: Remove this restriction if possible.
	 */
	*flags |= UK_VMA_MAP_POPULATE;

	mode = args->of->mode;
	/* We only check for read permission, as changes are not written back */
	if ((attr & UK_PAGING_PAGE_ATTR_PROT_READ) && !_CAN_READ(mode))
		return -EACCES;

	fvma = uk_malloc(vas->a, sizeof(*fvma));
	if (unlikely(!fvma))
		return -ENOMEM;
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
	if (fvma->of != nvma->of)
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

	/* Writable shared mappings are not supported. */
	if ((vma->flags & UK_VMA_FILE_SHARED) &&
	    (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE))
		return -EACCES;
	if ((attr & UK_PAGING_PAGE_ATTR_PROT_READ) &&
	    !_CAN_READ(fvma->of->mode))
		return -EACCES;
	/* Default handler */
	return uk_vma_op_set_attr(vma, attr);
}

/* We only support private mappings. Changes are not carried through to the
 * underlying file. So we can just use the default unmap handler that unmaps
 * the memory and forgets about it.
 */
const struct uk_vma_ops uk_fdio_vma_ops = {
	.get_base	= file_vma_op_get_base,
	.new		= file_vma_op_new,
	.destroy	= file_vma_op_destroy,
	.fault		= file_vma_op_fault,
	.unmap		= uk_vma_op_unmap,	/* default */
	.split		= file_vma_op_split,
	.merge		= file_vma_op_merge,
	.set_attr	= file_vma_op_set_attr,
	.advise		= uk_vma_op_advise,	/* default */
};
