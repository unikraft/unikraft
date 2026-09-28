/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* /dev/maps: the active address space in the format of Linux's
 * /proc/self/maps.  glibc's pthread_getattr_np() reads that file to find
 * the main thread's stack (musl probes instead); runtimes such as .NET
 * fail to start without it.  Link /proc/self/maps to /dev/maps to use it.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <uk/alloc.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/vmem.h>
#include <uk/vmem/vma_types.h>
#include <vfscore/uio.h>
#include <devfs/device.h>

#define DEV_MAPS_NAME "maps"

/* The text is rendered twice: once to size it, then into a buffer of that
 * size.  `buf` is __NULL on the sizing pass.
 */
struct maps_text {
	char *buf;
	__sz cap;
	__sz len;
};

static void maps_line(struct maps_text *t, __vaddr_t start, __vaddr_t end,
		      unsigned long attr, const char *name)
{
	int n;

	n = snprintf(t->buf ? t->buf + t->len : __NULL,
		     t->buf ? t->cap - t->len : 0,
		     "%lx-%lx %c%c%cp 00000000 00:00 0%s%s\n",
		     (unsigned long)start, (unsigned long)end,
		     (attr & UK_PAGING_PAGE_ATTR_PROT_READ) ? 'r' : '-',
		     (attr & UK_PAGING_PAGE_ATTR_PROT_WRITE) ? 'w' : '-',
		     (attr & UK_PAGING_PAGE_ATTR_PROT_EXEC) ? 'x' : '-',
		     name ? " " : "", name ? name : "");
	if (unlikely(n < 0))
		return;
	/* A line that no longer fits (a VMA added since the sizing pass) is
	 * dropped whole rather than cut.
	 */
	if (t->buf && t->len + (__sz)n >= t->cap)
		return;
	t->len += (__sz)n;
}

static void maps_render(struct uk_vas *vas, struct maps_text *t)
{
	const struct uk_vma *vma;

	for (vma = uk_vma_first(vas); vma; vma = uk_vma_next(vma)) {
		__vaddr_t start = vma->start, end = vma->end;
		const char *name = vma->name;

		if (vma->ops == &uk_vma_stack_ops) {
			/* Guard pages as their own inaccessible lines, as
			 * Linux shows its stack guard gap: glibc sizes the
			 * stack from the line that holds it.
			 */
			if (UK_VMA_STACK_BOTTOM_GUARD_SIZE) {
				maps_line(t, start,
					  start + UK_VMA_STACK_BOTTOM_GUARD_SIZE,
					  0, __NULL);
				start += UK_VMA_STACK_BOTTOM_GUARD_SIZE;
			}
			end -= UK_VMA_STACK_TOP_GUARD_SIZE;
			if (!name)
				name = "[stack]";
		} else if (vma->ops == &uk_vma_rsvd_ops) {
			maps_line(t, start, end, 0, name);
			continue;
		}

		maps_line(t, start, end, vma->attr, name);

		if (vma->ops == &uk_vma_stack_ops &&
		    UK_VMA_STACK_TOP_GUARD_SIZE)
			maps_line(t, end, vma->end, 0, __NULL);
	}
}

/* Every read renders the whole map afresh and copies from its offset, which
 * is simple and fine for the tens of VMAs a program has; a map that changes
 * between two reads of one pass can shift lines, as it can on Linux.
 */
static int dev_maps_read(struct device *dev __unused, struct uio *uio,
			 int flags __unused)
{
	struct maps_text t = { 0 };
	struct uk_vas *vas;
	int rc;

	if (unlikely(uio->uio_offset < 0))
		return EINVAL;

	vas = uk_vas_get_active();
	if (unlikely(!vas))
		return ENXIO;

	maps_render(vas, &t);
	if (uio->uio_offset >= (off_t)t.len)
		return 0;

	t.cap = t.len + 1;
	t.len = 0;
	t.buf = uk_malloc(uk_alloc_get_default(), t.cap);
	if (unlikely(!t.buf))
		return ENOMEM;
	maps_render(vas, &t);

	rc = 0;
	if (uio->uio_offset < (off_t)t.len)
		rc = vfscore_uiomove(t.buf + uio->uio_offset,
				     t.len - (__sz)uio->uio_offset, uio);
	uk_free(uk_alloc_get_default(), t.buf);
	return rc;
}

static int dev_maps_write(struct device *dev __unused,
			  struct uio *uio __unused, int flags __unused)
{
	return EACCES;
}

static struct devops maps_devops = {
	.open = dev_noop_open,
	.close = dev_noop_close,
	.read = dev_maps_read,
	.write = dev_maps_write,
	.ioctl = dev_noop_ioctl,
};

static struct driver drv_maps = {
	.devops = &maps_devops,
	.devsz = 0,
	.name = DEV_MAPS_NAME
};

static int devfs_register(struct uk_init_ctx *ictx __unused)
{
	int rc;

	rc = device_create(&drv_maps, DEV_MAPS_NAME, D_CHR, __NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to devfs: %d\n",
			  DEV_MAPS_NAME, rc);
		return -rc;
	}

	return 0;
}

devfs_initcall(devfs_register);
