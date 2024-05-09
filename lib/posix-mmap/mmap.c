/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include <uk/config.h>
#include <uk/syscall.h>
#include <uk/essentials.h>
#include <uk/arch/limits.h>
#include <uk/arch/lcpu.h>
#include <uk/vmem.h>

#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0x4000000
#endif /* !MAP_UNINITIALIZED */

#define VALID_PROT_MASK		(PROT_READ | PROT_WRITE | PROT_EXEC)

static inline unsigned long prot_to_attr(int prot)
{
	unsigned long attr = PAGE_ATTR_PROT_NONE;

	/* Note that specifying invalid values is not an error for mmap. You
	 * just don't get any meaningful permissions on the area.
	 */

	if (prot & PROT_READ)
		attr |= PAGE_ATTR_PROT_READ;
	if (prot & PROT_WRITE)
		attr |= PAGE_ATTR_PROT_WRITE;
	if (prot & PROT_EXEC)
		attr |= PAGE_ATTR_PROT_EXEC;

	return attr;
}

static int do_mmap(void **addr, size_t len, int prot, int flags, int fd,
		   off_t offset)
{
	struct uk_vas *vas = uk_vas_get_active();
	unsigned long vattr = prot_to_attr(prot);
	unsigned long vflags = 0;
	__vaddr_t vaddr;
#ifdef CONFIG_LIBVFSCORE
	struct uk_vma_file_args file_args;
#endif /* CONFIG_LIBVFSCORE */
	void *vargs;
	const struct uk_vma_ops *vops;
	unsigned int order, lvl = PAGE_LEVEL;
	int rc;

	if (unlikely(len == 0))
		return -EINVAL;

	/* len will overflow when aligning it to page size */
	if (unlikely(len > __SZ_MAX - PAGE_SIZE))
		return -ENOMEM;

	if (unlikely(offset < 0 || !PAGE_ALIGNED(offset)))
		return -EINVAL;

	if (unlikely((__sz)offset > __OFF_MAX - len))
		return -EOVERFLOW;

	if (*addr) {
		if (flags & MAP_FIXED)
			vflags |= UK_VMA_MAP_REPLACE;

		vaddr = (__vaddr_t)*addr;
	} else {
		/* Linux returns -EPERM if addr is NULL for a fixed mapping */
		if (unlikely(flags & MAP_FIXED))
			return -EPERM;

		if (unlikely(flags & MAP_FIXED_NOREPLACE))
			return -EPERM;

		vaddr = __VADDR_ANY;
	}

	if (flags & MAP_ANONYMOUS) {
		if ((flags & MAP_SHARED) ||
		    (flags & MAP_SHARED_VALIDATE) == MAP_SHARED_VALIDATE) {
			/* MAP_SHARED(_VALIDATE): Note, we ignore it for
			 * anonymous memory since we only have a single
			 * process. There is no one to share the mapping with.
			 * It is thus fine to create a private mapping.
			 */
		} else if (unlikely(!(flags & MAP_PRIVATE)))
			return -EINVAL;

		/* We do not support unrestricted VMAs */
		if (unlikely(flags & MAP_GROWSDOWN))
			return -ENOTSUP;

		if (flags & MAP_UNINITIALIZED)
			vflags |= UK_VMA_MAP_UNINITIALIZED;

		if (flags & MAP_HUGETLB) {
#ifdef PAGE_LARGE_SHIFT
			order = (flags >> MAP_HUGE_SHIFT) & MAP_HUGE_MASK;
			if (order == 0)
				order = PAGE_LARGE_SHIFT;

			lvl = PAGE_SHIFT_Lx(order);

			if (unlikely(!PAGE_Lx_HAS(lvl)))
				return -EINVAL;

			vflags |= UK_VMA_MAP_SIZE(order);
#else
			return -EINVAL;
#endif /* PAGE_LARGE_SHIFT */
		}

		vargs = NULL;
		vops  = &uk_vma_anon_ops;
	} else {
#ifdef CONFIG_LIBVFSCORE
		if ((flags & MAP_SHARED) ||
		    (flags & MAP_SHARED_VALIDATE) == MAP_SHARED_VALIDATE)
			vflags |= UK_VMA_FILE_SHARED;
		else if (unlikely(!(flags & MAP_PRIVATE)))
			return -EINVAL;

		if (unlikely(flags & MAP_GROWSDOWN))
			return -EINVAL;

		if (unlikely(flags & MAP_HUGETLB))
			return -EINVAL;

		/* Not supported yet. Linux returns EOPNOTSUPP here */
		if (unlikely(flags & MAP_SYNC))
			return -EOPNOTSUPP;

		if (unlikely(!PAGE_ALIGNED(offset)))
			return -EINVAL;

		if (unlikely(fd < 0))
			return -EBADF;

		/* Linux reports -ENODEV for stdout, stdin, stderr */
		if (unlikely(fd < 3))
			return -ENODEV;

		file_args.fd     = fd;
		file_args.offset = offset;

		vargs = &file_args;
		vops  = &uk_vma_file_ops;
#else
		(void)fd; /* silence warning */

		return -ENOTSUP;
#endif /* CONFIG_LIBVFSCORE */
	}

	/* Linux will always align len to the selected page size */
	len = PAGE_Lx_ALIGN_UP(len, lvl);

	if (flags & MAP_POPULATE)
		vflags |= UK_VMA_MAP_POPULATE;

	/* MAP_LOCKED: Ignored for now */
	/* MAP_NONBLOCKED: Ignored for now */
	/* MAP_NORESERVE : Ignored for now */

	do {
		rc = uk_vma_map(vas, &vaddr, len, vattr, vflags, NULL, vops,
				vargs);
		if (likely(rc == 0))
			break;

		if (vaddr == __VADDR_ANY)
			return rc;

		/* If addr was meant as a hint and we fail to map, we retry
		 * without specifying an address.
		 */
		vaddr = __VADDR_ANY;
	} while (1);

	*addr = (void *)vaddr;
	return rc;
}

UK_SYSCALL_DEFINE(void *, mmap, void *, addr, size_t, len, int, prot,
		  int, flags, int, fd, off_t, offset)
{
	int rc;

	rc = do_mmap(&addr, len, prot, flags, fd, offset);
	if (unlikely(rc)) {
		errno = -rc;
		return MAP_FAILED;
	}

	return addr;
}

UK_SYSCALL_R_DEFINE(int, munmap, void *, addr, size_t, len)
{
	struct uk_vas *vas = uk_vas_get_active();
	__vaddr_t vaddr = (__vaddr_t)addr;
	int rc;

	if (unlikely(len == 0))
		return -EINVAL;

	rc = uk_vma_unmap(vas, vaddr, PAGE_ALIGN_UP(len), 0);
	if (unlikely(rc)) {
		if (rc == -ENOENT)
			return 0;

		return rc;
	}

	return 0;
}

UK_SYSCALL_R_DEFINE(int, mprotect, void *, addr, size_t, len, int, prot)
{
	struct uk_vas *vas = uk_vas_get_active();
	unsigned long vattr = prot_to_attr(prot);
	__vaddr_t vaddr = (__vaddr_t)addr;
	int rc;

	/* mprotect does not tolerate invalid prot values */
	if (unlikely(prot & ~VALID_PROT_MASK))
		return -EINVAL;

	rc = uk_vma_set_attr(vas, vaddr, PAGE_ALIGN_UP(len), vattr,
			     UK_VMA_FLAG_STRICT_VMA_CHECK);
	if (unlikely(rc)) {
		if (rc == -ENOENT)
			return -ENOMEM;

		return rc;
	}

	return 0;
}

UK_SYSCALL_R_DEFINE(int, madvise, void *, addr, size_t, len, int, advice)
{
	struct uk_vas *vas = uk_vas_get_active();
	unsigned long vadvice = 0;
	__vaddr_t vaddr = (__vaddr_t)addr;
	int rc;

	switch (advice) {
	case MADV_DONTNEED:
		vadvice |= UK_VMA_ADV_DONTNEED;
		break;
	default:
		/* Just ignore unsupported advices for now. The call to
		 * uk_vma_advise() does not have an effect but will validate
		 * parameters nevertheless and return errors if needed.
		 */
		break;
	}

	rc = uk_vma_advise(vas, vaddr, PAGE_ALIGN_UP(len), vadvice,
			   UK_VMA_FLAG_STRICT_VMA_CHECK);
	if (unlikely(rc)) {
		if (rc == -ENOENT)
			return -ENOMEM;

		return rc;
	}

	return 0;
}

UK_SYSCALL_R_DEFINE(int, msync, void*, addr, size_t, length, int, flags)
{
	/* Since writing mmap memory back to file is not supported,
	 * this doesn't make sense.
	 */
	return 0;
}

UK_SYSCALL_R_DEFINE(int, mlock, const void*, addr, size_t, len)
{
	/* Since swap memory is not supported, this doesn't make sense */
	return 0;
}
