/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <linux/falloc.h>

#include <uk/alloc.h>
#include <uk/errptr.h>
#include <uk/file/iovutil.h>
#include <uk/fs.h>
#include <uk/fs/dirent.h>
#include <uk/fs/driver.h>
#include <uk/fs/pathutil.h>
#include <uk/fs/template/live.h>
#include <uk/init.h>
#include <uk/sched.h>

#include "virtiofs-base.h"

#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM

#include "virtiofs-iomem.h"

#else /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */

struct virtiofs_iomem {};
static inline void virtiofs_iomem_init(struct virtiofs_iomem *iom __unused) {}
static inline void virtiofs_iomem_cleanup(struct virtiofs_iomem *iom __unused,
					  void *arg __unused) {}
static inline int virtiofs_iomem_sync(struct virtiofs_iomem *iom __unused,
				      void *arg __unused)
{
	return 0;
}

#endif /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */

struct viofs_symnode {
	struct viofs_node node;
	char target[];
};

static inline
struct viofs_symnode *viofs_node2sym(struct viofs_node *n)
{
	return __containerof(n, struct viofs_symnode, node);
}

struct viofs_regnode {
	struct viofs_node node;
	struct virtiofs_iomem iomem;
};

static inline
struct viofs_regnode *viofs_node2reg(struct viofs_node *n)
{
	return __containerof(n, struct viofs_regnode, node);
}

UK_RB_KEY_GENERATE_STATIC(virtiofs_nodemap, viofs_node, rb_entry,
			  virtiofs_rb_node_cmp, virtiofs_rb_node_key);

UK_RB_HEAD(virtiofs_devmap, viofs_dev);
UK_RB_KEY_GENERATE_STATIC(virtiofs_devmap, viofs_dev, rb_entry,
			  virtiofs_rb_dev_cmp, virtiofs_rb_dev_key);

static struct {
	struct virtiofs_devmap devmap;
	struct uk_mutex maplk;
} opendevs = {
	.devmap = UK_RB_INITIALIZER(opendevs.devmap),
	.maplk = UK_MUTEX_INITIALIZER(opendevs.maplk)
};

static
int virtiofs_live_errnode(const struct viofs_node *n)
{
	return PTRISERR(n) ? PTR2ERR(n) : 0;
}

static
struct uk_file_state *virtiofs_live_state(struct viofs_node *n)
{
	return &n->fstate;
}

static
void virtiofs_live_acquire(struct viofs_node *n)
{
	uk_refcount_acquire(&n->refcnt);
}

static inline
int virtiofs_try_acquire(struct viofs_node *n)
{
	return uk_refcount_acquire_if_not_zero(&n->refcnt);
}

static inline
uint32_t virtiofs_node_fmode(const struct viofs_node *n)
{
	return n->attr.mode & S_IFMT;
}

static
int virtiofs_live_nodekind(const struct viofs_node *n,
			   enum uk_fs_tmpl_node_kind kind)
{
	uint32_t fmt = 0;

	switch (kind) {
	case UK_FS_TMPL_DIR:
		fmt = S_IFDIR;
		break;
	case UK_FS_TMPL_SYM:
		fmt = S_IFLNK;
		break;
	};
	return virtiofs_node_fmode(n) == fmt;
}

static
int virtiofs_node_fetchattr(struct viofs_node *n)
{
	const uint32_t fl = (n->flags & VIRTIOFS_NODE_OPEN) ?
			    FUSE_GETATTR_FH : 0;
	struct fuse_attr_out_trunc out;
	const union virtiofs_fuse_attr_outp op = { .trunc = &out };

	return virtiofs_fuse_getattr(&n->dev->fdev, n->inode, n->fh, fl,
				     op, &n->attr);
}

/* Convenience inline for FUSE_DESTROY that prints errors, but ignores them */
static inline
void fuse_destroy(struct viofs_dev *d)
{
	int r = virtiofs_fuse_destroy(&d->fdev);

	if (unlikely(r))
		/* Cannot meaningfully fail; print error and continue */
		uk_pr_err("DESTROY operation failed on dev %p: %d\n", d, r);
}

static
struct viofs_dev *virtiofs_dev_open(struct uk_virtiofs_dev *vd)
{
	struct viofs_dev *d;
	int err;

	d = uk_malloc(virtiofs_alloc, sizeof(*d));
	if (unlikely(!d)) {
		err = -ENOMEM;
		goto err_out;
	}
	virtiofs_init_dev(d, vd);

	err = uk_virtiofs_dev_configure(vd);
	if (unlikely(err))
		goto err_free;
	err = virtiofs_fuse_init(&d->fdev);
	if (unlikely(err))
		goto err_shutdown;
	virtiofs_init_node(&d->rootnode, d, FUSE_ROOT_ID, &d->rootnode);
	err = virtiofs_node_fetchattr(&d->rootnode);
	if (unlikely(err))
		goto err_destroy;

	return d;

err_destroy:
	fuse_destroy(d);
err_shutdown:
	uk_virtiofs_dev_shutdown(vd);
err_free:
	uk_free(virtiofs_alloc, d);
err_out:
	UK_ASSERT(err < 0);
	return ERR2PTR(err);
}

static
void virtiofs_dev_release(struct viofs_dev *d)
{
	UK_ASSERT(UK_RB_EMPTY(&d->nodemap));

	fuse_destroy(d);
	uk_virtiofs_dev_shutdown(d->fdev.dev);

	uk_mutex_lock(&opendevs.maplk);
	UK_RB_REMOVE(virtiofs_devmap, &opendevs.devmap, d);
	uk_mutex_unlock(&opendevs.maplk);

	uk_free(virtiofs_alloc, d);
}

static
struct viofs_node *virtiofs_live_vopen(union uk_fs_vopen_vol vol,
				       unsigned long flags __unused,
				       union uk_fs_vopen_data data __unused,
				       size_t fmt)
{
	struct uk_virtiofs_dev *vd;
	struct viofs_dev *d;

	if (unlikely(!(fmt & UK_FS_VOPEN_VOL_RAW)))
		return ERR2PTR(-EINVAL);
	if (unlikely(!vol.raw))
		return ERR2PTR(-EFAULT);

	vd = uk_virtiofs_dev_lookup(vol.raw);
	if (unlikely(!vd))
		return ERR2PTR(-ENOENT);

#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM
	/* Init the default system PoD, needed for file iomem */
	int r = uk_pod_default_init();

	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return ERR2PTR(r);
#endif /* CONFIG_LIBUKFS_VIRTIOFS_IOMEM */

	/* Lookup in open devs */
	for (;;) {
		uk_mutex_lock(&opendevs.maplk);
		d = UK_RB_FIND(virtiofs_devmap, &opendevs.devmap, vd);
		if (d) {
			if (unlikely(!virtiofs_try_acquire(&d->rootnode))) {
				/* Found dev mid-destruction; back off */
				uk_mutex_unlock(&opendevs.maplk);
				uk_sched_yield();
				continue;
			}
		} else {
			d = virtiofs_dev_open(vd);
			if (likely(!PTRISERR(d)))
				UK_RB_INSERT(virtiofs_devmap, &opendevs.devmap,
					     d);
		}
		uk_mutex_unlock(&opendevs.maplk);
		break;
	}
	UK_ASSERT(d);
	if (unlikely(PTRISERR(d)))
		return ERR2PTR(PTR2ERR(d));
	return &d->rootnode;
}

static
void virtiofs_node_close(struct viofs_node *n)
{
	int r;

	if (n->flags & VIRTIOFS_NODE_OPEN) {
		r = virtiofs_fuse_release(&n->dev->fdev, n->inode, n->fh);

		UK_ASSERT(r <= 0);
		if (unlikely(r))
			/* We cannot fail; log error */
			uk_pr_err("RELEASE(%" PRIu64 ") failed: %d\n",
				  n->inode, r);
	}
}

static
void virtiofs_node_forget(struct viofs_node *n)
{
	int r;

	r = virtiofs_fuse_forget(&n->dev->fdev, n->inode, 1);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		/* We cannot fail; log error */
		uk_pr_err("FORGET(%" PRIu64 ") failed: %d\n", n->inode, r);
}

static
void virtiofs_node_cleanup(struct viofs_node *n)
{
	/* Cleanup internal node state */
	if (virtiofs_node_fmode(n) == S_IFREG)
		virtiofs_iomem_cleanup(&viofs_node2reg(n)->iomem, n);
}

static
void virtiofs_node_free(struct viofs_node *n)
{
	void *p;

	switch (virtiofs_node_fmode(n)) {
	case S_IFLNK:
		p = viofs_node2sym(n);
		break;
	case S_IFREG:
		p = viofs_node2reg(n);
		break;
	default:
		p = n;
		break;
	}
	uk_free(virtiofs_alloc, p);
}

static
void virtiofs_live_release(struct viofs_node *n)
{
	const bool isroot = (n == &n->dev->rootnode);

	if (uk_refcount_release(&n->refcnt)) {
		virtiofs_node_cleanup(n);
		virtiofs_node_close(n);
		if (isroot) {
			virtiofs_dev_release(n->dev);
		} else {
			struct viofs_node *parent = n->parent;

			UK_ASSERT(n->inode != FUSE_ROOT_ID);

			virtiofs_node_forget(n);
			uk_mutex_lock(&n->dev->maplk);
			UK_RB_REMOVE(virtiofs_nodemap, &n->dev->nodemap, n);
			uk_mutex_unlock(&n->dev->maplk);

			virtiofs_node_free(n);
			virtiofs_live_release(parent);
		}
	}
}

static inline
void virtiofs_commit_open(struct viofs_node *n, struct fuse_open_out *out)
{
	uk_store_n(&n->fh, out->fh);
	uk_or(&n->flags, VIRTIOFS_NODE_OPEN);
}

static inline
int virtiofs_try_open(struct viofs_node *n)
{
	struct fuse_open_out out;
	int r;

	if (virtiofs_node_fmode(n) == S_IFDIR) {
		r = virtiofs_fuse_open(&n->dev->fdev, n->inode,
				       O_DIRECTORY, &out);
	} else if (virtiofs_node_fmode(n) == S_IFLNK) {
		r = virtiofs_fuse_open(&n->dev->fdev, n->inode,
				       O_NOFOLLOW | O_PATH, &out);
	} else {
		r = virtiofs_fuse_open(&n->dev->fdev, n->inode, O_RDWR, &out);
		if (r == -EACCES)
			r = virtiofs_fuse_open(&n->dev->fdev, n->inode,
					       O_RDONLY, &out);
	}
	if (!r)
		virtiofs_commit_open(n, &out);
	return r;
}

static
int virtiofs_ensure_open(struct viofs_node *n)
{
	int r = 0;

	if (!(n->flags & VIRTIOFS_NODE_OPEN)) {
		uk_mutex_lock(&n->openlk);
		/* Re-check flag after acquiring lock */
		if (!(uk_load_n(&n->flags) & VIRTIOFS_NODE_OPEN))
			r = virtiofs_try_open(n);
		uk_mutex_unlock(&n->openlk);
	}
	return r;
}

/* Live driver ops */

UK_FS_TMPL_LIVE_TYPES(virtiofs, struct viofs_node *);
UK_FS_TMPL_LIVE_OPS(virtiofs, struct viofs_node *);

#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM
static inline
ssize_t _io_read(struct viofs_node *n, struct virtiofs_iodest_ctx *ioctx,
		 size_t off, size_t rem, long flags,
		 const struct iovec *iov, size_t iovcnt,
		 size_t *iov_i, size_t *iov_off)
{
	char *buf = NULL;
	const size_t len = virtiofs_iodest(ioctx, off, &buf);
	const size_t toread = len ? MIN(len, rem) : rem;
	ssize_t r;

	if (buf) {
		/* Read from local iomem */
		UK_ASSERT(len);
		r = uk_iov_scatter(iov, iovcnt, buf, toread, iov_i, iov_off);
	} else {
		/* Read directly from device */
		if (*iov_off) {
			/* Issue read to partial iov */
			const size_t iolen = MIN(iov[*iov_i].iov_len - *iov_off,
						 toread);
			const struct iovec tmp = {
				.iov_base = ((char *)iov[*iov_i].iov_base) +
					    *iov_off,
				.iov_len = iolen
			};

			r = virtiofs_file_read(n, off, iolen, &tmp, 1, flags);
		} else {
			/* Issue read to whole iov(s) */
			r = virtiofs_file_read(n, off, toread, &iov[*iov_i],
					       iovcnt - *iov_i, flags);
		}
		if (r > 0)
			uk_iov_ff(iov, iovcnt, r, iov_i, iov_off);
	}
	return r;
}

static inline
ssize_t _do_read(struct viofs_node *n, const struct iovec *iov, size_t iovcnt,
		 size_t off, long flags)
{
	struct viofs_regnode *rn = viofs_node2reg(n);
	struct virtiofs_iodest_ctx ioctx;
	size_t bytes_read = 0;
	size_t iov_i = 0;
	size_t iov_off = 0;
	size_t rem;
	ssize_t r;

	virtiofs_iodest_prep(&rn->iomem, off, &ioctx);
	rem = uk_iov_len(iov, iovcnt);
	while (rem) {
		r = _io_read(n, &ioctx, off, rem, flags,
			     iov, iovcnt, &iov_i, &iov_off);
		if (unlikely(r <= 0))
			break;
		bytes_read += r;
		off += r;
		rem -= r;
	}
	return bytes_read ? (ssize_t)bytes_read : r;
}
#else /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */
static inline
ssize_t _do_read(struct viofs_node *n, const struct iovec *iov, size_t iovcnt,
		 size_t off, long flags)
{
	return virtiofs_file_read(n, off, uk_iov_len(iov, iovcnt),
				  iov, iovcnt, flags);
}
#endif /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */

static
ssize_t virtiofs_live_read(struct viofs_node *n,
			   const struct iovec *iov, size_t iovcnt, size_t off,
			   long flags, unsigned long mntflags __unused)
{
	ssize_t r;

	if (unlikely(virtiofs_node_fmode(n) == S_IFDIR))
		return -EISDIR;
	if (unlikely(virtiofs_node_fmode(n) == S_IFLNK))
		return -EBADF;

	r = virtiofs_ensure_open(n);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;
	return _do_read(n, iov, iovcnt, off, flags);
}

#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM
static inline
ssize_t _io_write(struct viofs_node *n, struct virtiofs_iodest_ctx *ioctx,
		  size_t off, size_t rem, long flags,
		  const struct iovec *iov, size_t iovcnt,
		  size_t *iov_i, size_t *iov_off)
{
	char *buf = NULL;
	const size_t len = virtiofs_iodest(ioctx, off, &buf);
	const size_t towrite = len ? MIN(len, rem) : rem;
	ssize_t r;

	if (buf) {
		/* Write to local iomem */
		UK_ASSERT(len);
		r = uk_iov_gather(buf, iov, iovcnt, towrite, iov_i, iov_off);
	} else {
		/* Write directly to device */
		if (*iov_off) {
			/* Issue write from partial iov */
			const size_t iolen = MIN(iov[*iov_i].iov_len - *iov_off,
						 towrite);
			const struct iovec tmp = {
				.iov_base = ((char *)iov[*iov_i].iov_base) +
					    *iov_off,
				.iov_len = iolen
			};

			r = virtiofs_file_write(n, off, iolen, &tmp, 1, flags);
		} else {
			/* Issue write from whole iov(s) */
			r = virtiofs_file_write(n, off, towrite,
						&iov[*iov_i], iovcnt - *iov_i,
						flags);
		}
		if (r > 0)
			uk_iov_ff(iov, iovcnt, r, iov_i, iov_off);
	}
	return r;
}

static inline
ssize_t _do_write(struct viofs_node *n, const struct iovec *iov, size_t iovcnt,
		  size_t off, long flags)
{
	struct viofs_regnode *rn = viofs_node2reg(n);
	struct virtiofs_iodest_ctx ioctx;
	size_t bytes_written = 0;
	size_t iov_i = 0;
	size_t iov_off = 0;
	size_t rem = uk_iov_len(iov, iovcnt);
	ssize_t r = 0;

	virtiofs_iodest_prep(&rn->iomem, off, &ioctx);
	while (rem) {
		r = _io_write(n, &ioctx, off, rem, flags,
			      iov, iovcnt, &iov_i, &iov_off);
		if (unlikely(r <= 0))
			break;
		bytes_written += r;
		off += r;
		rem -= r;
	}
	return bytes_written ? (ssize_t)bytes_written : r;
}
#else /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */
static inline
ssize_t _do_write(struct viofs_node *n, const struct iovec *iov, size_t iovcnt,
		  size_t off, long flags)
{
	return virtiofs_file_write(n, off, uk_iov_len(iov, iovcnt),
				   iov, iovcnt, flags);
}
#endif /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */

static
ssize_t virtiofs_live_write(struct viofs_node *n,
			    const struct iovec *iov, size_t iovcnt, size_t off,
			    long flags, unsigned long mntflags __unused)
{
	ssize_t r;

	if (unlikely(virtiofs_node_fmode(n) == S_IFDIR))
		return -EISDIR;
	if (unlikely(virtiofs_node_fmode(n) == S_IFLNK))
		return -EBADF;

	r = virtiofs_ensure_open(n);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;
	r = _do_write(n, iov, iovcnt, off, flags);
	if (r > 0 && off + r > n->attr.size)
		/* Update local file size if it has grown */
		n->attr.size = off + r;
	return r;
}

#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM
static
ssize_t virtiofs_live_mem(struct viofs_node *n,
			  enum uk_file_mem_op op, size_t off, size_t len,
			  struct iovec *iov, size_t iovcnt,
			  unsigned long mntflags __unused)
{
	const struct uk_sparsebuf_ctx ctx = {
		.funcs = &virtiofs_sbfuncs,
		.alloc = virtiofs_alloc,
		.arg = n
	};
	struct viofs_regnode *rn = viofs_node2reg(n);
	ssize_t r;
	UK_SPARSEBUF_EMBED_HEAD(iohead, &rn->iomem.io);

	if (unlikely(virtiofs_node_fmode(n) == S_IFDIR))
		return -EISDIR;
	if (unlikely(virtiofs_node_fmode(n) == S_IFLNK))
		return -EBADF;

	switch (op) {
	case UKFILE_MEM_RETRIEVE:
	case UKFILE_MEM_BORROW:
		if (unlikely(!iovcnt))
			return 0;
		__fallthrough;
	case UKFILE_MEM_ACQUIRE:
	case UKFILE_MEM_RELEASE:
		if (unlikely(off >= n->attr.size))
			return 0;
		if (off + len > n->attr.size)
			len = n->attr.size - off;
		break;
	case UKFILE_MEM_GIFT:
		return -ENODEV;
	default:
		return -EINVAL;
	}

	/* Need to declare here; len may have been adjusted by previous block */
	const size_t pgoff = off / PAGE_SIZE;
	const size_t pgend = ALIGN_UP(off + len, PAGE_SIZE) / PAGE_SIZE;
	const size_t npages = pgend - pgoff;

	r = virtiofs_ensure_open(n);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;

	switch (op) {
	case UKFILE_MEM_ACQUIRE:
		r = uk_sparsebuf_fill(
			&ctx, UK_SPARSEBUF_EMBED_HEADP(iohead, &rn->iomem.io),
			pgoff, npages, 1
		);
		if (unlikely(r <= 0))
			return r;
		return (pgoff + r) * PAGE_SIZE - off;
	case UKFILE_MEM_RELEASE:
		r = uk_sparsebuf_ref_release(
			&ctx, UK_SPARSEBUF_EMBED_HEADP(iohead, &rn->iomem.io),
			pgoff, npages, 1
		);
		UK_ASSERT(r <= 0);
		/* cannot fail, print warning */
		if (unlikely(r))
			uk_pr_err("Cannot release iomem 0x%zx + 0x%zx: %zd\n",
				  off, len, r);
		return 0;
	case UKFILE_MEM_RETRIEVE:
	case UKFILE_MEM_BORROW:
	{
		struct uk_sparsebuf_cur cur;
		size_t iov_i = 0;

		r = uk_sparsebuf_lookup(
			UK_SPARSEBUF_EMBED_HEADP(iohead, &rn->iomem.io),
			pgoff, &cur
		);
		if (unlikely(!r))
			return -ENOENT;
		while (iov_i < iovcnt) {
			char *buf;
			size_t blen = uk_sparsebuf_memat(off, &cur, &buf);

			if (unlikely(!blen || !buf)) {
				if (iov_i)
					break;
				return -ENOENT;
			}
			iov[iov_i].iov_base = buf;
			iov[iov_i].iov_len = blen;
			iov_i++;
			off += blen;
		}
		return iov_i;
	}
	default:
		UK_CRASH("Impossible");
	}
}
#else /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */
static
ssize_t virtiofs_live_mem(struct viofs_node *n __unused,
			  enum uk_file_mem_op op __unused,
			  size_t off __unused, size_t len __unused,
			  struct iovec *iov __unused, size_t iovcnt __unused,
			  unsigned long mntflags __unused)
{
	return -ENODEV;
}
#endif /* !CONFIG_LIBUKFS_VIRTIOFS_IOMEM */

static inline
void virtiofs_attr2statx(const struct fuse_attr *attr, struct uk_statx *sx)
{
	sx->stx_blksize = attr->blksize;
	sx->stx_nlink = attr->nlink;
	sx->stx_uid = attr->uid;
	sx->stx_gid = attr->gid;
	sx->stx_mode = attr->mode;
	sx->stx_ino = attr->ino;
	sx->stx_size = attr->size;
	sx->stx_blocks = attr->blocks;
	sx->stx_atime = (struct uk_statx_timestamp){
		.tv_sec = attr->atime,
		.tv_nsec = attr->atimensec
	};
	sx->stx_ctime = (struct uk_statx_timestamp){
		.tv_sec = attr->ctime,
		.tv_nsec = attr->ctimensec
	};
	sx->stx_mtime = (struct uk_statx_timestamp){
		.tv_sec = attr->mtime,
		.tv_nsec = attr->mtimensec
	};
	sx->stx_rdev_major = major(attr->rdev);
	sx->stx_rdev_minor = minor(attr->rdev);
}

#define VIRTIOFS_SETTABLE_STATS \
	(UK_STATX_MODE | UK_STATX_UID | UK_STATX_GID | UK_STATX_ATIME | \
	 UK_STATX_CTIME | UK_STATX_MTIME)

#define VIRTIOFS_VOLATILE_STATS \
	(VIRTIOFS_SETTABLE_STATS | UK_STATX_NLINK | UK_STATX_BTIME | \
	 UK_STATX_SIZE | UK_STATX_BLOCKS)

static
int virtiofs_live_getstat(struct viofs_node *n,
			  unsigned int mask, struct uk_statx *arg,
			  unsigned long mntflags __unused)
{
	/* statx op pending support by VMM; can only return basic stats */
	mask &= UK_STATX_BASIC_STATS;
	if (mask & VIRTIOFS_VOLATILE_STATS) {
		/* TODO: check validity & use cached vals w/ attr timeout */
		int r = virtiofs_node_fetchattr(n);

		UK_ASSERT(r <= 0);
		if (unlikely(r))
			return r;
	}
	/* We can return all basic stats at no cost */
	mask |= UK_STATX_BASIC_STATS;
	virtiofs_attr2statx(&n->attr, arg);
	arg->stx_mask = mask;
	return 0;
}

static
int virtiofs_live_setstat(struct viofs_node *n,
			  unsigned int mask, const struct uk_statx *arg,
			  unsigned long mntflags __unused)
{
	struct fuse_setattr_in in;
	struct fuse_attr_out_trunc out;
	const union virtiofs_fuse_attr_outp op = { .trunc = &out };
	uint32_t valid = 0;

	if (unlikely(mask & ~VIRTIOFS_SETTABLE_STATS))
		return -EINVAL;

	if (n->flags & VIRTIOFS_NODE_OPEN) {
		in.fh = n->fh;
		valid |= FATTR_FH;
	}
	if (mask & UK_STATX_MODE) {
		in.mode = arg->stx_mode;
		valid |= FATTR_MODE;
	}
	if (mask & UK_STATX_UID) {
		in.uid = arg->stx_uid;
		valid |= FATTR_UID;
	}
	if (mask & UK_STATX_GID) {
		in.gid = arg->stx_gid;
		valid |= FATTR_GID;
	}
	if (mask & UK_STATX_ATIME) {
		in.atime = arg->stx_atime.tv_sec;
		in.atimensec = arg->stx_atime.tv_nsec;
		valid |= FATTR_ATIME;
	}
	if (mask & UK_STATX_CTIME) {
		in.ctime = arg->stx_ctime.tv_sec;
		in.ctimensec = arg->stx_ctime.tv_nsec;
		valid |= FATTR_CTIME;
	}
	if (mask & UK_STATX_MTIME) {
		in.mtime = arg->stx_mtime.tv_sec;
		in.mtimensec = arg->stx_mtime.tv_nsec;
		valid |= FATTR_MTIME;
	}
	in.valid = valid;

	return virtiofs_fuse_setattr(&n->dev->fdev, n->inode, &in,
				     op, &n->attr);
}

static
int virtiofs_fsync(struct viofs_node *n, uint32_t flags)
{
	int r = virtiofs_ensure_open(n);

	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;
	/* Writeback any local iomem */
	if (virtiofs_node_fmode(n) == S_IFREG) {
		r = virtiofs_iomem_sync(&viofs_node2reg(n)->iomem, n);
		UK_ASSERT(r <= 0);
		if (unlikely(r))
			return r;
	}
	/* Send fsync to device */
	return virtiofs_fuse_fsync(&n->dev->fdev, n->inode, n->fh, flags);
}

static
int virtiofs_ftrunc(struct viofs_node *n, uint64_t len)
{
	const uint64_t prevlen __maybe_unused = n->attr.size;
	struct fuse_setattr_in in;
	struct fuse_attr_out_trunc out;
	const union virtiofs_fuse_attr_outp op = { .trunc = &out };
	int r;

	in.size = len;
	in.valid = FATTR_SIZE;
	if (n->flags & VIRTIOFS_NODE_OPEN) {
		in.fh = n->fh;
		in.valid |= FATTR_FH;
	}

	r = virtiofs_fuse_setattr(&n->dev->fdev, n->inode, &in, op, &n->attr);
	if (unlikely(r))
		return r;

#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM
	/* Update local iomem */
	if (virtiofs_node_fmode(n) == S_IFREG && len < prevlen) {
		r = virtiofs_iomem_trunc(&viofs_node2reg(n)->iomem, len, n);
		/* Cannot gracefully undo, assert success */
		UK_ASSERT(!r);
	}
#endif /* CONFIG_LIBUKFS_VIRTIOFS_IOMEM */
	return r;
}

static
int virtiofs_falloc(struct viofs_node *n, uint32_t mode,
		    uint64_t off, uint64_t len)
{
	struct viofs_regnode *rn __maybe_unused = viofs_node2reg(n);
	int r;

	if (unlikely(virtiofs_node_fmode(n) != S_IFREG))
		return -ENODEV;

	r = virtiofs_ensure_open(n);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;

	/* Validate mode */
	switch (mode) {
	case 0:
	case FALLOC_FL_KEEP_SIZE:
	case FALLOC_FL_ZERO_RANGE:
	case FALLOC_FL_ZERO_RANGE | FALLOC_FL_KEEP_SIZE:
	case FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE:
	case FALLOC_FL_COLLAPSE_RANGE:
	case FALLOC_FL_INSERT_RANGE:
		break;
	default:
		return -EINVAL;
	}

#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM
	/* Validate w/ local iomem */
	switch (mode) {
	case FALLOC_FL_COLLAPSE_RANGE:
	case FALLOC_FL_INSERT_RANGE:
		if (virtiofs_iomem_after(&rn->iomem, off))
			return -ETXTBSY;
		break;
	}
#endif /* CONFIG_LIBUKFS_VIRTIOFS_IOMEM */

	r = virtiofs_ensure_open(n);
	if (unlikely(r))
		return r;
	r = virtiofs_fuse_fallocate(&n->dev->fdev, n->inode, n->fh,
				    off, len, mode);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;

	/* Update local state post fallocate */
#if CONFIG_LIBUKFS_VIRTIOFS_IOMEM
	/* Update local iomem */
	switch (mode) {
	case FALLOC_FL_ZERO_RANGE:
	case FALLOC_FL_ZERO_RANGE | FALLOC_FL_KEEP_SIZE:
	case FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE:
		/* Local iomem needs scooping */
		r = virtiofs_iomem_scoop(&rn->iomem, off, len, n);
		UK_ASSERT(!r);
		break;
	}
#endif /* CONFIG_LIBUKFS_VIRTIOFS_IOMEM */
	/* Update cached size */
	switch (mode) {
	case 0:
	case FALLOC_FL_COLLAPSE_RANGE:
	case FALLOC_FL_INSERT_RANGE:
	case FALLOC_FL_ZERO_RANGE:
		/* Size may have changed; re-fetch from file */
		r = virtiofs_node_fetchattr(n);
		UK_ASSERT(r <= 0);
		if (unlikely(r))
			/* Fallocate did not fail => no error out, just warn */
			uk_pr_err("Fetchattr failed: %d\n", r);
	}
	return 0;
}

static
int virtiofs_live_ctl(struct viofs_node *n, int fam, int req,
		      uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
		      unsigned long mntflags __unused)
{
	switch (fam) {
	case UKFILE_CTL_FILE:
		switch (req) {
		case UKFILE_CTL_FILE_SYNC:
			return virtiofs_fsync(n,
					      arg1 ? 0 : FUSE_FSYNC_FDATASYNC);
		case UKFILE_CTL_FILE_TRUNC:
			return virtiofs_ftrunc(n, (uint64_t)arg1);
		case UKFILE_CTL_FILE_FALLOC:
			return virtiofs_falloc(n, (uint32_t)arg1,
					       (uint64_t)arg2, (uint64_t)arg3);
		case UKFILE_CTL_FILE_FADVISE:
			/* TODO: maybe pre-read? */
			return 0;
		default:
			return -EINVAL;
		}
	case UKFILE_CTL_IOCTL:
		/* STUB: pending VMM support */
		return -ENOSYS;
	default:
		return -EINVAL;
	}
}

static
int virtiofs_live_fs_stat(struct viofs_node *n, struct statfs *buf)
{
	struct fuse_statfs_out out;
	int r;

	r = virtiofs_fuse_statfs(&n->dev->fdev, &out);
	if (likely(!r)) {
		buf->f_type = 0x65735546; /* FUSE fs type */
		buf->f_bsize = out.st.bsize;
		buf->f_blocks = out.st.blocks;
		buf->f_bfree = out.st.bfree;
		buf->f_bavail = out.st.bavail;
		buf->f_files = out.st.files;
		buf->f_ffree = out.st.ffree;
		buf->f_fsid = (fsid_t){{ 0, 0 }};
		buf->f_namelen = out.st.namelen;
		buf->f_frsize = out.st.frsize;
		buf->f_flags = 0;
	}
	return r;
}

static
int virtiofs_live_fs_sync(struct viofs_node *n)
{
	return virtiofs_fuse_syncfs(&n->dev->fdev);
}

static
struct viofs_node *virtiofs_newsym(uint64_t inode, size_t target_len,
				   struct viofs_node *parent)
{
	struct viofs_symnode *sn;
	const size_t nodesz = sizeof(*sn) + target_len + 1;
	ssize_t r;

	sn = uk_malloc(virtiofs_alloc, nodesz);
	if (unlikely(!sn))
		return ERR2PTR(-ENOMEM);
	r = virtiofs_fuse_readlink(&parent->dev->fdev,
				   inode, sn->target, target_len);
	if (unlikely(r < 0)) {
		uk_free(virtiofs_alloc, sn);
		return ERR2PTR(r);
	}
	UK_ASSERT((size_t)r == target_len);
	sn->target[target_len] = '\0';
	return &sn->node;
}

static
struct viofs_node *virtiofs_newreg(void)
{
	struct viofs_regnode *rn = uk_malloc(virtiofs_alloc, sizeof(*rn));

	if (unlikely(!rn))
		return ERR2PTR(-ENOMEM);
	virtiofs_iomem_init(&rn->iomem);
	return &rn->node;
}

static
struct viofs_node *virtiofs_newplain(void)
{
	struct viofs_node *n = uk_malloc(virtiofs_alloc, sizeof(*n));

	return n ? n : ERR2PTR(-ENOMEM);
}

static
struct viofs_node *virtiofs_newnode(struct fuse_entry_out *out,
				    struct viofs_node *parent)
{
	struct viofs_node *newnode;
	/* Symlinks are immutable; we do readlink on node alloc */
	if ((out->attr.mode & S_IFMT) == S_IFLNK)
		newnode = virtiofs_newsym(out->nodeid, out->attr.size, parent);
	else if ((out->attr.mode & S_IFMT) == S_IFREG)
		newnode = virtiofs_newreg();
	else
		newnode = virtiofs_newplain();
	return newnode;
}

static
struct viofs_node *virtiofs_getnode(struct fuse_entry_out *out,
				    struct viofs_node *parent)
{
	struct viofs_dev *const dev = parent->dev;
	struct viofs_node *newnode;
	bool update_attr = false;

	/* Lookup inode in nodemap */
	for (;;) {
		uk_mutex_lock(&dev->maplk);
		newnode = UK_RB_FIND(virtiofs_nodemap, &dev->nodemap,
				     out->nodeid);
		if (newnode) {
			if (unlikely(!virtiofs_try_acquire(newnode))) {
				/* Found node mid-destruction; back off */
				uk_mutex_unlock(&dev->maplk);
				uk_sched_yield();
				continue;
			}
			/* Existing node; update attr after unlock */
			update_attr = true;
		} else {
			newnode = virtiofs_newnode(out, parent);
			if (!PTRISERR(newnode)) {
				virtiofs_live_acquire(parent);
				virtiofs_init_node(newnode, dev,
						   out->nodeid, parent);
				/* New node; write valid attr before insert */
				newnode->attr = out->attr;
				UK_RB_INSERT(virtiofs_nodemap, &dev->nodemap,
					     newnode);
			}
		}
		uk_mutex_unlock(&dev->maplk);
		break;
	}
	if (update_attr)
		newnode->attr = out->attr;
	return newnode;
}

static
int virtiofs_live_fs_lookup(struct viofs_node *n,
			    const char *path, size_t len,
			    struct viofs_node **out_node,
			    union uk_fs_lookup_out *out_ukfs __unused,
			    size_t *nout)
{
	struct viofs_node *newnode;
	struct fuse_entry_out out;
	const union virtiofs_fuse_entry_outp op = { .full = &out };
	size_t cur = 0;
	int r;

	if (unlikely(len && virtiofs_node_fmode(n) != S_IFDIR)) {
		*nout = 0;
		return UKFS_STOP_END;
	}

	/* Find first separator */
	while (cur < len && path[cur] != '/')
		cur++;
	/* Empty paths & '.' should be handled by upper levels */
	UK_ASSERT(cur);
	UK_ASSERT(!uk_fs_path_isdot(path, cur));

	/* Handle '..' */
	if (uk_fs_path_isdotdot(path, cur)) {
		virtiofs_live_acquire(n->parent);
		out_node[0] = n->parent;
		*nout = cur;
		return UKFS_STOP_NOD;
	}

	r = virtiofs_fuse_lookup(&n->dev->fdev, n->inode, path, cur, op, NULL);
	UK_ASSERT(r <= 0);
	if (unlikely(r < 0))
		return r;

	newnode = virtiofs_getnode(&out, n);
	if (unlikely(PTRISERR(newnode))) {
		r = PTR2ERR(newnode);
		goto out_noderr;
	}

	switch (virtiofs_node_fmode(newnode)) {
	case S_IFLNK:
		virtiofs_live_acquire(n);
		out_node[0] = newnode;
		out_node[1] = n;
		*nout = cur;
		return UKFS_STOP_SYM;
	case S_IFREG:
	case S_IFDIR:
		out_node[0] = newnode;
		*nout = cur;
		return UKFS_STOP_NOD;
	default:
		/* Unknown file type; forget & report not found */
		/* TODO: handle special files */
		virtiofs_live_release(newnode);
		r = -ENOENT;
		break;
	}

out_noderr:
	virtiofs_fuse_forget(&n->dev->fdev, out.nodeid, 1);
	return r;
}

static
ssize_t virtiofs_live_fs_listdir(struct viofs_node *n, size_t *curp,
				 void *buf, size_t len,
				 unsigned long mntflags __unused)
{
	/* TODO: pick a good size / make configurable */
	const size_t max_stacklen = 0x1000;

	bool use_stack = (len <= max_stacklen);
	struct fuse_dirent *fuse_dent;
	struct uk_fs_dirent *fs_dent;
	void *tmp = NULL;
	size_t cur = *curp;
	size_t tmplen;
	size_t rem;
	ssize_t r;

	/* FUSE dirents are equal or larger in size to equivalent dirent64s,
	 * thus the contents of a FUSE dirent buffer will always fit into a
	 * dirent64 buffer of the same size; we can use `len` for temp buffer.
	 */

	if (!use_stack)
		tmp = uk_malloc(virtiofs_alloc, len);

	/* Must declare it here, as size is runtime computed */
	char tmpbuf[tmp ? 0 : MIN(len, max_stacklen)];

	if (tmp) {
		tmplen = len;
	} else {
		tmp = tmpbuf;
		use_stack = true;
		tmplen = MIN(len, max_stacklen);
	}

	r = virtiofs_ensure_open(n);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		goto out;

	/* TODO: Use readdirplus if/when lookup cache */
	r = virtiofs_fuse_readdir(&n->dev->fdev, n->inode, n->fh,
				  cur, tmp, tmplen);
	if (unlikely(r < 0))
		goto out;
	if (unlikely((size_t)r > tmplen)) {
		/* Device is talking nonsense, report I/O error */
		r = -EIO;
		goto out;
	}

	/* Convert FUSE output to dirent64 */
	*curp = cur + r;
	rem = r;
	fuse_dent = tmp;
	fs_dent = buf;
	r = 0;
	while (rem) {
		const uint32_t namelen = fuse_dent->namelen;
		const unsigned short reclen = UKFS_DIRENT_RECLEN(namelen);
		const size_t fuselen = FUSE_DIRENT_SIZE(fuse_dent);

		UK_ASSERT(rem >= fuselen);
		UK_ASSERT(UKFS_DIRENT_RECLEN(namelen) <= USHRT_MAX);
		UK_ASSERT(len - r >= reclen);
		UK_ASSERT(fuse_dent->type <= UCHAR_MAX);

		fs_dent->d_ino = fuse_dent->ino;
		fs_dent->d_off = fuse_dent->off;
		fs_dent->d_reclen = reclen;
		fs_dent->d_type = fuse_dent->type;
		memcpy(fs_dent->d_name, fuse_dent->name, namelen);
		fs_dent->d_name[namelen] = '\0';

		r += reclen;
		rem -= fuselen;
		fs_dent = UKFS_DIRENT_NEXT(fs_dent, reclen);
		fuse_dent = (struct fuse_dirent *)((char *)fuse_dent + fuselen);
	}
out:
	if (!use_stack)
		uk_free(virtiofs_alloc, tmp);
	return r;
}

static
struct viofs_node *virtiofs_live_fs_create(struct viofs_node *n,
	const char *name, size_t len, unsigned int mode, int flags,
	union UK_FS_TMPL_LIVE_CREATE_TARGET(virtiofs) target,
	unsigned long mntflags __unused)
{
	int r;

	switch (mode & S_IFMT) {
	case S_IFMT: /* link */
	{
		struct viofs_node *target_node = target.livenode;
		struct fuse_entry_out out;
		const union virtiofs_fuse_entry_outp op = { .full = &out };
		struct viofs_node *newnode;

		/* Device cross-check is performed by upper layers */
		UK_ASSERT(target_node->dev == n->dev);
		r = virtiofs_fuse_link(&n->dev->fdev, n->inode,
				       target_node->inode,
				       name, len, op, NULL);
		UK_ASSERT(r <= 0);
		if (unlikely(r))
			return ERR2PTR(r);
		newnode = virtiofs_getnode(&out, n);
		UK_ASSERT(newnode == target_node);
		return newnode;
	}
	case S_IFLNK: /* symlink */
	{
		const char *sym_target = target.ukfs.path;
		const size_t linklen = strlen(sym_target);
		struct fuse_entry_out out;
		const union virtiofs_fuse_entry_outp op = { .full = &out };

		r = virtiofs_fuse_symlink(&n->dev->fdev, n->inode, name, len,
					  sym_target, linklen, op, NULL);
		UK_ASSERT(r <= 0);
		if (unlikely(r))
			return ERR2PTR(r);
		return virtiofs_getnode(&out, n);
	}
	case S_IFDIR: /* mkdir */
	{
		const struct fuse_mkdir_in in = {
			.mode = mode,
			.umask = 0 /* umask is already applied into mode */
		};
		struct fuse_entry_out out;
		const union virtiofs_fuse_entry_outp op = { .full = &out };

		r = virtiofs_fuse_mkdir(&n->dev->fdev, n->inode, name, len,
					&in, op, NULL);
		UK_ASSERT(r <= 0);
		if (unlikely(r))
			return ERR2PTR(r);
		return virtiofs_getnode(&out, n);
	}
	case S_IFREG: /* create */
	{
		const struct fuse_create_in in = {
			.flags = flags | O_RDWR,
			.mode = mode,
			.umask = 0 /* umask is already applied into mode */
		};
		struct fuse_entry_out_trunc entry_out;
		const union virtiofs_fuse_entry_outp op = {
			.trunc = &entry_out
		};
		struct fuse_open_out open_out;
		struct viofs_node *newnode;

		newnode = virtiofs_newreg();
		if (unlikely(PTRISERR(newnode)))
			return newnode;

		r = virtiofs_fuse_create(&n->dev->fdev, n->inode, name, len,
					 &in, op, &newnode->attr, &open_out);
		UK_ASSERT(r <= 0);
		if (unlikely(r)) {
			uk_free(virtiofs_alloc, newnode);
			return ERR2PTR(r);
		}

		virtiofs_live_acquire(n);
		virtiofs_init_node(newnode, n->dev, entry_out.nodeid, n);
		UK_ASSERT(virtiofs_node_fmode(n) == S_IFREG);
		virtiofs_commit_open(newnode, &open_out);
		uk_mutex_lock(&n->dev->maplk);
		UK_RB_INSERT(virtiofs_nodemap, &n->dev->nodemap, newnode);
		uk_mutex_unlock(&n->dev->maplk);

		return newnode;
	}
	case S_IFCHR:
	case S_IFBLK: /* TODO: mknod */
	default:
		return ERR2PTR(-EINVAL);
	}
}

static
int virtiofs_live_fs_unlink(struct viofs_node *n,
			    const char *name, size_t len, unsigned int flags,
			    unsigned long mntflags __unused)
{
	int r;

	/* Refuse to operate on '.' and '..' */
	if (unlikely(uk_fs_path_isdot(name, len) ||
		     uk_fs_path_isdotdot(name, len)))
		return -EINVAL;

	/* Mux between unlink & rmdir based on flags */
	if (flags & UKFS_UNLINK_DIR)
		return virtiofs_fuse_rmdir(&n->dev->fdev, n->inode, name, len);
	else if (flags & UKFS_UNLINK_NODIR)
		return virtiofs_fuse_unlink(&n->dev->fdev, n->inode, name, len);
	/* ... or try both */
	r = virtiofs_fuse_unlink(&n->dev->fdev, n->inode, name, len);
	if (r == -EISDIR)
		r = virtiofs_fuse_rmdir(&n->dev->fdev, n->inode,
					name, len);
	return r;
}

static
int virtiofs_live_fs_rename(struct viofs_node *n,
			    const char *name, size_t nlen,
			    struct viofs_node *dest,
			    const char *dname, size_t dlen,
			    unsigned int flags, unsigned long mntflags __unused)
{
	/* rename2 is not supported by all devices; use it only when required */
	if (flags)
		return virtiofs_fuse_rename2(&n->dev->fdev,
					     n->inode, name, nlen,
					     dest->inode, dname, dlen, flags);
	else
		return virtiofs_fuse_rename(&n->dev->fdev,
					    n->inode, name, nlen,
					    dest->inode, dname, dlen);
}

static
struct uk_fs_path virtiofs_live_fs_readlink(struct viofs_node *n)
{
	UK_ASSERT(virtiofs_node_fmode(n) == S_IFLNK);
	UK_ASSERT(n->attr.size);
	/* Symlink targets are cached on node creation */
	return (struct uk_fs_path){
		.s = viofs_node2sym(n)->target,
		.len = n->attr.size
	};
}

UK_FS_TMPL_LIVE_OPSTABLE(virtiofs, virtiofs_live_ops);

static const struct virtiofs_live_ops virtiofs_liveops = {
	.live_vopen = virtiofs_live_vopen,
	.live_read = virtiofs_live_read,
	.live_write = virtiofs_live_write,
	.live_mem = virtiofs_live_mem,
	.live_getstat = virtiofs_live_getstat,
	.live_setstat = virtiofs_live_setstat,
	.live_ctl = virtiofs_live_ctl,
	.live_fs_stat = virtiofs_live_fs_stat,
	.live_fs_sync = virtiofs_live_fs_sync,
	.live_fs_lookup = virtiofs_live_fs_lookup,
	.live_fs_listdir = virtiofs_live_fs_listdir,
	.live_fs_create = virtiofs_live_fs_create,
	.live_fs_unlink = virtiofs_live_fs_unlink,
	.live_fs_rename = virtiofs_live_fs_rename,
	.live_fs_readlink = virtiofs_live_fs_readlink,
	.live_nodekind = virtiofs_live_nodekind,
	.live_state = virtiofs_live_state,
	.live_errnode = virtiofs_live_errnode,
	.live_acquire = virtiofs_live_acquire,
	.live_release = virtiofs_live_release
};

static
int virtiofs_nodecmp(struct viofs_node *a, struct viofs_node *b)
{
	return a < b ? -1 : a == b ? 0 : 1;
}

UK_FS_TMPL_LIVE_GENERATE_STATIC(virtiofs, struct viofs_node *, virtiofs_nodecmp,
				virtiofs_liveops, 0);

UK_FS_DRIVER_REGISTER(virtiofs, UK_FS_TMPL_LIVE_OP_VOPEN(virtiofs),
		      UK_FS_VOPEN_VOL_RAW | UK_FS_VOPEN_DATA_IGNORE);
