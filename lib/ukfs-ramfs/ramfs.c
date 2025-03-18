/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE

/* Memory resident volatile filesystem */

#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <linux/falloc.h>

#include <uk/arch/paging.h>
#include <uk/alloc.h>
#include <uk/atomic.h>
#include <uk/errptr.h>
#include <uk/essentials.h>
#include <uk/file.h>
#include <uk/file/iovutil.h>
#include <uk/fs.h>
#include <uk/fs/dirent.h>
#include <uk/fs/driver.h>
#include <uk/fs/template/live.h>
#include <uk/init.h>
#include <uk/posix-fd.h>
#include <uk/posix-time.h>
#include <uk/spinlock.h>
#include <uk/weak_refcount.h>
#include <uk/tree.h>

/* RAMfs allocates memory from 2 sources:
 * - a driver-wide object allocator for metadata
 * - the system PoD for locked pages backing file contents
 */

/* Implemented in "ramfs-file.h"; shared alloc stored in larger struct */
static struct uk_alloc *ramfs_alloc(void);

static inline
void *ramfs_obj_alloc(size_t len)
{
	return uk_malloc(ramfs_alloc(), len);
}

static inline
void ramfs_obj_free(void *p)
{
	uk_free(ramfs_alloc(), p);
}

/* Implementation of files */
#include "ramfs-file.h"

/*
 * Data structures & internal utils
 */

struct ramfs_node;

union ramfs_dentry_target {
	struct ramfs_node *node;
	const struct uk_file *file;
	struct uk_fs_specfile special;
};

#define RAMFS_DENT_NODE 0
#define RAMFS_DENT_FILE 1
#define RAMFS_DENT_SPEC 2

/* RAMfs directory entry; associates a filename with a filesystem node */
struct ramfs_dentry {
	/* Bindings for lookup by name */
	UK_RB_ENTRY(ramfs_dentry) rb_name;
	/* Node referenced by this dentry */
	union ramfs_dentry_target target;
	/* Stuff used to fill a dirent64 */
	unsigned short namelen;
	unsigned char type;
	char name[];
};

#define RAMFS_MAX_NAMELEN USHRT_MAX

static
struct ramfs_dentry *ramfs_dentry_new(const char *name, unsigned short namelen)
{
	struct ramfs_dentry *ret = ramfs_obj_alloc(sizeof(*ret) + namelen);

	if (ret) {
		ret->namelen = namelen;
		memcpy(ret->name, name, namelen);
	}
	return ret;
}

struct ramfs_entry_name {
	size_t len;
	const char *name;
};

static struct ramfs_entry_name ramfs_rb_name_key(struct ramfs_dentry *ent)
{
	return (struct ramfs_entry_name){
		.len = ent->namelen,
		.name = ent->name
	};
}

static int ramfs_rb_name_cmp(struct ramfs_entry_name a,
			     struct ramfs_entry_name b)
{
	int r = memcmp(a.name, b.name, MIN(a.len, b.len));

	return r ? r : (int)(a.len - b.len);
}

UK_RB_HEAD(ramfs_name_map, ramfs_dentry);
UK_RB_KEY_GENERATE_STATIC(ramfs_name_map, ramfs_dentry, rb_name,
			  ramfs_rb_name_cmp, ramfs_rb_name_key);

/* Directory-specific node data */
struct ramfs_dir_data {
	struct ramfs_name_map children;
	struct ramfs_node *parent;
	struct uk_spinlock plock;
};

/* Symlink-specific node data */
struct ramfs_sym_data {
	size_t len; /* Same as in uk_fs_path; equal to `strlen(.path)` */
	char path[]; /* Includes terminating NUL */
};

/* Stat fields, common to all nodes */
struct ramfs_stat {
	uint32_t stx_nlink;
	uint32_t stx_uid;
	uint32_t stx_gid;
	uint16_t stx_mode;
	uint64_t stx_size;
	struct uk_statx_timestamp stx_atime, stx_btime, stx_ctime, stx_mtime;
};

/* RAMfs filesystem node */
struct ramfs_node {
	struct uk_swrefcount refcnt;
	struct uk_file_state fstate;
	struct ramfs_stat statx;
	union {
		struct ramfs_dir_data dir_data;
		struct ramfs_file_data file_data;
		struct ramfs_sym_data sym_data;
	};
};

/**
 * Return the allocation size in bytes of a ramfs node with `xlen` bytes of
 * extended data (e.g., target path for symlinks).
 */
#define RAMFS_NODESIZE(xlen) \
	MAX(sizeof(struct ramfs_node), \
	    __offsetof(struct ramfs_node, sym_data.path) + (xlen))

static
struct ramfs_node *ramfs_new(size_t xlen)
{
	struct ramfs_node *n = ramfs_obj_alloc(RAMFS_NODESIZE(xlen));

	if (n) {
		n->fstate = UK_FILE_STATE_EVENTS_INIT_VALUE(n->fstate,
							    UKFD_POLLIN |
							    UKFD_POLLOUT);
		uk_swrefcount_init(&n->refcnt, 1, 1);
	}
	return n;
}

static inline
struct uk_statx_timestamp ramfs_now(void)
{
	int r __maybe_unused;
	struct timespec ts;

	r = uk_sys_clock_gettime(CLOCK_REALTIME, &ts);
	UK_ASSERT(!r);
	return (struct uk_statx_timestamp){
		.tv_sec = ts.tv_sec,
		.tv_nsec = ts.tv_nsec
	};
}

static
void ramfs_init_statx(struct ramfs_node *n, unsigned int mode)
{
	const struct uk_statx_timestamp now = ramfs_now();

	n->statx = (struct ramfs_stat){
		.stx_nlink = 0,
		.stx_uid = 0, /* TODO: use euid */
		.stx_gid = 0, /* TODO use egid */
		.stx_mode = mode,
		.stx_size = 0,
		/* Timestamps */
		.stx_atime = now,
		.stx_btime = now,
		.stx_ctime = now,
		.stx_mtime = now,
	};
}

/**
 * Return file mode `m` with type bits replaced by those in `f`.
 */
#define RAMFS_FMODE(m, f) (((m) & ~S_IFMT) | ((f) & S_IFMT))

static
void ramfs_init_dir(struct ramfs_node *n, unsigned int mode)
{
	ramfs_init_statx(n, RAMFS_FMODE(mode, S_IFDIR));
	/* Init dir_data */
	UK_RB_INIT(&n->dir_data.children);
	n->dir_data.parent = NULL;
	uk_spin_init(&n->dir_data.plock);
}

static
void ramfs_init_file(struct ramfs_node *n, unsigned int mode)
{
	ramfs_init_statx(n, RAMFS_FMODE(mode, S_IFREG));
	ramfs_init_file_data(&n->file_data);
}

static
void ramfs_init_sym(struct ramfs_node *n, unsigned int mode,
		    const char *path, size_t len)
{
	ramfs_init_statx(n, RAMFS_FMODE(mode, S_IFLNK));
	/* Init sym_data */
	n->sym_data.len = len;
	memcpy(n->sym_data.path, path, len);
	n->sym_data.path[len] = '\0';
}

static inline
unsigned char ramfs_dentry_type(const struct ramfs_dentry *dp)
{
	switch (dp->type) {
	case RAMFS_DENT_NODE:
		switch (dp->target.node->statx.stx_mode & S_IFMT) {
		case S_IFDIR:
			return DT_DIR;
		case S_IFREG:
			return DT_REG;
		case S_IFLNK:
			return DT_LNK;
		default:
			return DT_UNKNOWN;
		}
	case RAMFS_DENT_SPEC:
		switch (dp->target.special.mode & S_IFMT) {
		case S_IFBLK:
			return DT_BLK;
		case S_IFCHR:
			return DT_CHR;
		case S_IFIFO:
			return DT_FIFO;
		case S_IFSOCK:
			return DT_SOCK;
		default:
			return DT_UNKNOWN;
		}
	case RAMFS_DENT_FILE:
		return DT_UNKNOWN;
	default:
		UK_BUG();
	}
}

static inline
ino_t ramfs_dentry_inode(const struct ramfs_dentry *dp)
{
	switch (dp->type) {
	case RAMFS_DENT_FILE:
		return (ino_t)(uintptr_t)dp->target.file;
	case RAMFS_DENT_NODE:
		return (ino_t)(uintptr_t)dp->target.node;
	case RAMFS_DENT_SPEC:
		return (ino_t)(uintptr_t)dp;
	default:
		UK_BUG();
	}
}

/*
 * Liveref Operations
 */

/* Glue ops & utils */

static
int ramfs_live_nodekind(const struct ramfs_node *n,
			enum uk_fs_tmpl_node_kind kind)
{
	const int desired = (kind == UK_FS_TMPL_DIR) ? S_IFDIR :
			    (kind == UK_FS_TMPL_SYM) ? S_IFLNK : 0;

	return (n->statx.stx_mode & S_IFMT) == desired;
}

static
struct uk_file_state *ramfs_live_state(struct ramfs_node *n)
{
	return &n->fstate;
}

static
int ramfs_live_errnode(const struct ramfs_node *n)
{
	return PTRISERR(n) ? PTR2ERR(n) : 0;
}

static
void ramfs_live_acquire(struct ramfs_node *n)
{
	uk_swrefcount_acquire(&n->refcnt);
}

static
void ramfs_node_acquire_weak(struct ramfs_node *n)
{
	uk_swrefcount_acquire_weak(&n->refcnt);
}

static
int ramfs_node_try_acquire(struct ramfs_node *n)
{
	return uk_swrefcount_try_acquire(&n->refcnt);
}

/* Non-zero if mount flags `mf` signify a readonly mount */
#define ISROFS(mf) (!!((mf) & MS_RDONLY))

/**
 * Return non-zero if filesystem node `n`, accessed though a mount with flags
 * `mntflags` needs its access time (atime) updated by an operation that would
 * normally do so.
 *
 * @param n Filesystem node
 * @param mntflags Mount flags of the originating filesystem instance
 *
 * @return
 *   != 0: Access time needs updating
 *   == 0: Access time does not need updating
 */
static inline
int ramfs_node_atime_need_update(const struct ramfs_node *n,
				 unsigned long mntflags)
{
	if (ISROFS(mntflags))
		return 0;
	if (mntflags & MS_STRICTATIME)
		return 1;
	if (mntflags & MS_NOATIME ||
	    ((mntflags & MS_NODIRATIME) &&
	     ramfs_live_nodekind(n, UK_FS_TMPL_DIR)))
		return 0;
	if ((mntflags & MS_RELATIME) &&
	    !(n->statx.stx_atime.tv_sec < n->statx.stx_mtime.tv_sec ||
	      (n->statx.stx_atime.tv_sec == n->statx.stx_mtime.tv_sec &&
	       n->statx.stx_atime.tv_nsec < n->statx.stx_mtime.tv_nsec)))
		return 0;
	return 1;
}

enum ramfs_touch_op {
	RAMFS_TOUCH_ACCESS,
	RAMFS_TOUCH_STATUS,
	RAMFS_TOUCH_MODIFY
};

static
void ramfs_node_touch(struct ramfs_node *n, enum ramfs_touch_op op,
		      unsigned long mntflags)
{
	const int mtime = (op == RAMFS_TOUCH_MODIFY);
	const int ctime = (op == RAMFS_TOUCH_STATUS ||
			   op == RAMFS_TOUCH_MODIFY);
	const int atime = (op == RAMFS_TOUCH_ACCESS &&
			   ramfs_node_atime_need_update(n, mntflags));

	if (ctime || mtime || atime) {
		const struct uk_statx_timestamp now = ramfs_now();

		if (ctime)
			n->statx.stx_ctime = now;
		if (mtime)
			n->statx.stx_mtime = now;
		if (atime)
			n->statx.stx_atime = now;
	}
}

static void ramfs_live_release(struct ramfs_node *n);
static void ramfs_node_release_weak(struct ramfs_node *n);

static
void ramfs_node_relink(struct ramfs_node *n, struct ramfs_node *parent)
{
	if (ramfs_live_nodekind(n, UK_FS_TMPL_DIR)) {
		/* RAMfs does not support multiple hardlinks to a dir.
		 * A dir may be simultaneously linked by multiple dentries for a
		 * short time (e.g., during a rename) only if all but the latest
		 * created link is guaranteed to be removed.
		 */
		struct ramfs_node *prev;

		ramfs_node_acquire_weak(parent);
		uk_spin_lock(&n->dir_data.plock);
		prev = uk_exchange_n(&n->dir_data.parent, parent);
		uk_spin_unlock(&n->dir_data.plock);
		if (prev)
			ramfs_node_release_weak(prev);
	}
}

static
void ramfs_node_newlink(struct ramfs_node *n, struct ramfs_node *parent)
{
	uk_inc(&n->statx.stx_nlink);
	ramfs_node_relink(n, parent);
}

static
void ramfs_node_unlink(struct ramfs_node *n)
{
	if (ramfs_live_nodekind(n, UK_FS_TMPL_DIR)) {
		/* RAMfs does not support multiple dir hardlinks. See above. */
		struct ramfs_node *parent;

		uk_spin_lock(&n->dir_data.plock);
		parent = uk_exchange_n(&n->dir_data.parent, NULL);
		uk_spin_unlock(&n->dir_data.plock);
		UK_ASSERT(parent); /* RAMfs root node cannot be unlinked */
		ramfs_node_release_weak(parent);
	}
	uk_dec(&n->statx.stx_nlink);
}

static
void ramfs_dentry_release_target(struct ramfs_dentry *dp)
{
	switch (dp->type) {
	case RAMFS_DENT_NODE:
		ramfs_node_unlink(dp->target.node);
		ramfs_live_release(dp->target.node);
		break;
	case RAMFS_DENT_FILE:
		uk_file_release(dp->target.file);
		break;
	default:
		/* No-op */
		break;
	}
}

static
void ramfs_node_free(struct ramfs_node *n)
{
	switch (n->statx.stx_mode & S_IFMT) {
	case S_IFDIR:
	{
		struct ramfs_dentry *dp;
		struct ramfs_dentry *tmp;

		UK_ASSERT(!n->dir_data.parent);
		UK_RB_FOREACH_SAFE(dp, ramfs_name_map,
				   &n->dir_data.children, tmp) {
			ramfs_dentry_release_target(dp);
			ramfs_obj_free(dp);
		}
		break;
	}
	case S_IFREG:
		ramfs_file_data_free(&n->file_data);
		break;
	default:
		break;
	}
}

static
void ramfs_live_release(struct ramfs_node *n)
{
	int r;

	r = uk_swrefcount_release(&n->refcnt);
	if (r & UK_SWREFCOUNT_LAST_STRONG) {
		ramfs_node_free(n);
		if (r & UK_SWREFCOUNT_LAST_REF)
			ramfs_obj_free(n);
	}
}

static
void ramfs_node_release_weak(struct ramfs_node *n)
{
	if (uk_swrefcount_release_weak(&n->refcnt))
		ramfs_obj_free(n);
}

/* Driver ops */

UK_FS_TMPL_LIVE_TYPES(ramfs, struct ramfs_node *);
UK_FS_TMPL_LIVE_OPS(ramfs, struct ramfs_node *);

static
struct ramfs_node *ramfs_live_vopen(const void *vol __unused,
				    unsigned long flags __unused,
				    const void *data __unused)
{
	struct ramfs_node *rootnode;
	int r;

	/* RAMfs files use the system PoD; ensure it's inited */
	r = uk_pod_default_init();
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return ERR2PTR(r);

	rootnode = ramfs_new(0);
	if (unlikely(!rootnode))
		return ERR2PTR(-ENOMEM);
	ramfs_init_dir(rootnode, 0777);
	return rootnode;
}

/* File ops */

static inline
int ramfs_check_regfile(const struct ramfs_node *n)
{
	if (ramfs_live_nodekind(n, UK_FS_TMPL_DIR))
		return -EISDIR;
	if (ramfs_live_nodekind(n, UK_FS_TMPL_SYM))
		return -EBADF;
	return 0;
}

static
ssize_t ramfs_live_read(struct ramfs_node *n, const struct iovec *iov,
			size_t iovcnt, size_t off, long flags,
			unsigned long mntflags)
{
	const size_t fsz = n->statx.stx_size;
	const size_t rem = fsz > off ? fsz - off : 0;
	const int r = ramfs_check_regfile(n);
	size_t ret;

	if (unlikely(r))
		return r;
	if (unlikely(!rem))
		return 0;

	ret = ramfs_file_data_read(&n->file_data, iov, iovcnt, off, rem);
	if (ret) {
		if (flags & O_NOATIME)
			mntflags |= MS_NOATIME;
		ramfs_node_touch(n, RAMFS_TOUCH_ACCESS, mntflags);
	}
	return ret;
}

static
ssize_t ramfs_live_write(struct ramfs_node *n, const struct iovec *iov,
			 size_t iovcnt, size_t off, long flags,
			 unsigned long mntflags)
{
	const int r = ramfs_check_regfile(n);
	size_t rem = uk_iov_len(iov, iovcnt);
	size_t written;

	if (unlikely(ISROFS(mntflags)))
		return -EROFS;
	if (unlikely(r))
		return r;
	if (unlikely(!rem))
		return 0;

	written = ramfs_file_data_write(&n->file_data, iov, iovcnt, off, rem);
	if (unlikely(!written))
		return -ENOSPC;
	/* Increase file size if needed */
	if (off + written > n->statx.stx_size)
		n->statx.stx_size = off + written;
	/* Touch & ret */
	if (flags & O_NOATIME)
		mntflags |= MS_NOATIME;
	ramfs_node_touch(n, RAMFS_TOUCH_MODIFY, mntflags);
	return written;
}

static
ssize_t ramfs_live_mem(struct ramfs_node *n, enum uk_file_mem_op op,
		       size_t off, size_t len, struct iovec *iov,
		       size_t iovcnt, unsigned long mntflags)
{
	int r;
	size_t total;

	r = ramfs_check_regfile(n);
	if (unlikely(r))
		return r;

	/* Validate args */
	switch (op) {
	case UKFILE_MEM_RETRIEVE:
	case UKFILE_MEM_BORROW:
		if (unlikely(!iovcnt))
			return 0;
		__fallthrough;
	case UKFILE_MEM_ACQUIRE:
	case UKFILE_MEM_RELEASE:
	case UKFILE_MEM_BUFCOUNT:
		if (unlikely(off >= n->statx.stx_size))
			return 0;
		if (off + len > n->statx.stx_size)
			len = n->statx.stx_size - off;
		break;
	case UKFILE_MEM_GIFT:
		if (unlikely(ISROFS(mntflags)))
			return -EROFS;
		if (unlikely(!IS_ALIGNED(off, PAGE_SIZE) ||
			     !IS_ALIGNED(len, PAGE_SIZE)))
			return -EINVAL;

		total = 0;
		for (size_t i = 0; i < iovcnt; i++) {
			if (unlikely(!IS_ALIGNED((uintptr_t)iov[i].iov_base,
						 PAGE_SIZE) ||
				     !IS_ALIGNED(iov[i].iov_len, PAGE_SIZE)))
				return -EINVAL;
			total += iov[i].iov_len;
		}
		if (unlikely(total != len))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	if (unlikely(!len))
		return 0;
	/* Do op */
	switch (op) {
	case UKFILE_MEM_BUFCOUNT:
		return ramfs_file_range_bufcount(&n->file_data, off, len);
	case UKFILE_MEM_ACQUIRE:
		return ramfs_file_range_acquire(&n->file_data, off, len);
	case UKFILE_MEM_RETRIEVE:
	case UKFILE_MEM_BORROW:
		return ramfs_file_range_get(&n->file_data, off, len,
					    iov, iovcnt);
	case UKFILE_MEM_RELEASE:
		ramfs_file_range_release(&n->file_data, off, len);
		return 0;
	case UKFILE_MEM_GIFT:
		r = ramfs_file_range_gift(&n->file_data, off / PAGE_SIZE,
					  iov, iovcnt);
		if (r > 0 && off + len > n->statx.stx_size)
			n->statx.stx_size = off + len;
		return r;
	default:
		UK_BUG(); /* Validate op beforehand */
	}
}

static
int ramfs_live_getstat(struct ramfs_node *n,
		       unsigned int mask __unused, struct uk_statx *arg,
		       unsigned long mntflags __unused)
{
	/* Everything is immediately available; ignore mask */
	/* Const/computed values */
	arg->stx_mask = UK_STATX_TYPE | UK_STATX_MODE | UK_STATX_NLINK |
			UK_STATX_UID | UK_STATX_GID | UK_STATX_ATIME |
			UK_STATX_BTIME | UK_STATX_CTIME | UK_STATX_MTIME |
			UK_STATX_INO | UK_STATX_SIZE | UK_STATX_DIOALIGN;
	arg->stx_blksize = PAGE_SIZE;
	arg->stx_ino = (uintptr_t)n;
	arg->stx_dio_mem_align = PAGE_SIZE;
	arg->stx_dio_offset_align = PAGE_SIZE;
	/* Readout values */
	arg->stx_nlink = n->statx.stx_nlink;
	arg->stx_uid = n->statx.stx_uid;
	arg->stx_gid = n->statx.stx_gid;
	arg->stx_mode = n->statx.stx_mode;
	arg->stx_size = n->statx.stx_size;
	arg->stx_atime = n->statx.stx_atime;
	arg->stx_btime = n->statx.stx_btime;
	arg->stx_ctime = n->statx.stx_ctime;
	arg->stx_mtime = n->statx.stx_mtime;
	return 0;
}

#define RAMFS_SETSTAT_MASK \
	(UK_STATX_MODE | UK_STATX_UID | UK_STATX_GID | \
	 UK_STATX_ATIME | UK_STATX_BTIME | UK_STATX_CTIME | UK_STATX_MTIME)

static
int ramfs_live_setstat(struct ramfs_node *n,
		       unsigned int mask, const struct uk_statx *arg,
		       unsigned long mntflags)
{
	if (unlikely(ISROFS(mntflags)))
		return -EROFS;
	if (unlikely(mask & ~RAMFS_SETSTAT_MASK))
		return -EINVAL;
	if (mask) {
		if (mask & UK_STATX_MODE)
			n->statx.stx_mode = (n->statx.stx_mode & ~0777) |
					    (arg->stx_mode & 0777);
		if (mask & UK_STATX_UID)
			n->statx.stx_uid = arg->stx_uid;
		if (mask & UK_STATX_GID)
			n->statx.stx_gid = arg->stx_gid;
		if (mask & UK_STATX_ATIME)
			n->statx.stx_atime = arg->stx_atime;
		if (mask & UK_STATX_MTIME)
			n->statx.stx_mtime = arg->stx_mtime;
		ramfs_node_touch(n, RAMFS_TOUCH_STATUS, mntflags);
	}
	return 0;
}

static
int ramfs_node_trunc(struct ramfs_node *n, off_t newsz, unsigned long mntflags)
{
	int r;

	if (unlikely(ISROFS(mntflags)))
		return -EROFS;
	if (unlikely(newsz < 0))
		return -EINVAL;
	r = ramfs_check_regfile(n);
	if (unlikely(r))
		return r;

	if ((size_t)newsz < n->statx.stx_size)
		r = ramfs_file_data_trunc(&n->file_data, newsz);
	if (!r) {
		n->statx.stx_size = newsz;
		ramfs_node_touch(n, RAMFS_TOUCH_MODIFY, mntflags);
	}
	return r;
}

static
int ramfs_node_falloc(struct ramfs_node *n, int mode, off_t soff, off_t slen,
		      unsigned long mntflags)
{
	size_t off;
	size_t len;
	int r;

	if (unlikely(ISROFS(mntflags)))
		return -EROFS;
	if (unlikely(soff < 0 || slen <= 0))
		return -EINVAL;
	r = ramfs_check_regfile(n);
	if (unlikely(r))
		return r;

	/* Validated soff & slen are positive, can assign to unsigned size_t */
	off = soff;
	len = slen;
	switch (mode) {
	case 0:
	case FALLOC_FL_KEEP_SIZE:
		return -EOPNOTSUPP;

	case FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE:
		r = ramfs_file_data_punch_hole(&n->file_data, off, len);
		break;

	case FALLOC_FL_COLLAPSE_RANGE:
		if (unlikely(!IS_ALIGNED(off, PAGE_SIZE) ||
			     !IS_ALIGNED(len, PAGE_SIZE)))
			return -EINVAL;
		if (unlikely(off + len >= n->statx.stx_size))
			return -EINVAL;
		if (unlikely(!len))
			return 0;
		r = ramfs_file_data_collapse(&n->file_data, off / PAGE_SIZE,
					     len / PAGE_SIZE);
		if (!r)
			n->statx.stx_size -= len;
		break;

	case FALLOC_FL_INSERT_RANGE:
		if (unlikely(!IS_ALIGNED(off, PAGE_SIZE) ||
			     !IS_ALIGNED(len, PAGE_SIZE)))
			return -EINVAL;
		if (unlikely(off >= n->statx.stx_size))
			return -EINVAL;
		if (unlikely(!len))
			return 0;
		r = ramfs_file_data_insert_hole(&n->file_data, off / PAGE_SIZE,
						len / PAGE_SIZE);
		if (!r)
			n->statx.stx_size += len;
		break;

	case FALLOC_FL_ZERO_RANGE:
	case FALLOC_FL_ZERO_RANGE | FALLOC_FL_KEEP_SIZE:
		return -EOPNOTSUPP;
	default:
		return -EINVAL;
	}
	if (!r)
		ramfs_node_touch(n, RAMFS_TOUCH_MODIFY, mntflags);
	return r;
}

static
int ramfs_live_ctl(struct ramfs_node *n, int fam, int req,
		   uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
		   unsigned long mntflags)
{
	switch (fam) {
	case UKFILE_CTL_FILE:
		switch (req) {
		case UKFILE_CTL_FILE_SYNC:
		case UKFILE_CTL_FILE_FADVISE:
			return 0;
		case UKFILE_CTL_FILE_TRUNC:
			return ramfs_node_trunc(n, (off_t)arg1, mntflags);
		case UKFILE_CTL_FILE_FALLOC:
			return ramfs_node_falloc(n, (int)arg1, (off_t)arg2,
						 (off_t)arg3, mntflags);
		default:
			return -ENOSYS;
		}
		break;
	case UKFILE_CTL_IOCTL:
		/* No ioctls supported */
		return -EINVAL;
	default:
		return -ENOSYS;
	}
}

/* FS ops */

/* Same as Linux's tmpfs out of paranoia; change to Unikraft-specific if ok */
#define RAMFS_MAGIC 0x01021994

/* TODO: make dynamic if needed */
static const struct statfs ramfs_fsstat = {
	.f_type = RAMFS_MAGIC,
	.f_bsize = PAGE_SIZE,
	.f_blocks = 0,
	.f_bfree = 0,
	.f_bavail = 0,
	.f_files = 0,
	.f_ffree = 0,
	.f_fsid = {{ 0, 0 }},
	.f_namelen = USHRT_MAX - 1,
	.f_frsize = PAGE_SIZE,
	.f_flags = 0,
};

static
int ramfs_live_fs_stat(struct ramfs_node *n __unused, struct statfs *buf)
{
	*buf = ramfs_fsstat;
	return 0;
}

static
int ramfs_live_fs_sync(struct ramfs_node *n __unused)
{
	/* Nothing to do, ramfs is always synced */
	return 0;
}

static
int ramfs_dir_isempty(struct ramfs_node *n)
{
	int r;

	UK_ASSERT(ramfs_live_nodekind(n, UK_FS_TMPL_DIR));
	uk_file_state_rlock(ramfs_live_state(n));
	r = UK_RB_EMPTY(&n->dir_data.children);
	uk_file_state_runlock(ramfs_live_state(n));
	return r;
}

static
struct ramfs_dentry *ramfs_dir_find(struct ramfs_node *n,
				    const char *name, size_t len)
{
	UK_ASSERT(ramfs_live_nodekind(n, UK_FS_TMPL_DIR));
	return UK_RB_FIND(ramfs_name_map, &n->dir_data.children,
		((struct ramfs_entry_name){ .len = len, .name = name }));
}

static
void ramfs_dir_insert(struct ramfs_node *n, struct ramfs_dentry *dp)
{
	struct ramfs_dentry *prev __maybe_unused;

	UK_ASSERT(ramfs_live_nodekind(n, UK_FS_TMPL_DIR));
	prev = UK_RB_INSERT(ramfs_name_map, &n->dir_data.children, dp);
	UK_ASSERT(!prev);
}

static
void ramfs_dir_remove(struct ramfs_node *n, struct ramfs_dentry *dp)
{
	UK_ASSERT(ramfs_live_nodekind(n, UK_FS_TMPL_DIR));
	UK_RB_REMOVE(ramfs_name_map, &n->dir_data.children, dp);
}

static
int ramfs_dir_lookup(struct ramfs_node *n, const char *name, size_t len,
		     struct ramfs_node **out_nod,
		     union uk_fs_lookup_out *out_ukfs)
{
	struct ramfs_dentry *dp;
	int r;

	r = uk_fs_checkperm(X_OK, n->statx.stx_mode & 0777,
			    n->statx.stx_uid, n->statx.stx_gid);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;

	/* Check for "." & ".." */
	if (len == 1 && name[0] == '.') {
		ramfs_live_acquire(n);
		out_nod[0] = n;
		return UKFS_STOP_NOD;
	}
	if (len == 2 && name[0] == '.' && name[1] == '.') {
		struct ramfs_node *parent;

		uk_spin_lock(&n->dir_data.plock);
		parent = n->dir_data.parent;
		/* Try to acquire a strong ref on parent; if fails, parent is
		 * being cleaned up and will unlink us soon; report self orphan.
		 */
		if (parent && !ramfs_node_try_acquire(parent))
			parent = NULL;
		uk_spin_unlock(&n->dir_data.plock);

		/* Orphans are their own parents */
		if (!parent) {
			ramfs_live_acquire(n);
			parent = n;
		}
		out_nod[0] = parent;
		return UKFS_STOP_NOD;
	}
	/* Lookup actual dir contents */
	dp = ramfs_dir_find(n, name, len);
	if (unlikely(!dp))
		return -ENOENT;

	switch (dp->type) {
	case RAMFS_DENT_NODE:
		ramfs_live_acquire(dp->target.node);
		out_nod[0] = dp->target.node;
		if (ramfs_live_nodekind(dp->target.node, UK_FS_TMPL_SYM)) {
			ramfs_live_acquire(n);
			out_nod[1] = n;
			return UKFS_STOP_SYM;
		}
		return UKFS_STOP_NOD;
	case RAMFS_DENT_FILE:
		uk_file_acquire(dp->target.file);
		out_ukfs->target = dp->target.file;
		return UKFS_STOP_FILE;
	case RAMFS_DENT_SPEC:
		out_ukfs->special = dp->target.special;
		return UKFS_STOP_SPEC;
	default:
		UK_BUG();
	}
}

/* TODO: optimize to not exit into live template loop on every node */
static
int ramfs_live_fs_lookup(struct ramfs_node *n, const char *path, size_t len,
			 struct ramfs_node **out_nod,
			 union uk_fs_lookup_out *out_ukfs, size_t *nout)
{
	size_t cur = uk_fs_path_sep(path, len);
	int ret;

	/* Non-directories are handled by the live template */
	UK_ASSERT(ramfs_live_nodekind(n, UK_FS_TMPL_DIR));
	ret = ramfs_dir_lookup(n, path, cur, out_nod, out_ukfs);
	if (unlikely(ret < 0))
		return ret;
	if (cur < len) {
		UK_ASSERT(path[cur] == '/');
		cur++;
	}
	*nout = cur;
	return ret;
}

/* Dir ops */

static
ssize_t ramfs_live_fs_listdir(struct ramfs_node *n, size_t *curp,
			      void *buf, size_t len, unsigned long mntflags)
{
	struct uk_fs_dirent *out = buf;
	size_t p = *curp;
	size_t rlen = 0;
	int full = 0;

	/* Entries for "." & ".." */
	while (p < 2) {
		size_t sz = UKFS_DIRENT_RECLEN(p ? 2 : 1);

		if (unlikely(sz > len)) {
			full = 1;
			break;
		}
		if (p == 0) { /* 0: "." */
			UKFS_DIRENT_OUT_DOT(out, (ino_t)(uintptr_t)n);
		} else { /* 1: ".." */
			/* We only need ptr value, not a counted reference;
			 * It's thus safe to skip locks and just read value.
			 */
			struct ramfs_node *parent = n->dir_data.parent;

			UKFS_DIRENT_OUT_DOTDOT(out,
				(ino_t)(uintptr_t)(parent ? parent : n));
		}
		rlen += sz;
		len -= sz;
		out = UKFS_DIRENT_NEXT(out, sz);
		p++;
	}

	/* Child entries */
	if (!full) {
		/* First child is entry number 2 (after "." & "..") */
		size_t entry = 2;
		struct ramfs_dentry *dp;

		UK_RB_FOREACH(dp, ramfs_name_map, &n->dir_data.children) {
			size_t sz;

			/* Skip until p */
			if (entry++ < p)
				continue;

			sz = UKFS_DIRENT_RECLEN(dp->namelen);
			if (unlikely(sz > len)) {
				full = 1;
				break;
			}
			/* Writeout */
			out->d_ino = ramfs_dentry_inode(dp);
			out->d_reclen = sz;
			out->d_type = ramfs_dentry_type(dp);
			memcpy(out->d_name, dp->name, dp->namelen);
			out->d_name[dp->namelen] = '\0';

			rlen += sz;
			len -= sz;
			out = UKFS_DIRENT_NEXT(out, sz);
			p++;
		}
	}
	if (rlen) {
		ramfs_node_touch(n, RAMFS_TOUCH_ACCESS, mntflags);
		*curp = p;
	}
	return (rlen || !full) ? (ssize_t)rlen : -EINVAL;
}

static
struct ramfs_node *ramfs_node_create(unsigned int mode,
	union UK_FS_TMPL_LIVE_CREATE_TARGET(ramfs) target)
{
	const unsigned int type = mode & S_IFMT;
	size_t xlen = 0;
	struct ramfs_node *newnode;

	/* new node */
	switch (type) {
	case S_IFLNK:
		xlen = strlen(target.ukfs.path) + 1;
		__fallthrough;
	case S_IFDIR:
	case S_IFREG:
		newnode = ramfs_new(xlen);
		if (unlikely(!newnode))
			return ERR2PTR(-ENOMEM);
		break;
	case S_IFMT:
		newnode = target.livenode;
		/* Refuse multiple hardlinks to directories */
		if (unlikely(ramfs_live_nodekind(newnode, UK_FS_TMPL_DIR) &&
			     newnode->dir_data.parent))
			return ERR2PTR(-EPERM);
		ramfs_live_acquire(newnode);
		break;
	default:
		return ERR2PTR(-EINVAL);
	}
	/* init new node */
	switch (type) {
	case S_IFLNK:
		ramfs_init_sym(newnode, mode, target.ukfs.path, xlen - 1);
		break;
	case S_IFDIR:
		ramfs_init_dir(newnode, mode);
		break;
	case S_IFREG:
		ramfs_init_file(newnode, mode);
		break;
	default:
		/* No new node to init; nop */
		break;
	}
	return newnode;
}

union ramfs_link {
	struct ramfs_node *node;
	const struct uk_file *file;
	const struct uk_fs_specfile *special;
};

static
int ramfs_linkin(struct ramfs_node *n, const char *name, size_t namelen,
		 unsigned char dtype, union ramfs_link target, int flags)
{
	/* Lookup name dentry */
	struct ramfs_dentry *dp = ramfs_dir_find(n, name, namelen);

	if (unlikely(dp && (flags & O_EXCL)))
		return -EEXIST;

	if (!dp) {
		dp = ramfs_dentry_new(name, namelen);
		if (unlikely(!dp))
			return -ENOMEM;
		ramfs_dir_insert(n, dp);
	} else {
		/* Reuse dentry; release previous target */
		ramfs_dentry_release_target(dp);
	}

	dp->type = dtype;
	switch (dtype) {
	case RAMFS_DENT_NODE:
		dp->target.node = target.node;
		ramfs_live_acquire(target.node);
		ramfs_node_newlink(target.node, n);
		break;
	case RAMFS_DENT_FILE:
		dp->target.file = target.file;
		uk_file_acquire(target.file);
		break;
	case RAMFS_DENT_SPEC:
		dp->target.special = *target.special;
		break;
	default:
		UK_BUG();
	}
	return 0;
}

static
struct ramfs_node *ramfs_live_fs_create(struct ramfs_node *n,
	const char *name, size_t namelen, unsigned int mode, int flags,
	union UK_FS_TMPL_LIVE_CREATE_TARGET(ramfs) target,
	unsigned long mntflags)
{
	union ramfs_link newlink;
	unsigned char dtype;
	int err;

	/* refuse to operate on "." and ".." */
	if (unlikely(!(flags & O_TMPFILE) &&
		     (uk_fs_path_isdot(name, namelen) ||
		      uk_fs_path_isdotdot(name, namelen))))
		return ERR2PTR(-EEXIST);
	if (unlikely(!(flags & O_TMPFILE) && namelen > RAMFS_MAX_NAMELEN))
		return ERR2PTR(-ENAMETOOLONG);

	switch (mode & S_IFMT) {
	case S_IFIFO:
	case S_IFSOCK:
	case S_IFCHR:
	case S_IFBLK:
		dtype = RAMFS_DENT_SPEC;
		newlink.special = target.ukfs.special;
		err = 0;
		break;
	case 0:
		dtype = RAMFS_DENT_FILE;
		newlink.file = target.ukfs.file;
		err = 0;
		break;
	default:
		dtype = RAMFS_DENT_NODE;
		newlink.node = ramfs_node_create(mode, target);
		err = PTRISERR(newlink.node) ? PTR2ERR(newlink.node) : 0;
		break;
	}
	if (unlikely(err))
		return ERR2PTR(err);

	/* Link in if not tmpfile */
	if (!(flags & O_TMPFILE)) {
		err = ramfs_linkin(n, name, namelen, dtype, newlink, flags);
		if (unlikely(err)) {
			if (dtype == RAMFS_DENT_NODE)
				ramfs_live_release(newlink.node);
			return ERR2PTR(err);
		}
		ramfs_node_touch(n, RAMFS_TOUCH_MODIFY, mntflags);
	}
	/* Success */
	switch (dtype) {
	case RAMFS_DENT_SPEC:
	case RAMFS_DENT_FILE:
		return NULL;
	case RAMFS_DENT_NODE:
		return newlink.node;
	default:
		UK_BUG();
	}
}

static
int ramfs_live_fs_unlink(struct ramfs_node *n, const char *name, size_t len,
			 unsigned int flags, unsigned long mntflags)
{
	struct ramfs_dentry *dp;

	/* refuse to operate on '.' and '..' */
	if (unlikely(uk_fs_path_isdot(name, len) ||
		     uk_fs_path_isdotdot(name, len)))
		return -EINVAL;

	dp = ramfs_dir_find(n, name, len);
	if (unlikely(!dp))
		return -ENOENT;
	if (ramfs_dentry_type(dp) == DT_DIR) {
		if (unlikely(flags & UKFS_UNLINK_NODIR))
			return -EISDIR;
		if (unlikely((flags & UKFS_UNLINK_EMPTY) &&
			     !ramfs_dir_isempty(dp->target.node)))
			return -ENOTEMPTY;
	} else {
		if (unlikely(flags & UKFS_UNLINK_DIR))
			return -ENOTDIR;
	}

	ramfs_dentry_release_target(dp);
	ramfs_dir_remove(n, dp);
	ramfs_obj_free(dp);
	ramfs_node_touch(n, RAMFS_TOUCH_MODIFY, mntflags);
	return 0;
}

/* TODO: import these into nolibc */
#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE 1
#endif /* RENAME_NOREPLACE */

#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE 2
#endif /* RENAME_EXCHANGE */

#ifndef RENAME_WHITEOUT
#define RENAME_WHITEOUT 4
#endif /* RENAME_WHITEOUT */

static inline
int ramfs_dentry_same_file(const struct ramfs_dentry *da,
			   const struct ramfs_dentry *db)
{
	if (da->type != db->type)
		return 0;

	switch (da->type) {
	case RAMFS_DENT_NODE:
		return da->target.node == db->target.node;
	case RAMFS_DENT_FILE:
		return da->target.file == db->target.file;
	default:
		return 0;
	}
}

static inline
void ramfs_dentry_xchg(struct ramfs_dentry *da, struct ramfs_dentry *db,
		       struct ramfs_node *pa, struct ramfs_node *pb)
{
	unsigned char dtype;
	union ramfs_dentry_target tmp;

	dtype = da->type;
	tmp = da->target;
	da->type = db->type;
	da->target = db->target;
	db->type = dtype;
	db->target = tmp;
	if (da->type == RAMFS_DENT_NODE)
		ramfs_node_relink(da->target.node, pa);
	if (db->type == RAMFS_DENT_NODE)
		ramfs_node_relink(db->target.node, pb);
}

static inline
void ramfs_dentry_move(struct ramfs_dentry *sd, struct ramfs_dentry *dd,
		       struct ramfs_node *src, struct ramfs_node *dest)
{
	ramfs_dentry_release_target(dd);
	dd->type = sd->type;
	dd->target = sd->target;
	if (dd->type == RAMFS_DENT_NODE)
		ramfs_node_relink(dd->target.node, dest);
	ramfs_dir_remove(src, sd);
	ramfs_obj_free(sd);
}

static inline
void ramfs_dentry_rename(struct ramfs_dentry *dp, const char *name, size_t len,
			 struct ramfs_node *src, struct ramfs_node *dest)
{
	ramfs_dir_remove(src, dp);
	if (dp->type == RAMFS_DENT_NODE)
		ramfs_node_relink(dp->target.node, dest);
	memcpy(dp->name, name, len);
	dp->namelen = len;
	ramfs_dir_insert(dest, dp);
}

static inline
int ramfs_dentry_newname(struct ramfs_dentry *sd, const char *name, size_t len,
			 struct ramfs_node *src, struct ramfs_node *dest)
{
	struct ramfs_dentry *dp = ramfs_dentry_new(name, len);

	if (unlikely(!dp))
		return -ENOMEM;

	dp->type = sd->type;
	dp->target = sd->target;
	if (dp->type == RAMFS_DENT_NODE)
		ramfs_node_relink(dp->target.node, dest);
	ramfs_dir_insert(dest, dp);
	ramfs_dir_remove(src, sd);
	ramfs_obj_free(sd);
	return 0;
}

static
int ramfs_live_fs_rename(struct ramfs_node *n,
			 const char *name, size_t nlen,
			 struct ramfs_node *dest,
			 const char *dname, size_t dlen,
			 unsigned int flags,
			 unsigned long mntflags)
{
	struct ramfs_dentry *sd;
	struct ramfs_dentry *dd;
	int ret = 0;

	if (unlikely(flags & RENAME_WHITEOUT))
		return -EINVAL;
	/* refuse to operate on "." and ".." */
	if (unlikely(uk_fs_path_isdot(name, nlen) ||
		     uk_fs_path_isdotdot(name, nlen) ||
		     uk_fs_path_isdot(dname, dlen) ||
		     uk_fs_path_isdotdot(dname, dlen)))
		return -EINVAL;

	sd = ramfs_dir_find(n, name, nlen);
	if (unlikely(!sd))
		return -ENOENT;

	dd = ramfs_dir_find(dest, dname, dlen);
	if (dd) {
		/* Destination exists */
		if (ramfs_dentry_same_file(sd, dd))
			/* Same file; shortcut no-op regardless of args */
			ret = 0;
		else if (flags & RENAME_NOREPLACE)
			/* We cannot replace destination; error */
			ret = -EEXIST;
		else if (flags & RENAME_EXCHANGE)
			/* Unlink & re-link as eachother */
			ramfs_dentry_xchg(sd, dd, n, dest);
		else
			/* Replace dest node; reuse dest dentry */
			ramfs_dentry_move(sd, dd, n, dest);
	} else {
		/* Destination does not exist */
		if (flags & RENAME_EXCHANGE)
			/* Nothing to exchange with; error */
			ret = -ENOENT;
		else if (dlen <= sd->namelen)
			/* Name fits in sd, reuse dentry object */
			ramfs_dentry_rename(sd, dname, dlen, n, dest);
		else
			/* Name too big for sd; new dentry */
			ret = ramfs_dentry_newname(sd, dname, dlen, n, dest);
	}
	if (!ret) {
		ramfs_node_touch(n, RAMFS_TOUCH_MODIFY, mntflags);
		if (dest != n)
			ramfs_node_touch(dest, RAMFS_TOUCH_MODIFY, mntflags);
	}
	return ret;
}

/* Sym ops */

static
struct uk_fs_path ramfs_live_fs_readlink(struct ramfs_node *n)
{
	return (struct uk_fs_path){
		.s = n->sym_data.path,
		.len = n->sym_data.len
	};
}

/* Ops table, driver templating & registration */

UK_FS_TMPL_LIVE_OPSTABLE(ramfs, ramfs_live_ops);

static const struct ramfs_live_ops ramfs_liveops = {
	.live_vopen = ramfs_live_vopen,
	.live_read = ramfs_live_read,
	.live_write = ramfs_live_write,
	.live_mem = ramfs_live_mem,
	.live_getstat = ramfs_live_getstat,
	.live_setstat = ramfs_live_setstat,
	.live_ctl = ramfs_live_ctl,
	.live_fs_stat = ramfs_live_fs_stat,
	.live_fs_sync = ramfs_live_fs_sync,
	.live_fs_lookup = ramfs_live_fs_lookup,
	.live_fs_listdir = ramfs_live_fs_listdir,
	.live_fs_create = ramfs_live_fs_create,
	.live_fs_unlink = ramfs_live_fs_unlink,
	.live_fs_rename = ramfs_live_fs_rename,
	.live_fs_readlink = ramfs_live_fs_readlink,
	.live_nodekind = ramfs_live_nodekind,
	.live_state = ramfs_live_state,
	.live_errnode = ramfs_live_errnode,
	.live_acquire = ramfs_live_acquire,
	.live_release = ramfs_live_release
};

static int ramfs_node_cmp(struct ramfs_node *a, struct ramfs_node *b)
{
	return a - b;
}

UK_FS_TMPL_LIVE_GENERATE_STATIC(ramfs, struct ramfs_node *, ramfs_node_cmp,
				ramfs_liveops, 0);

UK_FS_DRIVER_REGISTER(ramfs, UK_FS_TMPL_LIVE_OP_VOPEN(ramfs));
