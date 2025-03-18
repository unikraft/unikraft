/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* RAMfs file implementation; internal use only in ramfs.c */

#ifndef __UK_RAMFS_FILE_H__
#define __UK_RAMFS_FILE_H__

#include <uk/file/iovutil.h>
#include <uk/fs/prio.h>
#include <uk/pod.h>
#include <uk/pod/anon.h>
#include <uk/sparsebuf.h>
#include <uk/sparsebuf/util.h>

/* Sparse buffer callbacks & context */

/* Chosen arbitrarily; TODO: tune / make configurable */
#define RAMFS_FILE_EMBED_BLKS 1

UK_SPARSEBUF_EMBED_HEADBLK(ramfs_file_data, RAMFS_FILE_EMBED_BLKS);

static inline
void ramfs_init_file_data(struct ramfs_file_data *file_data)
{
	*file_data = UK_SPARSEBUF_EMBED_HEADBLK_INIT_VALUE(
		ramfs_file_data, RAMFS_FILE_EMBED_BLKS
	);
}

static
void *ramfs_file_alloc_new(__u64 pgoff, __u32 npages, void *arg __unused)
{
	return uk_pod_default_alloc(npages, 1, &uk_pod_anon_ops, NULL, pgoff);
}

static
void ramfs_file_alloc_free(void *addr, __u64 pgoff, __u32 npages,
			   void *arg __unused)
{
	int r __maybe_unused;

	r = uk_pod_default_free(addr, npages, &uk_pod_anon_ops, NULL, pgoff);
	UK_ASSERT(!r);
}

static
void ramfs_file_alloc_drop(void *addr, __u64 pgoff, __u32 npages,
			   void *arg __unused)
{
	int r __maybe_unused;

	r = uk_pod_default_drop(addr, npages, &uk_pod_anon_ops, NULL, pgoff);
	UK_ASSERT(!r);
}

static const struct uk_sparsebuf_alloc_funcs ramfs_file_sbfuncs = {
	.alloc_new = ramfs_file_alloc_new,
	.alloc_free = ramfs_file_alloc_free,
	.alloc_drop = ramfs_file_alloc_drop
};

static struct uk_sparsebuf_ctx ramfs_sb_ctx = {
	.funcs = &ramfs_file_sbfuncs,
	.arg = NULL
};

/* TODO: Make configurable */
/* TODO: Make compile-time when default alloc is constexpr */
static
int init_ramfs_sbctx(struct uk_init_ctx *ictx __unused)
{
	ramfs_sb_ctx.alloc = uk_alloc_get_default();
	return 0;
}

uk_rootfs_initcall_prio(init_ramfs_sbctx, 0x0, UK_FS_PRIO_PRIMARY);

static inline struct uk_alloc *ramfs_alloc(void)
{
	return ramfs_sb_ctx.alloc;
}

/* RAMfs file data */

static
void ramfs_file_data_free(struct ramfs_file_data *file_data)
{
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	uk_sparsebuf_clear(&ramfs_sb_ctx,
			   UK_SPARSEBUF_EMBED_HEADP(fhead, file_data));
}

static
size_t ramfs_file_range_bufcount(struct ramfs_file_data *file_data,
				 size_t off, size_t len)
{
	int r;
	struct uk_sparsebuf_cur cur;
	size_t ret = 0;
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	UK_ASSERT(len);

	r = uk_sparsebuf_lookup(UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				off / PAGE_SIZE, &cur);
	if (!r)
		/* File empty, one sparse area */
		return 1;
	while (len) {
		char *buf __unused;
		const size_t blen = uk_sparsebuf_memat(off, &cur, &buf);

		if (!blen)
			break;
		ret++;
		off += blen;
		if (blen >= len)
			break;
		len -= blen;
	}
	return ret;
}

static
size_t ramfs_file_range_get(struct ramfs_file_data *file_data,
			    size_t off, size_t len,
			    struct iovec *iov, size_t iovcnt)
{
	int r;
	struct uk_sparsebuf_cur cur;
	size_t itop = 0;
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	UK_ASSERT(iovcnt);
	UK_ASSERT(len);

	r = uk_sparsebuf_lookup(UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				off / PAGE_SIZE, &cur);
	if (!r) {
		/* Return all sparse */
		iov[0].iov_base = NULL;
		iov[0].iov_len = len;
		return 1;
	}
	while (len && itop < iovcnt) {
		char *buf;
		const size_t blen = uk_sparsebuf_memat(off, &cur, &buf);
		const size_t wlen = MIN(blen, len);

		if (unlikely(!blen))
			break;
		iov[itop].iov_base = buf;
		iov[itop].iov_len = wlen;
		itop++;
		off += wlen;
		len -= wlen;
	}
	return itop;
}

static
ssize_t ramfs_file_range_acquire(struct ramfs_file_data *file_data,
				 size_t off, size_t len)
{
	const size_t pgoff = off / PAGE_SIZE;
	const size_t pgend = ALIGN_UP(off + len, PAGE_SIZE) / PAGE_SIZE;
	const size_t npages = pgend - pgoff;
	ssize_t r;
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	r = uk_sparsebuf_fill(&ramfs_sb_ctx,
			      UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
			      pgoff, npages, 1);
	if (unlikely(r < 0))
		return r;
	/* Return what we managed to acquire */
	return (pgoff + r) * PAGE_SIZE - off;
}

static
void ramfs_file_range_release(struct ramfs_file_data *file_data,
			      size_t off, size_t len)
{
	int r;
	const size_t pgoff = off / PAGE_SIZE;
	const size_t pgend = ALIGN_UP(off + len, PAGE_SIZE) / PAGE_SIZE;
	const size_t npages = pgend - pgoff;
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	r = uk_sparsebuf_ref_release(&ramfs_sb_ctx,
				     UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				     pgoff, npages, 0);
	if (unlikely(r))
		uk_pr_err("Cannot release file %p range 0x%zx + 0x%zx: %d\n",
			  file_data, off, len, r);
}

static
int ramfs_file_range_gift(struct ramfs_file_data *file_data,
			  unsigned int pgoff, struct iovec *iov, size_t iovcnt)
{
	int r = 0;
	int ret = 0;
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	for (size_t i = 0; i < iovcnt; i++) {
		const size_t npages = iov[i].iov_len / PAGE_SIZE;

		r = uk_sparsebuf_assign(
			&ramfs_sb_ctx,
			UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
			pgoff, npages, iov[i].iov_base
		);
		UK_ASSERT(r <= 0);
		if (unlikely(r))
			break;
		pgoff += npages;
		ret++;
	}
	return ret ? ret : r;
}

static
size_t ramfs_file_data_read(struct ramfs_file_data *file_data,
			    const struct iovec *iov, size_t iovcnt,
			    size_t off, size_t rem)
{
	struct uk_sparsebuf_cur cur;
	size_t i = 0;
	size_t boff = 0;
	size_t ret = 0;
	int r;
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	r = uk_sparsebuf_lookup(UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				off / PAGE_SIZE, &cur);
	if (!r)
		return uk_iov_zero(iov, iovcnt, rem, &i, &boff);

	while (rem && i < iovcnt) {
		char *buf;
		size_t iolen;
		size_t nbytes;
		const size_t blen = uk_sparsebuf_memat(off, &cur, &buf);

		if (unlikely(!blen)) {
			/* Exhausted allocs, fill rest w/ zeros */
			ret += uk_iov_zero(iov, iovcnt, rem, &i, &boff);
			break;
		}
		iolen = MIN(rem, blen);
		if (buf)
			nbytes = uk_iov_scatter(iov, iovcnt, buf, iolen,
						&i, &boff);
		else
			nbytes = uk_iov_zero(iov, iovcnt, iolen, &i, &boff);
		off += nbytes;
		rem -= nbytes;
		ret += nbytes;
	}
	return ret;
}

static
size_t ramfs_file_data_write(struct ramfs_file_data *file_data,
			     const struct iovec *iov, size_t iovcnt,
			     size_t off, size_t rem)
{
	const size_t pgoff = off / PAGE_SIZE;
	const size_t pgend = DIV_ROUND_UP(off + rem, PAGE_SIZE);
	const size_t npages = pgend - pgoff;
	struct uk_sparsebuf_cur cur;
	size_t i = 0;
	size_t boff = 0;
	size_t ret = 0;
	ssize_t nmapped;
	int r;
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	nmapped = uk_sparsebuf_fill(&ramfs_sb_ctx,
				    UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				    pgoff, npages, 0);
	if (unlikely(nmapped <= 0))
		return 0;
	r = uk_sparsebuf_lookup(UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				pgoff, &cur);
	if (unlikely(!r))
		UK_CRASH("Inconsistent file state\n");
	while (rem) {
		size_t nbytes;
		char *buf;
		const size_t blen = uk_sparsebuf_memat(off, &cur, &buf);

		if (!blen || !buf)
			break;
		nbytes = uk_iov_gather(buf, iov, iovcnt, blen, &i, &boff);
		off += nbytes;
		rem -= nbytes;
		ret += nbytes;
	}
	return ret;
}

static
int ramfs_file_data_trunc(struct ramfs_file_data *file_data, size_t newsz)
{
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	return uk_sparsebuf_truncate(&ramfs_sb_ctx,
				     UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				     newsz);
}

static
int ramfs_file_data_punch_hole(struct ramfs_file_data *file_data,
			       size_t off, size_t len)
{
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	return uk_sparsebuf_punch_hole(&ramfs_sb_ctx,
		UK_SPARSEBUF_EMBED_HEADP(fhead, file_data), off, len);
}

static
int ramfs_file_data_collapse(struct ramfs_file_data *file_data,
			     unsigned int pgoff, unsigned int npages)
{
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	return uk_sparsebuf_collapse(&ramfs_sb_ctx,
				     UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				     pgoff, npages);
}

static
int ramfs_file_data_insert_hole(struct ramfs_file_data *file_data,
				unsigned int pgoff, unsigned int npages)
{
	UK_SPARSEBUF_EMBED_HEAD(fhead, file_data);

	return uk_sparsebuf_insert(&ramfs_sb_ctx,
				   UK_SPARSEBUF_EMBED_HEADP(fhead, file_data),
				   pgoff, npages);
}

#endif /* __UK_RAMFS_FILE_H__ */
