/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <fcntl.h>

#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/init.h>
#include <uk/syscall.h>

#include <uk/posix-fdtab.h>

#include "fmap.h"

#if CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM
#include <uk/posix-fdtab-legacy.h>
#endif /* CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */

#define UK_FDTAB_SIZE CONFIG_LIBPOSIX_FDTAB_MAXFDS
UK_CTASSERT(UK_FDTAB_SIZE <= UK_FD_MAX);

/* Static init fdtab */

static char init_bmap[UK_BMAP_SZ(UK_FDTAB_SIZE)];
static void *init_fdmap[UK_FDTAB_SIZE];

struct uk_fdtab {
	struct uk_alloc *alloc;
	struct uk_fmap fmap;
};

static struct uk_fdtab init_fdtab = {
	.fmap = {
		.bmap = {
			.size = UK_FDTAB_SIZE,
			.bitmap = (unsigned long *)init_bmap
		},
		.map = init_fdmap
	}
};

static int init_posix_fdtab(struct uk_init_ctx *ictx __unused)
{
	init_fdtab.alloc = uk_alloc_get_default();
	/* Consider skipping init for .map (static vars are inited to 0) */
	uk_fmap_init(&init_fdtab.fmap);
	return 0;
}

/* Init fdtab as early as possible, to enable functions that rely on fds */
uk_early_initcall_prio(init_posix_fdtab, 0x0, UK_PRIO_EARLIEST);

/* TODO: Adapt when multiple processes are supported */
static inline struct uk_fdtab *_active_tab(void)
{
	return &init_fdtab;
}

/* Encode flags in entry pointer using the least significant bits */
/* made available by the open file structure's alignment */
struct fdval {
	void *p;
	int flags;
};

#define UK_FDTAB_CLOEXEC 1

#if CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM
#define UK_FDTAB_VFSCORE 2
#define _MAX_FLAG 2
#else /* !CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */
#define _MAX_FLAG 1
#endif /* !CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */

#define _FLAG_MASK (((uintptr_t)_MAX_FLAG << 1) - 1)

UK_CTASSERT(__alignof__(struct uk_ofile) > _MAX_FLAG);
#if CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM && CONFIG_LIBVFSCORE
UK_CTASSERT(__alignof__(struct vfscore_file) > _MAX_FLAG);
#endif /* CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM && CONFIG_LIBVFSCORE */

static inline const void *fdtab_encode(const void *f, int flags)
{
	UK_ASSERT(!((uintptr_t)f & _FLAG_MASK));
	return (const void *)((uintptr_t)f | flags);
}

static inline struct fdval fdtab_decode(void *p)
{
	uintptr_t v = (uintptr_t)p;

	return (struct fdval) {
		.p = (void *)(v & ~_FLAG_MASK),
		.flags = v & _FLAG_MASK
	};
}

/* struct uk_ofile allocation & refcounting */
static inline struct uk_ofile *ofile_new(struct uk_fdtab *tab)
{
	struct uk_ofile *of = uk_malloc(tab->alloc, sizeof(*of));

	if (of)
		uk_ofile_init(of);
	return of;
}
static inline void ofile_del(struct uk_fdtab *tab, struct uk_ofile *of)
{
	uk_free(tab->alloc, of);
}

static inline void ofile_acq(struct uk_ofile *of)
{
	uk_refcount_acquire(&of->refcnt);
}
static inline void ofile_rel(struct uk_fdtab *tab, struct uk_ofile *of)
{
	if (uk_refcount_release(&of->refcnt)) {
		uk_file_release(of->file);
		ofile_del(tab, of);
	}
}

#if CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM

static inline void file_acq(void *p, int flags)
{
#if CONFIG_LIBVFSCORE
	if (flags & UK_FDTAB_VFSCORE)
		fhold((struct vfscore_file *)p);
	else
#endif /* CONFIG_LIBVFSCORE */
		ofile_acq((struct uk_ofile *)p);
}
static inline void file_rel(struct uk_fdtab *tab, void *p, int flags)
{
#if CONFIG_LIBVFSCORE
	if (flags & UK_FDTAB_VFSCORE)
		fdrop((struct vfscore_file *)p);
	else
#endif /* CONFIG_LIBVFSCORE */
		ofile_rel(tab, (struct uk_ofile *)p);
}

#else /* !CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */

#define file_acq(p, f) ofile_acq((struct uk_ofile *)(p))
#define file_rel(t, p, f) ofile_rel((t), (struct uk_ofile *)(p))

#endif /* !CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */

/* Ops */

int uk_fdtab_open(const struct uk_file *f, unsigned int mode)
{
	struct uk_fdtab *tab;
	struct uk_ofile *of;
	int flags;
	const void *entry;
	int fd;

	UK_ASSERT(f);

	tab = _active_tab();
	of = ofile_new(tab);
	if (!of)
		return -ENOMEM;
	/* Take refs on file & ofile */
	uk_file_acquire(f);
	ofile_acq(of);
	/* Prepare open file */
	of->file = f;
	of->pos = 0;
	of->mode = mode & ~O_CLOEXEC;
	/* Place the file in fdtab */
	flags = (mode & O_CLOEXEC) ? UK_FDTAB_CLOEXEC : 0;
	entry = fdtab_encode(of, flags);
	fd = uk_fmap_put(&tab->fmap, entry, 0);
	if (fd >= UK_FDTAB_SIZE)
		goto err_out;
	return fd;
err_out:
	/* Release open file & file ref */
	ofile_rel(tab, of);
	return -ENFILE;
}

int uk_fdtab_setflags(int fd, int flags)
{
	struct uk_fdtab *tab;
	struct uk_fmap *fmap;
	void *p;
	struct fdval v;
	const void *newp;

	if (flags & ~O_CLOEXEC)
		return -EINVAL;

	tab = _active_tab();
	fmap = &tab->fmap;

	p = uk_fmap_critical_take(fmap, fd);
	if (!p)
		return -EBADF;
	v = fdtab_decode(p);
	v.flags &= ~UK_FDTAB_CLOEXEC;
	v.flags |= flags ? UK_FDTAB_CLOEXEC : 0;

	newp = fdtab_encode(v.p, v.flags);
	uk_fmap_critical_put(fmap, fd, newp);
	return 0;
}

int uk_fdtab_getflags(int fd)
{
	struct uk_fdtab *tab = _active_tab();
	void *p = uk_fmap_lookup(&tab->fmap, fd);
	struct fdval v;
	int ret;

	if (!p)
		return -EBADF;

	v = fdtab_decode(p);
	ret = 0;
	if (v.flags & UK_FDTAB_CLOEXEC)
		ret |= O_CLOEXEC;
	return ret;
}

#if CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM
#if CONFIG_LIBVFSCORE
int uk_fdtab_legacy_open(struct vfscore_file *vf)
{
	struct uk_fdtab *tab = _active_tab();
	const void *entry;
	int fd;

	fhold(vf);
	entry = fdtab_encode(vf, UK_FDTAB_VFSCORE);
	fd = uk_fmap_put(&tab->fmap, entry, 0);
	if (fd >= UK_FDTAB_SIZE)
		goto err_out;
	vf->fd = fd;
	return fd;
err_out:
	fdrop(vf);
	return -ENFILE;
}

struct vfscore_file *uk_fdtab_legacy_get(int fd)
{
	struct uk_fdtab *tab = _active_tab();
	struct uk_fmap *fmap = &tab->fmap;
	struct vfscore_file *vf = NULL;
	void *p = uk_fmap_critical_take(fmap, fd);

	if (p) {
		struct fdval v = fdtab_decode(p);

		if (v.flags & UK_FDTAB_VFSCORE) {
			vf = (struct vfscore_file *)v.p;
			fhold(vf);
		}
		uk_fmap_critical_put(fmap, fd, p);
	}
	return vf;
}
#endif /* CONFIG_LIBVFSCORE */

int uk_fdtab_shim_get(int fd, union uk_shim_file *out)
{
	struct uk_fdtab *tab;
	struct uk_fmap *fmap;
	void *p;

	if (fd < 0)
		return -1;

	tab = _active_tab();
	fmap = &tab->fmap;

	p = uk_fmap_critical_take(fmap, fd);
	if (p) {
		struct fdval v = fdtab_decode(p);

#if CONFIG_LIBVFSCORE
		if (v.flags & UK_FDTAB_VFSCORE) {
			struct vfscore_file *vf = (struct vfscore_file *)v.p;

			fhold(vf);
			uk_fmap_critical_put(fmap, fd, p);
			out->vfile = vf;
			return UK_SHIM_LEGACY;
		} else
#endif /* CONFIG_LIBVFSCORE */
		{
			struct uk_ofile *of = (struct uk_ofile *)v.p;

			ofile_acq(of);
			uk_fmap_critical_put(fmap, fd, p);
			out->ofile = of;
			return UK_SHIM_OFILE;
		}
	}
	return -1;
}
#endif /* CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */

static struct fdval _fdtab_get(struct uk_fdtab *tab, int fd)
{
	struct fdval ret = { NULL, 0 };

	if (fd >= 0) {
		/* Need to refcount atomically => critical take & put */
		struct uk_fmap *fmap = &tab->fmap;
		void *p = uk_fmap_critical_take(fmap, fd);

		if (p) {
			ret = fdtab_decode(p);
			file_acq(ret.p, ret.flags);
			uk_fmap_critical_put(fmap, fd, p);
		}
	}
	return ret;
}

struct uk_ofile *uk_fdtab_get(int fd)
{
	struct uk_fdtab *tab = _active_tab();
	struct fdval v = _fdtab_get(tab, fd);

#if CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM
	/* Report legacy files as not present if called through new API */
	if (v.p && v.flags & UK_FDTAB_VFSCORE) {
		file_rel(tab, v.p, v.flags);
		return NULL;
	}
#endif /* CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */
	return (struct uk_ofile *)v.p;
}

void uk_fdtab_ret(struct uk_ofile *of)
{
	UK_ASSERT(of);
	ofile_rel(_active_tab(), of);
}

void uk_fdtab_cloexec(void)
{
	struct uk_fdtab *tab = _active_tab();
	struct uk_fmap *fmap = &tab->fmap;

	for (int i = 0; i < UK_FDTAB_SIZE; i++) {
		void *p = uk_fmap_lookup(fmap, i);

		if (p) {
			struct fdval v = fdtab_decode(p);

			if (v.flags & UK_FDTAB_CLOEXEC) {
				void *pp = uk_fmap_take(fmap, i);

				UK_ASSERT(p == pp);
				file_rel(tab, v.p, v.flags);
			}
		}
	}
}

/* Internal Syscalls */

int uk_sys_close(int fd)
{
	struct uk_fdtab *tab;
	void *p;
	struct fdval v;

	tab = _active_tab();
	p = uk_fmap_take(&tab->fmap, fd);
	if (!p)
		return -EBADF;
	v = fdtab_decode(p);
	file_rel(tab, v.p, v.flags);
	return 0;
}

int uk_sys_dup3(int oldfd, int newfd, int flags)
{
	int r __maybe_unused;
	struct uk_fdtab *tab;
	struct fdval dup;
	void *prevp;
	const void *newent;

	if (oldfd == newfd)
		return -EINVAL;
	if (oldfd < 0 || oldfd >= UK_FDTAB_SIZE ||
	    newfd < 0 || newfd >= UK_FDTAB_SIZE)
		return -EBADF;
	if (flags & ~O_CLOEXEC)
		return -EINVAL;

	tab = _active_tab();
	dup = _fdtab_get(tab, oldfd);
	if (!dup.p)
		return -EBADF; /* oldfd not open */
	dup.flags &= ~UK_FDTAB_CLOEXEC;
	dup.flags |= flags ? UK_FDTAB_CLOEXEC : 0;

	prevp = NULL;
	newent = fdtab_encode(dup.p, dup.flags);
	r = uk_fmap_xchg(&tab->fmap, newfd, newent, &prevp);
	UK_ASSERT(!r); /* newfd should be in range */
	if (prevp) {
		struct fdval prevv = fdtab_decode(prevp);

		file_rel(tab, prevv.p, prevv.flags);
	}
	return newfd;
}

int uk_sys_dup2(int oldfd, int newfd)
{
	if (oldfd == newfd)
		if (uk_fmap_lookup(&(_active_tab())->fmap, oldfd))
			return newfd;
		else
			return -EBADF;
	else
		return uk_sys_dup3(oldfd, newfd, 0);
}

int uk_sys_dup_min(int oldfd, int min, int flags)
{
	struct uk_fdtab *tab;
	struct fdval dup;
	const void *newent;
	int fd;

	if (oldfd < 0)
		return -EBADF;
	if (flags & ~O_CLOEXEC)
		return -EINVAL;

	tab = _active_tab();
	dup = _fdtab_get(tab, oldfd);
	if (!dup.p)
		return -EBADF;
	dup.flags &= ~UK_FDTAB_CLOEXEC;
	dup.flags |= flags ? UK_FDTAB_CLOEXEC : 0;

	newent = fdtab_encode(dup.p, dup.flags);
	fd = uk_fmap_put(&tab->fmap, newent, min);
	if (fd >= UK_FDTAB_SIZE) {
		file_rel(tab, dup.p, dup.flags);
		return -ENFILE;
	}
	return fd;
}

int uk_sys_dup(int oldfd)
{
	return uk_sys_dup_min(oldfd, 0, 0);
}

/* Userspace Syscalls */

UK_SYSCALL_R_DEFINE(int, close, int, fd)
{
	return uk_sys_close(fd);
}

UK_SYSCALL_R_DEFINE(int, dup, int, oldfd)
{
	return uk_sys_dup(oldfd);
}

UK_SYSCALL_R_DEFINE(int, dup2, int, oldfd, int, newfd)
{
	return uk_sys_dup2(oldfd, newfd);
}

UK_SYSCALL_R_DEFINE(int, dup3, int, oldfd, int, newfd, int, flags)
{
	return uk_sys_dup3(oldfd, newfd, flags);
}
