/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE

#include <string.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/vfs.h>

#include <uk/alloc.h>
#include <uk/atomic.h>
#include <uk/errptr.h>
#include <uk/essentials.h>
#include <uk/fs.h>
#include <uk/fs/driver.h>
#include <uk/fs/pathutil.h>
#include <uk/mutex.h>
#include <uk/posix-fdio.h>
#include <uk/posix-vfs.h>
#include <uk/posix-vfsroot.h>
#include <uk/spinlock.h>

#if CONFIG_LIBPOSIX_VFS_MULTICTX
#include <uk/init.h>
#include <uk/refcount.h>
#include <uk/thread.h>
#endif /* CONFIG_LIBPOSIX_VFS_MULTICTX */

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#include <uk/process.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#if CONFIG_LIBPOSIX_VFS_MOUNT_CLEANUP
#include <uk/init.h>
#endif /* CONFIG_LIBPOSIX_VFS_MOUNT_CLEANUP */

/* posix-ey perms checking */
#if CONFIG_LIBPOSIX_VFS_PERMISSIONS
int uk_fs_checkperm(int desired __unused, int mode __unused,
		    int uid __unused, int gid __unused)
{
	/* TODO: impl & integrate w/ posix-user */
	return 0;
}
#endif /* CONFIG_LIBPOSIX_VFS_PERMISSIONS */

/* mount table */
struct vfs_mtab_entry {
	const struct uk_file *point;
	const struct uk_file *root;
	const struct uk_file *parent_root;
};

struct vfs_mtab {
	struct uk_mutex lock;
	size_t top;
	struct vfs_mtab_entry tab[CONFIG_LIBPOSIX_VFS_MAX_MOUNTS];
};

static struct vfs_mtab mtab = {
	.lock = UK_MUTEX_INITIALIZER(mtab.lock),
	.top = 0
};

#if CONFIG_LIBPOSIX_VFS_MOUNT_CLEANUP

static
void vfs_mtab_cleanup(struct uk_term_ctx *tctx __unused)
{
	/* This function is called at system teardown and is the only one
	 * touching the mtab; we can safely ignore any locks.
	 */
	for (size_t i = mtab.top; i-- > 0;) {
		const struct uk_file *point = mtab.tab[i].point;
		const struct uk_file *prev = NULL;
		int r = uk_fs_mountat(point, &prev);

		if (unlikely(r))
			uk_pr_warn("Unable to unmount %p: %d\n", point, r);
		else
			uk_file_release(prev);
	}
}

uk_rootfs_initcall(0x0, vfs_mtab_cleanup);

#endif /* CONFIG_LIBPOSIX_VFS_MOUNT_CLEANUP */

/**
 * Find the mount table entry that is the parent of directory `f`.
 *
 * Must be called with the mtab lock held.
 */
static
const struct vfs_mtab_entry *vfs_parent_mount(const struct uk_file *f)
{
	const struct uk_file *d = f;
	const struct vfs_mtab_entry *ret = NULL;
	const struct uk_file *parent __maybe_unused;
	unsigned int lflags = UKFS_LOOKUP_IGNMNT | UKFS_LOOKUP_NO_MNTAUX;

	/* Clearing flag for later UK_ASSERT */
	UK_ASSERT((lflags &= ~UKFS_LOOKUP_NO_MNTAUX));

	UK_ASSERT(uk_fs_isdir(d));
	do {
		union uk_fs_lookup_out lout;
		size_t prog __maybe_unused;
		int r = uk_fs_lookupat(d, "..", 2, lflags, &lout, &prog);

		switch (r) {
		case UKFS_STOP_MNT:
			UK_ASSERT((uk_file_release(lout.aux), lout.aux == d));
			parent = lout.target;
			uk_file_release(lout.target);
			goto found;
		case UKFS_STOP_NOD:
			if (!prog) {
				UK_ASSERT(lout.target == d);
				uk_file_release(lout.target);
				uk_file_poll(d, UKFD_POLLIN);
				continue;
			}
			UK_ASSERT(prog == 2);
			__fallthrough;
		case UKFS_SUCCESS:
			if (d == lout.target) {
				/* Root reached, no mount found */
				uk_file_release(lout.target);
				goto out;
			}
			if (d != f)
				uk_file_release(d);
			UK_ASSERT(lout.target != f);
			d = lout.target;
			break;
		case UKFS_STOP_FILE:
		case UKFS_STOP_SPEC:
		case UKFS_STOP_END:
		case UKFS_STOP_SYM:
			UK_CRASH("Impossible return from parent lookup: %d\n",
				 r);
		default:
			UK_CRASH("Unknown return from parent lookup: %d\n", r);
		}
	} while (1);
found:
	for (size_t i = mtab.top; i-- > 0;)
		if (d == mtab.tab[i].root) {
			UK_ASSERT(parent == mtab.tab[i].point);
			ret = &mtab.tab[i];
			break;
		}
out:
	if (d != f)
		uk_file_release(d);
	return ret;
}

/* per-process vfs state */

struct vfs_proc_state {
	const struct uk_file *fsroot;
	struct uk_ofile *cwd;
	struct uk_spinlock fsroot_lock;
	struct uk_spinlock cwd_lock;
	mode_t umask;
#if CONFIG_LIBPOSIX_VFS_MULTICTX
	__atomic refcnt;
#endif /* CONFIG_LIBPOSIX_VFS_MULTICTX */
};

static struct vfs_proc_state vfs_init_ctx = {
	.fsroot = &uk_posix_vfsroot_file,
	.cwd = &uk_posix_vfsroot_ofile,
	.fsroot_lock = UK_SPINLOCK_INITIALIZER(),
	.cwd_lock = UK_SPINLOCK_INITIALIZER(),
	.umask = 0,
#if CONFIG_LIBPOSIX_VFS_MULTICTX
	.refcnt = UK_REFCOUNT_INITIALIZER(1)
#endif /* CONFIG_LIBPOSIX_VFS_MULTICTX */
};

#if CONFIG_LIBPOSIX_VFS_MULTICTX

/* When using multi-vfsctx, the init thread has a ref to the init vfs ctx.
 *
 * Newly-created raw threads will start off with a copy of their parent's ref,
 * as a compatibility stop-gap.
 * It is the responsibility of other posix libs and their callbacks to more
 * meaningfully init a new thread's vfs ctx, unsharing as necessary.
 */

/* Every thread keeps a counted reference to a VFS context */
static __uk_tls struct vfs_proc_state *vfs_ctx;

static inline
void vfs_ctx_acquire(struct vfs_proc_state *ctx)
{
	uk_refcount_acquire(&ctx->refcnt);
}

static inline
void vfs_ctx_release(struct vfs_proc_state *ctx)
{
	if (uk_refcount_release(&ctx->refcnt)) {
		uk_file_release(ctx->fsroot);
		uk_ofile_release(ctx->cwd);
		uk_free(uk_alloc_get_default(), ctx);
	}
}

static
int init_posix_vfs(struct uk_init_ctx *ictx __unused)
{
	vfs_ctx_acquire(&vfs_init_ctx);
	vfs_ctx = &vfs_init_ctx;
	return 0;
}

uk_lib_initcall(init_posix_vfs, 0x0);

static
int vfs_thread_init(struct uk_thread *child, struct uk_thread *parent)
{
	struct vfs_proc_state *ctx;

	if (!parent)
		ctx = &vfs_init_ctx;
	else
		ctx = uk_thread_uktls_var(parent, vfs_ctx);
	vfs_ctx_acquire(ctx);
	uk_thread_uktls_var(child, vfs_ctx) = ctx;
	return 0;
}

static
void vfs_thread_term(struct uk_thread *child)
{
	struct vfs_proc_state *ctx = uk_thread_uktls_var(child, vfs_ctx);

	if (ctx)
		vfs_ctx_release(ctx);
}

UK_THREAD_INIT(vfs_thread_init, vfs_thread_term);

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING

static
struct vfs_proc_state *vfs_ctx_duplicate(struct vfs_proc_state *ctx)
{
	struct vfs_proc_state *newctx = uk_malloc(uk_alloc_get_default(),
						  sizeof(*newctx));

	if (newctx) {
		const struct uk_file *fsroot;
		struct uk_ofile *cwd;

		uk_spin_init(&newctx->fsroot_lock);
		uk_spin_init(&newctx->cwd_lock);
		newctx->refcnt = UK_REFCOUNT_INIT_VALUE(1);
		newctx->umask = ctx->umask;

		uk_spin_lock(&ctx->fsroot_lock);
		uk_spin_lock(&ctx->cwd_lock);
		fsroot = ctx->fsroot;
		cwd = ctx->cwd;
		uk_file_acquire(fsroot);
		uk_ofile_acquire(cwd);
		uk_spin_unlock(&ctx->cwd_lock);
		uk_spin_unlock(&ctx->fsroot_lock);

		newctx->fsroot = fsroot;
		newctx->cwd = cwd;
	}
	return newctx;
}

static
int vfs_clone(void *arg)
{
	struct posix_process_clone_event_data *event_data;
	const struct clone_args *cl_args;
	struct vfs_proc_state *newctx;
	struct vfs_proc_state *ctx;
	struct uk_thread *parent;
	struct uk_thread *child;

	event_data = (struct posix_process_clone_event_data *)arg;
	UK_ASSERT(event_data);

	cl_args = event_data->cl_args;
	UK_ASSERT(cl_args);

	child = event_data->child;
	parent = event_data->parent;

	ctx = uk_thread_uktls_var(parent, vfs_ctx);
	UK_ASSERT(ctx); /* Do not call clone from raw threads */
	if ((cl_args->flags & CLONE_FS)) {
		/* Inherit parent's vfs ctx */
		/* As a compat stop-gap, the raw thread already inherited the
		 * parent's vfs ctx; we don't need to do anything.
		 *
		 * TODO: move inheritance here once stopgap is removed.
		 */
		UK_ASSERT(uk_thread_uktls_var(child, vfs_ctx) == ctx);
		return UK_EVENT_HANDLED_CONT;
	} else {
		/* Duplicate parent's vfs ctx */
		newctx = vfs_ctx_duplicate(ctx);
		if (unlikely(!newctx))
			return -ENOMEM;
		/* Compat stop-gap: release previous duplicate ref */
		UK_ASSERT(uk_thread_uktls_var(child, vfs_ctx) == ctx);
		vfs_ctx_release(ctx);
	}
	uk_thread_uktls_var(child, vfs_ctx) = newctx;
	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_CLONE_HANDLER(CLONE_FS, vfs_clone);

#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#else /* !CONFIG_LIBPOSIX_VFS_MULTICTX */

/* All threads share the same static reference */
static struct vfs_proc_state *const vfs_ctx = &vfs_init_ctx;

#endif /* !CONFIG_LIBPOSIX_VFS_MULTICTX */

static inline
const struct uk_file *vfs_active_fsroot(void)
{
	const struct uk_file *fsroot;

	UK_ASSERT(vfs_ctx);
	uk_spin_lock(&vfs_ctx->fsroot_lock);
	fsroot = vfs_ctx->fsroot;
	UK_ASSERT(fsroot);
	uk_file_acquire(fsroot);
	uk_spin_unlock(&vfs_ctx->fsroot_lock);
	return fsroot;
}

static
const struct uk_file *vfs_set_fsroot(const struct uk_file *newroot)
{
	const struct uk_file *prev;

	UK_ASSERT(vfs_ctx);
	UK_ASSERT(newroot);
	uk_spin_lock(&vfs_ctx->fsroot_lock);
	prev = uk_exchange_n(&vfs_ctx->fsroot, newroot);
	uk_spin_unlock(&vfs_ctx->fsroot_lock);
	UK_ASSERT(prev);
	return prev;
}

static inline
struct uk_ofile *vfs_active_cwd(void)
{
	struct uk_ofile *cwd;

	UK_ASSERT(vfs_ctx);
	uk_spin_lock(&vfs_ctx->cwd_lock);
	cwd = vfs_ctx->cwd;
	UK_ASSERT(cwd);
	uk_ofile_acquire(cwd);
	uk_spin_unlock(&vfs_ctx->cwd_lock);
	return cwd;
}

static
struct uk_ofile *vfs_set_cwd(struct uk_ofile *newcwd)
{
	struct uk_ofile *prev;

	UK_ASSERT(vfs_ctx);
	UK_ASSERT(newcwd);
	uk_spin_lock(&vfs_ctx->cwd_lock);
	prev = uk_exchange_n(&vfs_ctx->cwd, newcwd);
	uk_spin_unlock(&vfs_ctx->cwd_lock);
	UK_ASSERT(prev);
	return prev;
}

static inline
mode_t vfs_apply_umask(mode_t mask)
{
	mode_t ubits = mask & 0777;

	ubits &= ~vfs_ctx->umask;
	return (mask & ~0777) | ubits;
}

/* path utils */

/**
 * INTERNAL. Retrieve the appropriate root for an *at lookup.
 *
 * @return
 *  - active filesystem root for absolute paths, otherwise
 *  - `f` if provided, otherwise
 *  - active current working directory
 */
static inline
const struct uk_file *vfs_atroot(const struct uk_file *f,
				 const char *path, size_t len)
{
	struct uk_ofile *cwd;
	const struct uk_file *ret;

	if (len >= 1 && path[0] == '/')
		return vfs_active_fsroot();
	if (f) {
		uk_file_acquire(f);
		return f;
	}
	cwd = vfs_active_cwd();
	ret = cwd->file;
	uk_file_acquire(ret);
	uk_ofile_release(cwd);
	return ret;
}

/**
 * INTERNAL. Retrieve the appropriate root for an *at lookup, along with a named
 * open file reference of the relative lookup base.
 *
 * @return
 *  - active filesystem root for absolute paths, otherwise
 *  - `of->file` if provided, otherwise
 *  - active current working directory
 */
static inline
const struct uk_file *vfs_atroot_named(struct uk_ofile *of,
				       const char *path, size_t len,
				       struct uk_ofile **base)
{
	struct uk_ofile *cwd;
	const struct uk_file *ret;

	if (len >= 1 && path[0] == '/')
		return vfs_active_fsroot();
	if (of) {
		uk_ofile_acquire(of);
		uk_file_acquire(of->file);
		*base = of;
		return of->file;
	}
	cwd = vfs_active_cwd();
	ret = cwd->file;
	uk_file_acquire(ret);
	*base = cwd;
	return ret;
}

static inline
int _vfs_should_lock(const struct uk_file *f)
{
	UK_ASSERT(uk_fs_isfs(f));
	return !(f->fsops->constraints & UKFS_MODE_NOIOLOCK);
}

static inline
void vfs_rlock(const struct uk_file *f)
{
	if (_vfs_should_lock(f))
		uk_file_rlock(f);
}

static inline
void vfs_runlock(const struct uk_file *f)
{
	if (_vfs_should_lock(f))
		uk_file_runlock(f);
}

static inline
void vfs_wlock(const struct uk_file *f)
{
	if (_vfs_should_lock(f))
		uk_file_wlock(f);
}

static inline
void vfs_wunlock(const struct uk_file *f)
{
	if (_vfs_should_lock(f))
		uk_file_wunlock(f);
}

#define VFS_LOOKUP_NOFOLLOW 1

/**
 * INTERNAL. Symlink follow function.
 */
#if CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH

static
const struct uk_file *_vfs_lookupat(const struct uk_file *atroot,
				    const char *path, size_t len,
				    unsigned int flags,
				    size_t lvl);

static inline
const struct uk_file *_vfs_symfollow(const struct uk_file *sym,
				     const struct uk_file *parent,
				     size_t lvl)
{
	struct uk_fs_path sympath;

	++lvl;
	if (unlikely(lvl > CONFIG_LIBPOSIX_VFS_MAX_SYMDEPTH))
		return ERR2PTR(-ELOOP);

	sympath = uk_fs_readlink(sym);
	return _vfs_lookupat(vfs_atroot(parent, sympath.s, sympath.len),
			     sympath.s, sympath.len, 0, lvl);
}

#elif CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM

static
const struct uk_file *_vfs_lookupat(const struct uk_file *atroot,
				    const char *path, size_t len,
				    unsigned int flags,
				    size_t *nsyms);

static inline
const struct uk_file *_vfs_symfollow(const struct uk_file *sym,
				     const struct uk_file *parent,
				     size_t *nsyms)

{
	struct uk_fs_path sympath;

	++*nsyms;
	if (unlikely(*nsyms > CONFIG_LIBPOSIX_VFS_MAX_SYMNUM))
		return ERR2PTR(-ELOOP);

	sympath = uk_fs_readlink(sym);
	return _vfs_lookupat(vfs_atroot(parent, sympath.s, sympath.len),
			     sympath.s, sympath.len, 0, nsyms);
}

#endif /* CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM */

/**
 * INTERNAL. Main lookup function.
 *
 * Uses `lvl`/`nsyms` to enforce a limit on symlinks traversed.
 * Consumes the `atroot` file reference.
 */
#if CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH
static
const struct uk_file *_vfs_lookupat(const struct uk_file *atroot,
				    const char *path, size_t len,
				    unsigned int flags,
				    size_t lvl)
#elif CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM
static
const struct uk_file *_vfs_lookupat(const struct uk_file *atroot,
				    const char *path, size_t len,
				    unsigned int flags,
				    size_t *nsyms)
#endif /* CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM */
{
	const struct uk_file *d = atroot;
	const struct uk_file *fsroot;
	const struct uk_file *ret;
	size_t pos;

	/* Skip leading '/' */
	while (len && path[0] == '/') {
		path++;
		len--;
	}
	/* vfs_active_fsroot() (& using locks) is only needed for a counted ref.
	 * We only need the pointer value for comparison; can simply read value.
	 */
	fsroot = vfs_ctx->fsroot;

	for (;;) {
		union uk_fs_lookup_out lout;
		int r;

		UK_ASSERT(d);
		UK_ASSERT(uk_fs_isfs(d));
		/* Skip parent lookups if at fsroot */
		if (d == fsroot && uk_fs_path_isdotdot(path, len)) {
			path += (len == 2) ? 2 : 3;
			len -= (len == 2) ? 2 : 3;
			continue;
		}
		/* Lookup in filesystem */
		vfs_rlock(d);
		r = uk_fs_lookupat(d, path, len, UKFS_LOOKUP_NO_MNTAUX,
				   &lout, &pos);
		vfs_runlock(d);
		if (r < 0) {
			ret = ERR2PTR(r);
			goto out;
		}

		switch (r) {
		case UKFS_SUCCESS:
			ret = lout.target;
			goto out;

		case UKFS_STOP_END:
			UK_ASSERT(pos < len);
			ret = ERR2PTR(-ENOTDIR);
			goto out;

		case UKFS_STOP_NOD:
			if (!pos) {
				/* No progress, should wait */
				UK_ASSERT(lout.target == d);
				uk_file_release(lout.target);
				uk_file_poll(d, UKFD_POLLIN);
			} else {
				/* Progress made, continue */
				uk_file_release(d);
				d = lout.target;
			}
			break;

		case UKFS_STOP_MNT:
			uk_file_release(d);
			d = lout.target;
			break;

		case UKFS_STOP_SYM:
			if ((flags & VFS_LOOKUP_NOFOLLOW) && pos == len) {
				/* No follow; return raw symlink */
				uk_file_release(lout.aux);
				ret = lout.target;
				goto out;
			}
			/* (Attempt to) follow symlink */
			ret = _vfs_symfollow(lout.target, lout.aux,
#if CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM
					     nsyms
#elif CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH
					     lvl
#endif /* CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH */
			);

			uk_file_release(lout.target);
			uk_file_release(lout.aux);
			if (unlikely(PTRISERR(ret)))
				goto out;
			uk_file_release(d);
			d = ret;
			break;

		case UKFS_STOP_FILE:
			if (pos == len) {
				/* Return adopted file verbatim */
				ret = lout.target;
			} else if (uk_fs_isfs(lout.target)) {
				/* Adopted file is FS-aware; continue lookup */
				uk_file_release(d);
				d = lout.target;
				break;
			} else {
				/* Deeper lookups not possible */
				uk_file_release(lout.target);
				ret = ERR2PTR(-ENOTDIR);
			}
			goto out;

		case UKFS_STOP_SPEC:
			if (pos == len) {
				uk_pr_warn("Special files not supported\n");
				ret = ERR2PTR(-ENXIO);
			} else {
				/* Deeper lookups not possible */
				uk_file_release(lout.target);
				ret = ERR2PTR(-ENOTDIR);
			}
			goto out;

		default:
			UK_CRASH("Invalid return from fs lookup\n");
		}

		UK_ASSERT(pos <= len);
		path += pos;
		len -= pos;
	}
out:
	uk_file_release(d);
	return ret;
}

/**
 * INTERNAL. Main lookup function entry point.
 *
 * Convenience wrapper to start a lookup at symlink depth 0.
 * Consumes the `atroot` reference.
 */
static inline
const struct uk_file *vfs_lookupat(const struct uk_file *atroot,
				   const char *path, size_t len,
				   unsigned int flags)
{
#if CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM
	size_t nsyms = 0;

	return _vfs_lookupat(atroot, path, len, flags, &nsyms);
#elif CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH
	return _vfs_lookupat(atroot, path, len, flags, 0);
#endif /* CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH */
}

#if CONFIG_LIBPOSIX_VFS_PERMISSIONS

#define VFS_MODEMASK (UK_STATX_TYPE | UK_STATX_MODE | \
		      UK_STATX_UID | UK_STATX_GID)

/**
 * INTERNAL. Check whether the filesystem permissions of `f` allow the `desired`
 * access type.
 *
 * @return
 *  == 0: OK
 *   < 0: Negative errno.
 */
static
int vfs_check_perms(const struct uk_file *f, mode_t desired)
{
	struct uk_statx sx;
	int r = uk_file_getstat(f, VFS_MODEMASK, &sx);

	if (!r) {
		UK_ASSERT((sx.stx_mask & VFS_MODEMASK) == VFS_MODEMASK);
		r = uk_fs_checkperm(desired, sx.stx_mode,
				    sx.stx_uid, sx.stx_gid);
	}
	return r;
}

#else /* !CONFIG_LIBPOSIX_VFS_PERMISSIONS */

static inline
int vfs_check_perms(const struct uk_file *f __unused, mode_t desired __unused)
{
	return 0;
}

#endif /* !CONFIG_LIBPOSIX_VFS_PERMISSIONS */

/**
 * INTERNAL. Check whether `path` is acceptable for an *at lookup.
 *
 * NULL paths are always rejected.
 * Empty paths are only accepted if AT_EMPTY_PATH is set in `flags`.
 *
 * @return
 *  == 0: OK
 *   < 0: Negative errno.
 */
static inline
int vfs_check_path(const char *path, int flags)
{
	if (!path)
		return -EFAULT;
	if (!path[0] && !(flags & AT_EMPTY_PATH))
		return -ENOENT;
	return 0;
}

/**
 * INTERNAL. Initialize in `of` a temporary open file description of `f` with
 * the open file mode `mode`, while also handling filesystem mode constraints.
 */
static inline
void vfs_tmpofile_init(struct uk_ofile *of, const struct uk_file *f,
		       unsigned int mode)
{
	if (uk_fs_isfs(f))
		mode |= uk_fs_mode_constraints(f);
	uk_ofile_init(of, f, mode);
}

/**
 * INTERNAL. Create a named open file description of `f` with the open file mode
 * `mode`, opened through `path` of length `pathlen` relative to open directory
 * `basedir`, while also handling filesystem mode constraints.
 */
static
struct uk_ofile *vfs_open_file(const struct uk_file *f, unsigned int mode,
			       struct uk_ofile *basedir,
			       const char *path, size_t pathlen)
{
	struct uk_ofile *of;
	const char *basename;
	size_t baselen;
	size_t len;

	UK_ASSERT(path);
	UK_ASSERT(pathlen);

	if (uk_fs_isfs(f))
		mode |= uk_fs_mode_constraints(f);
	if (path[0] == '/') {
		/* Absolute path */
		baselen = 0;
	} else {
		/* Relative path, prefix basedir name */
		UK_ASSERT(basedir);
		basename = uk_ofile_name(basedir, "");
		baselen = strlen(basename);
	}
	len = pathlen + (baselen ? baselen + 1 : 0);
	of = uk_ofile_new(f, mode, len);
	if (of) {
		char *p = of->name;

		if (baselen) {
			memcpy(p, basename, baselen);
			p[baselen] = '/';
			p += baselen + 1;
		}
		memcpy(p, path, pathlen);
		p[pathlen] = '\0';
	}
	return of;
}

/*
 * Internal syscalls
 */

/* FS ops */
void uk_sys_sync(void)
{
	uk_mutex_lock(&mtab.lock);
	for (size_t i = 0; i < mtab.top; i++)
		uk_sys_syncfs(mtab.tab[i].root);
	uk_mutex_unlock(&mtab.lock);
}

int uk_sys_statfs(const char *path, struct statfs *buf)
{
	const struct uk_file *target;
	size_t pathlen;
	int r;

	if (unlikely(!path))
		return -EFAULT;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(NULL, path, pathlen),
			      path, pathlen, 0);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);

	r = uk_sys_fstatfs(target, buf);
	uk_file_release(target);
	return r;
}

/* VFS context */
int uk_sys_fchdir(struct uk_ofile *of)
{
	struct uk_ofile *prev;
	int r;

	if (unlikely(!uk_fs_isdir(of->file)))
		return -ENOTDIR;
	if (unlikely((r = vfs_check_perms(of->file, X_OK))))
		return r;

	uk_ofile_acquire(of);
	prev = vfs_set_cwd(of);
	uk_ofile_release(prev);
	return 0;
}

int uk_sys_chdir(const char *path)
{
	const struct uk_file *dir;
	struct uk_ofile *odir;
	struct uk_ofile *base = NULL;
	size_t pathlen;
	int ret;

	if (unlikely(!path))
		return -EFAULT;

	pathlen = strlen(path);
	dir = vfs_lookupat(vfs_atroot_named(NULL, path, pathlen, &base),
			   path, pathlen, 0);
	if (unlikely(PTRISERR(dir))) {
		if (base)
			uk_ofile_release(base);
		return PTR2ERR(dir);
	}
	/* Create open file description and fchdir */
	odir = vfs_open_file(dir, O_RDONLY | O_PATH, base, path, pathlen);
	uk_file_release(dir);
	if (base)
		uk_ofile_release(base);
	if (unlikely(!odir))
		return -ENOMEM;
	ret = uk_sys_fchdir(odir);
	uk_ofile_release(odir);
	return ret;
}

ssize_t uk_sys_getcwd(char *buf, size_t len)
{
	struct uk_ofile *cwd = vfs_active_cwd();
	const char *cwdpath = uk_ofile_name(cwd, "");
	const size_t cwdlen = strlen(cwdpath);

	if (unlikely(cwdlen + 1 > len))
		return -ERANGE;
	memcpy(buf, cwdpath, cwdlen);
	buf[cwdlen] = '\0';
	return cwdlen + 1;
}

int uk_sys_chroot(const char *path)
{
	const struct uk_file *newroot;
	const struct uk_file *prev;
	size_t pathlen;
	int err;

	if (unlikely(!path))
		return -EFAULT;

	pathlen = strlen(path);
	newroot = vfs_lookupat(vfs_atroot(NULL, path, pathlen),
			       path, pathlen, 0);
	if (unlikely(PTRISERR(newroot)))
		return PTR2ERR(newroot);

	if (unlikely(!uk_fs_isdir(newroot))) {
		err = -ENOTDIR;
		goto out_err;
	}
	if (unlikely((err = vfs_check_perms(newroot, X_OK))))
		goto out_err;

	prev = vfs_set_fsroot(newroot);
	uk_file_release(prev);
	return 0;

out_err:
	uk_file_release(newroot);
	return err;
}

mode_t uk_sys_umask(mode_t mask)
{
	mask &= 0777;
	return uk_exchange_n(&vfs_ctx->umask, mask);
}

/* Read node metadata */
int uk_sys_faccessat(const struct uk_file *f, const char *path,
		     int mode, int flags)
{
	const int lflags = (flags & AT_SYMLINK_NOFOLLOW) ?
			   VFS_LOOKUP_NOFOLLOW : 0;
	const struct uk_file *target;
	size_t pathlen;
	int ret;

	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(f, path, pathlen),
			      path, pathlen, lflags);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);
	ret = (mode == F_OK) ? 0 : vfs_check_perms(target, mode);
	uk_file_release(target);
	return ret;
}

int uk_sys_statx(const struct uk_file *f, const char *restrict path, int flags,
		 unsigned int mask, struct uk_statx *restrict statxbuf)
{
	const int lflags = (flags & AT_SYMLINK_NOFOLLOW) ?
			   VFS_LOOKUP_NOFOLLOW : 0;
	const struct uk_file *target;
	struct uk_ofile of;
	size_t pathlen;
	int ret;

	if (unlikely(!statxbuf))
		return -EFAULT;
	if (unlikely((ret = vfs_check_path(path, flags))))
		return ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(f, path, pathlen),
			      path, pathlen, lflags);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);

	/* No perms checking is required on the file itself */
	vfs_tmpofile_init(&of, target, O_RDONLY);
	ret = uk_sys_fstatx(&of, mask, statxbuf);
	uk_file_release(target);
	return ret;
}

int uk_sys_fstatat(const struct uk_file *f, const char *restrict path,
		   struct stat *restrict statbuf, int flags)
{
	struct uk_statx sx;
	int ret;

	if (unlikely(!statbuf))
		return -EFAULT;

	ret = uk_sys_statx(f, path, flags, UK_STATX_BASIC_STATS, &sx);
	if (!ret)
		uk_fdio_statx_cpyout(statbuf, &sx);
	return ret;
}

/* Change node metadata */
int uk_sys_fchownat(const struct uk_file *f, const char *path,
		    uid_t owner, gid_t group, int flags)
{
	const int lflags = (flags & AT_SYMLINK_NOFOLLOW) ?
			   VFS_LOOKUP_NOFOLLOW : 0;
	const struct uk_file *target;
	struct uk_ofile of;
	size_t pathlen;
	int ret;

	if (unlikely(flags & ~(AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW)))
		return -EINVAL;
	if (unlikely((ret = vfs_check_path(path, flags))))
		return ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(f, path, pathlen),
			      path, pathlen, lflags);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);

	/* fchown should handle perms checking */
	vfs_tmpofile_init(&of, target, O_WRONLY);
	ret = uk_sys_fchown(&of, owner, group);
	uk_file_release(target);
	return ret;
}

int uk_sys_fchmodat(const struct uk_file *f, const char *path,
		    mode_t mode, int flags)
{
	const int lflags = (flags & AT_SYMLINK_NOFOLLOW) ?
			   VFS_LOOKUP_NOFOLLOW : 0;
	const struct uk_file *target;
	struct uk_ofile of;
	size_t pathlen;
	int ret;

	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(f, path, pathlen),
			      path, pathlen, lflags);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);

	/* fchmod should handle perms checking */
	vfs_tmpofile_init(&of, target, O_WRONLY);
	ret = uk_sys_fchmod(&of, mode);
	uk_file_release(target);
	return ret;
}

int uk_sys_futimesat(const struct uk_file *f, const char *path,
		     const struct timeval *times)
{
	const struct uk_file *target;
	struct uk_ofile of;
	size_t pathlen;
	int ret;

	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(f, path, pathlen), path, pathlen, 0);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);
	if (!times) {
		ret = vfs_check_perms(target, W_OK);
		if (unlikely(ret))
			goto out;
	}

	/* futimes should check perms for times != NULL */
	vfs_tmpofile_init(&of, target, O_WRONLY);
	ret = uk_sys_futimes(&of, times);
out:
	uk_file_release(target);
	return ret;
}

int uk_sys_utime(const char *path, const struct utimbuf *times)
{
	if (times) {
		struct timeval t[2] = {
			{ .tv_sec = times->actime },
			{ .tv_sec = times->modtime }
		};

		return uk_sys_futimesat(NULL, path, t);
	}
	return uk_sys_futimesat(NULL, path, NULL);
}

int uk_sys_utimensat(const struct uk_file *f, const char *path,
		     const struct timespec *times, int flags)
{
	const int lflags = (flags & AT_SYMLINK_NOFOLLOW) ?
			   VFS_LOOKUP_NOFOLLOW : 0;
	const struct uk_file *target;
	struct uk_ofile of;
	size_t pathlen;
	int ret;

	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(f, path, pathlen),
			      path, pathlen, lflags);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);
	if (!times ||
	    ((times[0].tv_nsec == UTIME_NOW ||
	      times[0].tv_nsec == UTIME_OMIT) &&
	     (times[1].tv_nsec == UTIME_NOW ||
	      times[1].tv_nsec == UTIME_OMIT))) {
		ret = vfs_check_perms(target, W_OK);
		if (unlikely(ret))
			goto out;
	}

	/* futimens should check perms for custom times */
	vfs_tmpofile_init(&of, target, O_WRONLY);
	ret = uk_sys_futimens(&of, times);
out:
	uk_file_release(target);
	return ret;
}

/* Read node contents */
ssize_t uk_sys_getdents64(const struct uk_file *f, size_t *curp,
			  void *dirp, size_t count)
{
	ssize_t ret;

	UK_ASSERT(curp);
	if (unlikely(!dirp))
		return -EFAULT;
	if (unlikely(!uk_fs_isdir(f)))
		return -ENOTDIR;

	vfs_rlock(f);
	ret = uk_fs_listdir(f, curp, dirp, count);
	vfs_runlock(f);
	return ret;
}

ssize_t uk_sys_readlinkat(const struct uk_file *f, const char *restrict path,
			  char *restrict buf, size_t bufsz)
{
	const struct uk_file *target;
	struct uk_fs_path linkpath;
	size_t pathlen;
	ssize_t ret;

	if (unlikely(!buf))
		return -EFAULT;
	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(f, path, pathlen),
			      path, pathlen, VFS_LOOKUP_NOFOLLOW);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);
	if (unlikely(!uk_fs_issym(target))) {
		ret = -EINVAL;
		goto out;
	}

	/* Symlinks uniquely don't have meaningful permissions to check */
	linkpath = uk_fs_readlink(target);
	ret = MIN(bufsz, linkpath.len);
	memcpy(buf, linkpath.s, ret);
out:
	uk_file_release(target);
	return ret;
}

/* Change node contents */
int uk_sys_truncate(const char *path, off_t len)
{
	const struct uk_file *target;
	struct uk_ofile of;
	size_t pathlen;
	int ret;

	pathlen = strlen(path);
	target = vfs_lookupat(vfs_atroot(NULL, path, pathlen),
			      path, pathlen, 0);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);

	ret = vfs_check_perms(target, W_OK);
	if (unlikely(ret))
		goto out;

	vfs_tmpofile_init(&of, target, O_WRONLY);
	ret = uk_sys_ftruncate(&of, len);
out:
	uk_file_release(target);
	return ret;
}

/* Create node w/o opening */
int uk_sys_mkdirat(const struct uk_file *f, const char *path, mode_t mode)
{
	const struct uk_file *newdir;
	const struct uk_file *dir;
	struct uk_fs_poslen p;
	int ret;

	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;

	/* Lookup parent of path */
	/* ignore trailing slashes (uniquely among *dir syscalls) */
	p = uk_fs_path_len_trail(path, 1);
	dir = vfs_lookupat(vfs_atroot(f, path, p.len), path, p.pos, 0);
	if (unlikely(PTRISERR(dir)))
		return PTR2ERR(dir);

	if (unlikely(!uk_fs_isdir(dir))) {
		ret = -ENOTDIR;
		goto out;
	}

	mode &= ~S_IFMT;
	mode |= S_IFDIR;

	vfs_wlock(dir);
	ret = vfs_check_perms(dir, W_OK);
	if (!ret)
		newdir = uk_fs_createat(dir, &path[p.pos], p.len - p.pos,
					vfs_apply_umask(mode), O_EXCL,
					UKFS_NOTARGET);
	vfs_wunlock(dir);
	if (unlikely(ret))
		goto out;
	if (unlikely(PTRISERR(newdir))) {
		ret = PTR2ERR(newdir);
		goto out;
	}

	uk_file_release(newdir);
out:
	uk_file_release(dir);
	return ret;
}

int uk_sys_linkat(const struct uk_file *olddir, const char *oldpath,
		  const struct uk_file *newdir, const char *newpath, int flags)
{
	union uk_fs_create_target target;
	const struct uk_file *dest;
	const struct uk_file *newlink;
	size_t oldpathlen;
	struct uk_fs_poslen np;
	int ret;

	if (unlikely((ret = vfs_check_path(newpath, 0))))
		return ret;
	if (unlikely(flags & ~(AT_EMPTY_PATH | AT_SYMLINK_FOLLOW)))
		return -EINVAL;
	if (unlikely((ret = vfs_check_path(oldpath, flags))))
		return ret;

	oldpathlen = strlen(oldpath);
	target.file = vfs_lookupat(vfs_atroot(olddir, oldpath, oldpathlen),
				   oldpath, oldpathlen,
				   (flags & AT_SYMLINK_FOLLOW) ?
				   0 : VFS_LOOKUP_NOFOLLOW);
	if (unlikely(PTRISERR(target.file)))
		return PTR2ERR(target.file);

	np = uk_fs_path_len_trail(newpath, 0);
	dest = vfs_lookupat(vfs_atroot(newdir, newpath, np.pos),
			    newpath, np.pos, 0);
	if (unlikely(PTRISERR(dest))) {
		ret = PTR2ERR(dest);
		goto out_targ;
	}

	if (unlikely(!uk_fs_isdir(dest))) {
		ret = -ENOTDIR;
		goto out_dest;
	}

	vfs_wlock(dest);
	ret = vfs_check_perms(dest, W_OK);
	if (!ret)
		newlink = uk_fs_createat(dest,
					 &newpath[np.pos], np.len - np.pos,
					 S_IFMT, O_EXCL, target);
	vfs_wunlock(dest);
	if (unlikely(ret))
		goto out_dest;
	if (unlikely(PTRISERR(newlink))) {
		ret = PTR2ERR(newlink);
		goto out_dest;
	}

	uk_file_release(newlink);
out_dest:
	uk_file_release(dest);
out_targ:
	uk_file_release(target.file);
	return ret;
}

int uk_sys_symlinkat(const char *targetpath, const struct uk_file *f,
		     const char *linkpath)
{
	const union uk_fs_create_target target = { .path = targetpath };
	const struct uk_file *dir;
	const struct uk_file *newsym;
	struct uk_fs_poslen p;
	int ret;

	if (unlikely((ret = vfs_check_path(linkpath, 0))))
		return ret;

	p = uk_fs_path_len_trail(linkpath, 0);
	dir = vfs_lookupat(vfs_atroot(f, linkpath, p.pos), linkpath, p.pos, 0);
	if (unlikely(PTRISERR(dir)))
		return PTR2ERR(dir);

	if (unlikely(!uk_fs_isdir(dir))) {
		ret = -ENOTDIR;
		goto out;
	}

	vfs_wlock(dir);
	ret = vfs_check_perms(dir, W_OK);
	if (!ret)
		newsym = uk_fs_createat(dir, &linkpath[p.pos], p.len - p.pos,
					S_IFLNK | 0777, O_EXCL, target);
	vfs_wunlock(dir);
	if (unlikely(ret))
		goto out;
	if (unlikely(PTRISERR(newsym))) {
		ret = PTR2ERR(newsym);
		goto out;
	}

	uk_file_release(newsym);
out:
	uk_file_release(dir);
	return ret;
}

int uk_sys_mknodat(const struct uk_file *f, const char *path,
		   mode_t mode, dev_t dev __unused)
{
	const mode_t ftype = mode & S_IFMT;
	const struct uk_file *dir;
	const struct uk_file *newf;
	struct uk_fs_poslen p;
	int ret;

	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;

	/* By default create regular file */
	if (!ftype)
		mode |= S_IFREG;
	else
		switch (ftype) {
		case S_IFREG:
			break;
		case S_IFCHR:
		case S_IFBLK:
		case S_IFIFO:
		case S_IFSOCK:
			uk_pr_warn("Special files not supported\n");
			__fallthrough;
		default:
			return -EINVAL;
		}

	p = uk_fs_path_len_trail(path, 0);
	dir = vfs_lookupat(vfs_atroot(f, path, p.pos), path, p.pos, 0);
	if (unlikely(PTRISERR(dir)))
		return PTR2ERR(dir);

	if (unlikely(!uk_fs_isdir(dir))) {
		ret = -ENOTDIR;
		goto out;
	}

	vfs_wlock(dir);
	ret = vfs_check_perms(dir, W_OK);
	if (!ret)
		newf = uk_fs_createat(dir, &path[p.pos], p.len - p.pos,
				      vfs_apply_umask(mode), O_EXCL,
				      UKFS_NOTARGET);
	vfs_wunlock(dir);
	if (unlikely(ret))
		goto out;
	if (unlikely(PTRISERR(newf))) {
		ret = PTR2ERR(newf);
		goto out;
	}

	uk_file_release(newf);
out:
	uk_file_release(dir);
	return ret;
}

/* Remove/rename node */
int uk_sys_unlinkat(const struct uk_file *f, const char *path, int flags)
{
	const struct uk_file *dir;
	struct uk_fs_poslen p;
	int uflags;
	int ret;

	if (unlikely((ret = vfs_check_path(path, 0))))
		return ret;
	if (unlikely(flags & ~AT_REMOVEDIR))
		return -EINVAL;

	if (flags & AT_REMOVEDIR) {
		uflags = UKFS_UNLINK_DIR;
	} else {
		uflags = 0;
#if !CONFIG_LIBPOSIX_VFS_RELAX_UNLINK
		uflags |= UKFS_UNLINK_NODIR;
#endif /* !CONFIG_LIBPOSIX_VFS_RELAX_UNLINK */
	}
#if !CONFIG_LIBPOSIX_VFS_RELAX_RMDIR
	uflags |= UKFS_UNLINK_EMPTY;
#endif /* !CONFIG_LIBPOSIX_VFS_RELAX_RMDIR */

	/* Get parent of path; rmdir ignores trailing slashes in path */
	p = uk_fs_path_len_trail(path, !!(flags & AT_REMOVEDIR));
	dir = vfs_lookupat(vfs_atroot(f, path, p.pos), path, p.pos, 0);
	if (unlikely(PTRISERR(dir)))
		return PTR2ERR(dir);

	if (unlikely(!uk_fs_isdir(dir))) {
		ret = -ENOTDIR;
		goto out;
	}

	vfs_wlock(dir);
	ret = vfs_check_perms(dir, W_OK);
	if (!ret)
		ret = uk_fs_unlinkat(dir, &path[p.pos], p.len - p.pos, uflags);
	vfs_wunlock(dir);
out:
	uk_file_release(dir);
	return ret;
}

/* Need to take care to prevent deadlocks when locking both src and dest dirs */
static
void vfs_rename_lock(const struct uk_file *sdir, const struct uk_file *ddir)
{
	const struct uk_file *d1, *d2;

	/* We always lock & unlock in ascending state pointer order */
	if (sdir->state <= ddir->state) {
		d1 = sdir;
		d2 = ddir;
	} else {
		d1 = ddir;
		d2 = sdir;
	}
	vfs_wlock(d1);
	/* We do not lock the 2nd directory if it shares state with the 1st */
	if (d1->state != d2->state)
		vfs_wlock(d2);
}

static
void vfs_rename_unlock(const struct uk_file *sdir, const struct uk_file *ddir)
{
	const struct uk_file *d1, *d2;

	/* We always lock & unlock in ascending state pointer order */
	if (sdir->state <= ddir->state) {
		d1 = sdir;
		d2 = ddir;
	} else {
		d1 = ddir;
		d2 = sdir;
	}
	/* We do not lock the 2nd directory if it shares state with the 1st */
	if (d1->state != d2->state)
		vfs_wunlock(d2);
	vfs_wunlock(d1);
}

int uk_sys_renameat(const struct uk_file *olddir, const char *oldpath,
		    const struct uk_file *newdir, const char *newpath,
		    int flags)
{
	const struct uk_file *sdir;
	const struct uk_file *ddir;
	struct uk_fs_poslen op;
	struct uk_fs_poslen np;
	int ret;

	if (unlikely((ret = vfs_check_path(oldpath, 0))))
		return ret;
	if (unlikely((ret = vfs_check_path(newpath, 0))))
		return ret;

	op = uk_fs_path_len_trail(oldpath, 1);
	sdir = vfs_lookupat(vfs_atroot(olddir, oldpath, op.pos),
			    oldpath, op.pos, 0);
	if (unlikely(PTRISERR(sdir)))
		return PTR2ERR(sdir);
	if (unlikely(!uk_fs_isdir(sdir))) {
		ret = -ENOTDIR;
		goto out_src;
	}

	np = uk_fs_path_len_trail(newpath, 0);
	ddir = vfs_lookupat(vfs_atroot(newdir, newpath, np.pos),
			    newpath, np.pos, 0);
	if (unlikely(PTRISERR(ddir))) {
		ret = PTR2ERR(ddir);
		goto out_src;
	}
	if (unlikely(!uk_fs_isdir(ddir))) {
		ret = -ENOTDIR;
		goto out_dest;
	}

	vfs_rename_lock(sdir, ddir);
	ret = vfs_check_perms(sdir, W_OK);
	if (unlikely(ret))
		goto out_unlock;

	ret = vfs_check_perms(ddir, W_OK);
	if (unlikely(ret))
		goto out_unlock;

	ret = uk_fs_renameat(sdir, &oldpath[op.pos], op.len - op.pos,
			     ddir, &newpath[np.pos], np.len - np.pos,
			     flags);
out_unlock:
	vfs_rename_unlock(sdir, ddir);
out_dest:
	uk_file_release(ddir);
out_src:
	uk_file_release(sdir);
	return ret;
}

/* Mounting */
int uk_sys_mount(const char *source, const char *targetpath, const char *fstype,
		 unsigned long flags, void *data)
{
	const struct uk_file *target;
	const struct uk_file *target_p;
	const struct uk_file *newmnt;
	const struct vfs_mtab_entry *parent_ent;
	struct uk_fs_poslen p;
	int ret;

	if (unlikely(flags & MS_REMOUNT)) {
		uk_pr_err("MS_REMOUNT not supported\n");
		return -ENOSYS;
	}
	if (unlikely(flags & MS_MOVE)) {
		uk_pr_err("MS_MOVE not supported\n");
		return -ENOSYS;
	}

	/* TODO: require privileges for (un)mounting */

	/* Find mount target */
	p = uk_fs_path_len_trail(targetpath, 0);
	target = vfs_lookupat(vfs_atroot(NULL, targetpath, p.len),
			      targetpath, p.len, 0);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);

	/* Find mount point parent */
	if (uk_fs_isdir(target)) {
		uk_file_acquire(target);
		target_p = target;
	} else {
		target_p = vfs_lookupat(vfs_atroot(NULL, targetpath, p.pos),
					targetpath, p.pos, 0);
		if (unlikely(PTRISERR(target_p))) {
			ret = PTR2ERR(target_p);
			goto out_targ;
		}
	}

	if (flags & MS_BIND) {
		const struct uk_file *src;
		size_t srclen;

		srclen = strlen(source);
		src = vfs_lookupat(vfs_atroot(NULL, source, srclen),
				   source, srclen, 0);

		if (unlikely(PTRISERR(src))) {
			ret = PTR2ERR(src);
			goto out_targ_p;
		}
		newmnt = uk_fs_rebind(src, flags, data);
		uk_file_release(src);
	} else {
		uk_fs_vopen_func vopen = uk_fs_driver(fstype);

		if (unlikely(!vopen)) {
			ret = -ENODEV;
			goto out_targ_p;
		}
		newmnt = vopen(source, flags, data);
	}
	if (unlikely(PTRISERR(newmnt))) {
		ret = PTR2ERR(newmnt);
		goto out_targ_p;
	}
	/* uk_fs_mount */
	uk_mutex_lock(&mtab.lock);
	if (unlikely(mtab.top == CONFIG_LIBPOSIX_VFS_MAX_MOUNTS)) {
		ret = -EMFILE;
		goto out_unlock;
	}
	if (uk_fs_isdir(newmnt)) {
		ret = uk_fs_graft(newmnt, target);
		if (unlikely(ret))
			goto out_unlock;
	}
	ret = uk_fs_mountat(target, &newmnt);
	if (unlikely(ret))
		goto out_unlock;
	/* Mount is now live in VFS, cannot fail past this point */
	parent_ent = vfs_parent_mount(target_p);
	mtab.tab[mtab.top].point = target;
	mtab.tab[mtab.top].root = newmnt;
	mtab.tab[mtab.top].parent_root = parent_ent ? parent_ent->root : NULL;
	mtab.top++;
out_unlock:
	uk_mutex_unlock(&mtab.lock);
	uk_file_release(newmnt);
out_targ_p:
	uk_file_release(target_p);
out_targ:
	uk_file_release(target);
	return ret;
}

int uk_sys_umount(const char *targetpath, int flags __unused)
{
	const struct uk_file *target;
	size_t pathlen;
	size_t mntidx;
	int ret;
	int inval __maybe_unused = 0;

	/* TODO: require privileges for (un)mounting */

	pathlen = strlen(targetpath);
	target = vfs_lookupat(vfs_atroot(NULL, targetpath, pathlen),
			      targetpath, pathlen, 0);
	if (unlikely(PTRISERR(target)))
		return PTR2ERR(target);

	uk_mutex_lock(&mtab.lock);
	/* Find target in mtab */
	for (mntidx = 0; mntidx < mtab.top; mntidx++) {
		if (mtab.tab[mntidx].parent_root == target)
			inval = 1;
		if (mtab.tab[mntidx].root == target)
			break;
	}
	UK_ASSERT(!inval);
	if (mntidx != mtab.top) {
		const struct uk_file *prev = NULL;

		for (size_t i = mntidx + 1; i < mtab.top; i++)
			if (unlikely(mtab.tab[i].parent_root == target)) {
				ret = -EBUSY;
				goto out;
			}
		/* unmount & release */
		ret = uk_fs_mountat(mtab.tab[mntidx].point, &prev);
		if (unlikely(ret))
			goto out;
		UK_ASSERT(prev == target);
		uk_file_release(prev);
		/* rm from mtab */
		for (size_t i = mntidx + 1; i < mtab.top; i++)
			mtab.tab[i - 1] = mtab.tab[i];
		mtab.top--;
		ret = 0;
	} else {
		ret = -EINVAL;
	}
out:
	uk_mutex_unlock(&mtab.lock);
	uk_file_release(target);
	return ret;
}

/* Open */

/**
 * Lookup a path, following all symlinks; if not exist but could be created,
 * return a reference to a parent dir, the desired filename, and an optional
 * reference to the symlink that ensures the filename's lifetime.
 *
 * The reference to `atroot` is consumed by this call.
 */
static
int vfs_lookupat_parent(const struct uk_file *atroot,
			const char *path, size_t pathlen,
			const struct uk_file **ref, const char **name,
			size_t *len, const struct uk_file **symref)
{
	const struct uk_file *sym = NULL;
	const struct uk_file *f = atroot;
	size_t symlvl = 0;
	int ret;

	do {
		size_t ppos = pathlen;
		const struct uk_file *dir;
		const struct uk_file *target;

		while (ppos && path[ppos - 1] != '/')
			ppos--;
		dir = _vfs_lookupat(f, path, ppos, 0,
#if CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH
				    symlvl
#elif CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM
				    &symlvl
#endif /* CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM */
		);
		if (unlikely(PTRISERR(dir))) {
			ret = PTR2ERR(dir);
			break;
		}
		/* f is consumed at this point */
		/* Take extra ref on dir as following call consumes 1 ref */
		uk_file_acquire(dir);
		target = _vfs_lookupat(dir, &path[ppos], pathlen - ppos,
				       VFS_LOOKUP_NOFOLLOW,
#if CONFIG_LIBPOSIX_VFS_SYMLIMIT_DEPTH
				     symlvl
#elif CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM
				     &symlvl
#endif /* CONFIG_LIBPOSIX_VFS_SYMLIMIT_NUM */
		);
		if (PTRISERR(target)) {
			if (PTR2ERR(target) == -ENOENT) {
				/* Reached end & not present, ret parent */
				*ref = dir;
				*name = &path[ppos];
				*len = pathlen - ppos;
				*symref = sym;
				ret = 1;
			} else {
				uk_file_release(dir);
				ret = PTR2ERR(target);
				UK_ASSERT(ret < 0);
			}
			break;
		}
		/* Present; if symlink follow, if not ret success */
		if (sym)
			uk_file_release(sym);
		if (uk_fs_issym(target)) {
			struct uk_fs_path p = uk_fs_readlink(target);

			f = dir;
			path = p.s;
			pathlen = p.len;
			sym = target;
			symlvl++;
		} else {
			uk_file_release(dir);
			*ref = target;
			ret = 0;
			break;
		}
	} while (1);
	return ret;
}

static
const struct uk_file *vfs_createx(struct uk_ofile *of,
				  const char *path, mode_t mode,
				  size_t *pathlen,
				  struct uk_ofile **base)
{
	const struct uk_file *dir;
	const struct uk_file *newf;
	struct uk_fs_poslen p;
	int r;

	p = uk_fs_path_len_trail(path, 0);
	*pathlen = p.len;
	dir = vfs_lookupat(vfs_atroot_named(of, path, p.pos, base),
			   path, p.pos, 0);
	if (unlikely(PTRISERR(dir)))
		return dir;
	if (unlikely(!uk_fs_isdir(dir))) {
		newf = ERR2PTR(-ENOTDIR);
		goto out;
	}

	vfs_wlock(dir);
	r = vfs_check_perms(dir, W_OK);
	if (unlikely(r))
		newf = ERR2PTR(r);
	else
		newf = uk_fs_createat(dir, &path[p.pos], p.len - p.pos,
				      mode, O_EXCL, UKFS_NOTARGET);
	vfs_wunlock(dir);
out:
	uk_file_release(dir);
	return newf;
}

static
const struct uk_file *vfs_create(struct uk_ofile *of,
				 const char *path, size_t len,
				 mode_t mode, int *created,
				 struct uk_ofile **base)
{
	UK_ASSERT(base && !*base);
	do {
		const struct uk_file *newf;
		const struct uk_file *ref = NULL;
		const struct uk_file *symref = NULL;
		const char *fname = NULL;
		size_t fnlen = 0;
		int r;

		r = vfs_lookupat_parent(vfs_atroot_named(of, path, len, base),
					path, len,
					&ref, &fname, &fnlen, &symref);
		if (!r) {
			/* Lookup success */
			UK_ASSERT(ref);
			return ref;
		}
		if (unlikely(r != 1)) {
			/* Lookup error */
			UK_ASSERT(r < 0);
			return ERR2PTR(r);
		}
		/* Not found, but could be created */
		UK_ASSERT(ref);
		UK_ASSERT(uk_fs_isdir(ref));

		vfs_wlock(ref);
		r = vfs_check_perms(ref, W_OK);
		if (unlikely(r))
			newf = ERR2PTR(r);
		else
			newf = uk_fs_createat(ref, fname, fnlen, mode, O_EXCL,
					      UKFS_NOTARGET);
		vfs_wunlock(ref);

		uk_file_release(ref);
		if (symref)
			uk_file_release(symref);

		if (PTRISERR(newf) && PTR2ERR(newf) == -EEXIST) {
			if (*base) {
				uk_ofile_release(*base);
				*base = NULL;
			}
			continue;
		}
		if (!PTRISERR(newf))
			*created = 1;
		return newf;
	} while (1);
}

struct uk_ofile *uk_sys_openat(struct uk_ofile *of, const char *path,
			       int flags, mode_t mode)
{
	const int oflags = flags & UKFD_MODE_MASK;
	struct uk_ofile *base = NULL;
	int created = 0; /* Set to non-zero if we create a new file */
	const struct uk_file *target;
	struct uk_ofile *ret;
	size_t pathlen;
	int err;

	if (unlikely((flags & O_CREAT) && (flags & O_DIRECTORY)))
		return ERR2PTR(-EINVAL);
	if (unlikely(((flags & O_TMPFILE) == O_TMPFILE) &&
		     !((flags & O_WRONLY) || (flags & O_RDWR))))
		return ERR2PTR(-EINVAL);

	/* Open creates only regular files */
	mode = (mode & ~S_IFMT) | S_IFREG;
	mode = vfs_apply_umask(mode);

	/* Do actual open */
	switch (flags & (O_CREAT | O_EXCL | O_TMPFILE)) {
	case O_CREAT | O_TMPFILE:
	case O_CREAT | O_TMPFILE | O_EXCL:
		return ERR2PTR(-EINVAL);
	case O_CREAT | O_EXCL:
		target = vfs_createx(of, path, mode, &pathlen, &base);
		created = 1;
		break;
	case O_CREAT:
		pathlen = strlen(path);
		target = vfs_create(of, path, pathlen, mode, &created, &base);
		break;
	case O_TMPFILE | O_EXCL:
		uk_pr_warn("O_EXCL ignored in combination with O_TMPFILE\n");
		__fallthrough;
	case O_TMPFILE:
		uk_pr_warn("O_TMPFILE not supported\n");
		return ERR2PTR(-ENOSYS);
	default:
		/* lookup & ret */
		pathlen = strlen(path);
		target = vfs_lookupat(vfs_atroot_named(of, path, pathlen,
						       &base),
				      path, pathlen,
				      (flags & O_NOFOLLOW) ?
				      VFS_LOOKUP_NOFOLLOW : 0);
		break;
	}
	if (unlikely(PTRISERR(target))) {
		ret = ERR2PTR(PTR2ERR(target));
		goto out_base;
	}

	/* Check file type & perms */
	if (!created && !(flags & O_PATH)) {
		if (uk_fs_isdir(target)) {
			if (unlikely((flags & O_WRONLY) || (flags & O_RDWR))) {
				ret = ERR2PTR(-EISDIR);
				goto out_targ;
			}
			if (unlikely((err = vfs_check_perms(target, R_OK)))) {
				ret = ERR2PTR(err);
				goto out_targ;
			}
		} else if (unlikely(flags & O_DIRECTORY)) {
			ret = ERR2PTR(-ENOTDIR);
			goto out_targ;
		}
		if (unlikely(uk_fs_issym(target))) {
			ret = ERR2PTR(-ELOOP);
			goto out_targ;
		}
		if (!uk_fs_isdir(target) && !uk_fs_issym(target)) {
			/* Check access perms */
			mode_t desired = (flags & O_WRONLY) ? W_OK : R_OK;

			if (flags & O_RDWR)
				desired |= R_OK | W_OK;
			err = vfs_check_perms(target, desired);
			if (unlikely(err)) {
				ret = ERR2PTR(err);
				goto out_targ;
			}
		}
	}
	/* Open named file description */
	ret = vfs_open_file(target, oflags, base, path, pathlen);
	if (unlikely(!ret))
		ret = ERR2PTR(-ENOMEM);
out_targ:
	uk_file_release(target);
out_base:
	if (base)
		uk_ofile_release(base);

	/* Truncate if success and required */
	if (!PTRISERR(ret) && (flags & O_TRUNC) &&
	    ((flags & O_WRONLY) || (flags & O_RDWR))) {
		err = uk_sys_ftruncate(ret, 0);
		if (unlikely(err)) {
			UK_ASSERT(err < 0);
			uk_ofile_release(ret);
			ret = ERR2PTR(err);
		}
	}

	return ret;
}
