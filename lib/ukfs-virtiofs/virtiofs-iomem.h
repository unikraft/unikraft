/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Virtiofs I/O memory */

#ifndef __UK_VIRTIOFS_IOMEM_H__
#define __UK_VIRTIOFS_IOMEM_H__

#if !CONFIG_LIBUKFS_VIRTIOFS_IOMEM
#error "Do not include this file directly; enable the feature via Kconfig"
#endif

#include <uk/pod.h>
#include <uk/pod/anon.h>
#include <uk/sparsebuf.h>
#include <uk/sparsebuf/util.h>

#include "virtiofs-base.h"

#define VIRTIOFS_EMBED_BLKS 1

UK_SPARSEBUF_EMBED_HEADBLK(virtiofs_sb_head, VIRTIOFS_EMBED_BLKS);

struct virtiofs_iomem {
	/* Local iomem sparsebuf */
	struct virtiofs_sb_head io;
};

static inline
void virtiofs_iomem_init(struct virtiofs_iomem *iom)
{
	/* Local iomem */
	iom->io = UK_SPARSEBUF_EMBED_HEADBLK_INIT_VALUE(virtiofs_sb_head,
							VIRTIOFS_EMBED_BLKS);
}

static
ssize_t virtiofs_pod_pgin(void *addr, size_t npages, __paddr_t pa __unused,
			  void *arg, size_t pgoff)
{
	struct viofs_node *n = arg;
	struct iovec iov = {
		.iov_base = addr,
		.iov_len = npages * PAGE_SIZE
	};
	ssize_t r;

	if (!npages)
		return 0;
	r = virtiofs_file_read(n, pgoff * PAGE_SIZE, npages * PAGE_SIZE,
			       &iov, 1, 0);
	if (unlikely(r < 0))
		return r;
	if (!r)
		/* Offset is past EOF; page in anon mem */
		return uk_pod_anon_pagein(addr, npages, pa, arg, pgoff);

	while (!IS_ALIGNED((size_t)r, PAGE_SIZE)) {
		/* We've likely hit EOF; check & zero out rest */
		const size_t end = pgoff * PAGE_SIZE + r;
		const size_t rem = npages * PAGE_SIZE - r;
		void *const base = (char *)addr + end;

		if (end != n->attr.size) {
			/* Size may be out of date; do another read */
			ssize_t err;

			iov.iov_base = base;
			iov.iov_len = rem;
			err = virtiofs_file_read(n, end, rem, &iov, 1, 0);
			if (unlikely(err < 0)) {
				/* Return partial success if read min 1 page */
				if ((size_t)r > PAGE_SIZE)
					break;
				return err;
			}
			if (err > 0) {
				r += err;
				continue;
			}
		}
		/* Confirmed at EOF, clear remaining bytes in page */
		memset(base, 0, rem);
		r += rem;
	}
	return r / PAGE_SIZE;
}

static
ssize_t virtiofs_pod_pgwb(const void *addr, size_t npages,
			  void *arg, size_t pgoff)
{
	struct viofs_node *n = arg;
	size_t off = pgoff * PAGE_SIZE;
	size_t rem = npages * PAGE_SIZE;
	size_t wbytes = 0;
	size_t wpages;
	ssize_t r;

	if (!npages)
		return 0;
	do {
		const struct iovec iov = {
			.iov_base = (char *)addr + off,
			.iov_len = rem
		};

		r = virtiofs_file_write(n, off, rem, &iov, 1, 0);
		if (unlikely(!r))
			r = -EIO; /* Writes should always progress or error */
		if (unlikely(r < 0))
			break;

		wbytes += r;
		off += r;
		rem -= r;
	} while (rem && !IS_ALIGNED(wbytes, PAGE_SIZE));

	/* If we managed to writeback at least 1 page, report success */
	wpages = wbytes / PAGE_SIZE;
	if (wpages)
		return wpages;
	/* We should only get here on error */
	UK_ASSERT(r < 0);
	return r;
}

static const struct uk_pod_pgio virtiofs_pod_ops = {
	.pagein = virtiofs_pod_pgin,
	.writeback = virtiofs_pod_pgwb,
	.pageout = uk_pod_anon_pageout
};

static
void *virtiofs_sb_alloc_new(__u64 pgoff, __u32 npages, void *arg)
{
	return uk_pod_default_alloc(npages, 0, &virtiofs_pod_ops, arg, pgoff);
}

static
void virtiofs_sb_alloc_free(void *addr, __u64 pgoff, __u32 npages, void *arg)
{
	int r __maybe_unused;

	r = uk_pod_default_free(addr, npages, &virtiofs_pod_ops, arg, pgoff);
	UK_ASSERT(!r);
}

static
void virtiofs_sb_alloc_drop(void *addr, __u64 pgoff, __u32 npages, void *arg)
{
	int r __maybe_unused;

	r = uk_pod_default_drop(addr, npages, &virtiofs_pod_ops, arg, pgoff);
	UK_ASSERT(!r);
}

static const struct uk_sparsebuf_alloc_funcs virtiofs_sbfuncs = {
	.alloc_new = virtiofs_sb_alloc_new,
	.alloc_free = virtiofs_sb_alloc_free,
	.alloc_drop = virtiofs_sb_alloc_drop
};

static inline
void virtiofs_iomem_cleanup(struct virtiofs_iomem *iom, struct viofs_node *arg)
{
	const struct uk_sparsebuf_ctx ctx = {
		.funcs = &virtiofs_sbfuncs,
		.alloc = virtiofs_alloc,
		.arg = arg
	};

	/* Cleanup iomem buffers */
	UK_SPARSEBUF_EMBED_HEAD(iohead, &iom->io);
	uk_sparsebuf_clear(&ctx, UK_SPARSEBUF_EMBED_HEADP(iohead, &iom->io));
}

static inline
int virtiofs_iomem_sync(struct virtiofs_iomem *iom, struct viofs_node *arg)
{
	UK_SPARSEBUF_EMBED_HEAD(iohead, &iom->io);
	struct uk_sparsebuf_cur cur;
	struct uk_sparsebuf_slice *sl;
	int r;

	UK_SPARSEBUF_FOREACH(UK_SPARSEBUF_EMBED_HEADP(iohead, &iom->io), &cur) {
		sl = uk_sparsebuf_slice_at(&cur);
		r = uk_pod_default_writeback(sl->buf, sl->npages,
					     &virtiofs_pod_ops, arg, sl->pgoff);
		if (unlikely(r))
			return r;
	}
	return 0;
}

static inline
int virtiofs_iomem_scoop(struct virtiofs_iomem *iom, size_t off, size_t len,
			 struct viofs_node *arg)
{
	const struct uk_sparsebuf_ctx ctx = {
		.funcs = &virtiofs_sbfuncs,
		.alloc = virtiofs_alloc,
		.arg = arg
	};
	UK_SPARSEBUF_EMBED_HEAD(iohead, &iom->io);

	return uk_sparsebuf_punch_hole(&ctx,
		UK_SPARSEBUF_EMBED_HEADP(iohead, &iom->io), off, len);
}

static inline
int virtiofs_iomem_trunc(struct virtiofs_iomem *iom, size_t newsz,
			 struct viofs_node *arg)
{
	const struct uk_sparsebuf_ctx ctx = {
		.funcs = &virtiofs_sbfuncs,
		.alloc = virtiofs_alloc,
		.arg = arg
	};
	UK_SPARSEBUF_EMBED_HEAD(iohead, &iom->io);

	return uk_sparsebuf_truncate(&ctx,
				     UK_SPARSEBUF_EMBED_HEADP(iohead, &iom->io),
				     newsz);
}

#define VIRTIOFS_IODEST_IOMEM  1

struct virtiofs_iodest_ctx {
	struct uk_sparsebuf_cur iocur;
	int flags;
};

static
void virtiofs_iodest_prep(struct virtiofs_iomem *iom, size_t off,
			  struct virtiofs_iodest_ctx *c)
{
	const size_t pgoff = off / PAGE_SIZE;
	int flags = 0;
	UK_SPARSEBUF_EMBED_HEAD(iohead, &iom->io);

	if (uk_sparsebuf_lookup(UK_SPARSEBUF_EMBED_HEADP(iohead, &iom->io),
				pgoff, &c->iocur))
		flags |= VIRTIOFS_IODEST_IOMEM;
	c->flags = flags;
}

static
size_t virtiofs_iodest(struct virtiofs_iodest_ctx *c, size_t off, char **bufp)
{
	if (c->flags & VIRTIOFS_IODEST_IOMEM)
		return uk_sparsebuf_memat(off, &c->iocur, bufp);
	return 0;
}

static
int virtiofs_iomem_after(struct virtiofs_iomem *iom, size_t off)
{
	const size_t pgoff = off / PAGE_SIZE;
	struct uk_sparsebuf_cur cur;
	char *buf __maybe_unused;
	UK_SPARSEBUF_EMBED_HEAD(iohead, &iom->io);

	if (uk_sparsebuf_lookup(UK_SPARSEBUF_EMBED_HEADP(iohead, &iom->io),
				pgoff, &cur) &&
	    uk_sparsebuf_memat(off, &cur, &buf))
		return 1;
	return 0;
}

#endif /* __UK_VIRTIOFS_IOMEM_H__ */
