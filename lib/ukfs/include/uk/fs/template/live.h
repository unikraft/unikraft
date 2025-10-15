/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Unikraft filesystem template for live reference drivers.
 *
 * This header file contains boilerplate code for implementing a Unikraft
 * filesystem driver based on code that natively handles live references.
 * This functionality is provided in the form of code generation macros that
 * define functions conforming to the ukfile and ukfs file operation APIs,
 * functions which in turn call into driver-implemented live reference ops.
 *
 * A "live reference" to a file is understood to be a reference that handles
 * both file state (struct uk_file_state) as well as node refcounting and
 * lifetime. A live reference does not need to concern itself with other runtime
 * properties such as mount points or filesystem instances.
 */

#ifndef __UKFS_FS_TEMPLATE_LIVE_H__
#define __UKFS_FS_TEMPLATE_LIVE_H__

#include <sys/mount.h>
#include <sys/stat.h>

#include <uk/alloc.h>
#include <uk/atomic.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/fs.h>
#include <uk/fs/common-ops.h>
#include <uk/fs/pathutil.h>
#include <uk/refcount.h>
#include <uk/rwlock.h>
#include <uk/sched.h>
#include <uk/spinlock.h>
#include <uk/tree.h>

/* Types of special filesystem nodes recognized by the live driver template */
enum uk_fs_tmpl_node_kind {
	UK_FS_TMPL_DIR, /* Directory */
	UK_FS_TMPL_SYM /* Symbolic link */
};

/* Driver-specific types */

#define UK_FS_TMPL_LIVE_CREATE_TARGET(drv) drv##_LIVE_CREATE_TARGET

/**
 * Define the specific types of live driver `drv` with nodes of `nodetype`.
 *
 * @param drv Driver name
 * @param nodetype Type used for live node references
 */
#define UK_FS_TMPL_LIVE_TYPES(drv, nodetype)				\
/* Target argument type for live create operation */			\
union UK_FS_TMPL_LIVE_CREATE_TARGET(drv) {				\
	nodetype livenode;						\
	union uk_fs_create_target ukfs;					\
}

/* Live ops declarations
 *
 * These operations are implemented by the live driver and supplied to the
 * appropriate code generation macro(s). In broad terms, these map to
 * ukfile/ukfs operations, but with filesystem nodes referenced by `nodetype`.
 * Additionally, the live driver must supply a set of "glue" ops, that help
 * the template code interpret and manage live node references.
 *
 * The extra argument `mntflags`, when present, contains the standard mount
 * flags active on the filesystem instance that the request is coming from.
 *
 * Live ops that return a `nodetype` are assumed to return an active
 * (refcounted) reference that the caller is responsible for releasing.
 * Such operations should signal error conditions by special values of
 * `nodetype` that are mapped to negative error codes by `live_errnode`.
 *
 * For best optimizaton, these functions should be implemented statically in
 * the same compilation unit as the UK_FS_TMPL_LIVE_GEN_* macro that uses them.
 */

/**
 * Define the live ops function types for driver `drv` with nodes of `nodetype`.
 *
 * Requires previous use of `UK_FS_TMPL_LIVE_TYPES(drv, nodetype)`.
 *
 * @param drv Driver name
 * @param nodetype Type used for live node references
 */
#define UK_FS_TMPL_LIVE_OPS(drv, nodetype)				\
/* Similar to ukfs vopen; return new root via nodetype */		\
typedef nodetype (*drv##LIVEOP_VOPEN)(union uk_fs_vopen_vol vol,	\
				      unsigned long flags,		\
				      union uk_fs_vopen_data data,	\
				      size_t fmt);			\
\
/* Standard ukfile operations */					\
typedef ssize_t (*drv##LIVEOP_IO)(nodetype n, const struct iovec *iov,	\
				  size_t iovcnt, size_t off, long flags,\
				  unsigned long mntflags);		\
typedef ssize_t (*drv##LIVEOP_MEM)(nodetype n, enum uk_file_mem_op op,	\
				   size_t off, size_t len,		\
				   struct iovec *iov, size_t iovcnt,	\
				   unsigned long mntflags);		\
typedef int (*drv##LIVEOP_GETSTAT)(nodetype n, unsigned int mask,	\
				   struct uk_statx *arg,		\
				   unsigned long mntflags);		\
typedef int (*drv##LIVEOP_SETSTAT)(nodetype n, unsigned int mask,	\
				   const struct uk_statx *arg,		\
				   unsigned long mntflags);		\
typedef int (*drv##LIVEOP_CTL)(nodetype n, int fam, int req,		\
			       uintptr_t arg1, uintptr_t arg2,		\
			       uintptr_t arg3, unsigned long mntflags);	\
\
/* Subset of standard ukfs operations */				\
typedef int (*drv##LIVEOP_FS_STAT)(nodetype n, struct statfs *buf);	\
typedef int (*drv##LIVEOP_FS_SYNC)(nodetype n);				\
typedef int (*drv##LIVEOP_FS_LOOKUP)(nodetype n,			\
				     const char *path, size_t len,	\
				     nodetype *out_nodes,		\
				     union uk_fs_lookup_out *out_ukfs,	\
				     size_t *nout);			\
typedef ssize_t (*drv##LIVEOP_FS_LISTDIR)(nodetype n, size_t *curp,	\
					  void *buf, size_t len,	\
					  unsigned long mntflags);	\
typedef nodetype (*drv##LIVEOP_FS_CREATE)(nodetype n,			\
	const char *name, size_t len, unsigned int mode, int flags,	\
	union UK_FS_TMPL_LIVE_CREATE_TARGET(drv) target,		\
	unsigned long mntflags);					\
typedef int (*drv##LIVEOP_FS_UNLINK)(nodetype n, const char *name,	\
				     size_t len, unsigned int flags,	\
				     unsigned long mntflags);		\
typedef int (*drv##LIVEOP_FS_RENAME)(nodetype n,			\
				     const char *name, size_t nlen,	\
				     nodetype dest,			\
				     const char *dname, size_t dlen,	\
				     unsigned int flags,		\
				     unsigned long mntflags);		\
typedef struct uk_fs_path (*drv##LIVEOP_FS_READLINK)(nodetype n);	\
\
/* Glue operations */							\
/* Return non-zero if `n` is a fs node of `kind` */			\
typedef int (*drv##LIVEOP_NKIND)(const nodetype n,			\
				 enum uk_fs_tmpl_node_kind kind);	\
/* Return pointer to live file state of `n`. */				\
/* File state is only valid as long as there are active references to `n`. */\
typedef struct uk_file_state *(*drv##LIVEOP_STATE)(nodetype n);		\
/* If `n` encodes an error, return a negative error code, otherwise 0 */\
typedef int (*drv##LIVEOP_ERRNODE)(const nodetype n);			\
/* Acquire a reference to `n` */					\
typedef void (*drv##LIVEOP_ACQUIRE)(nodetype n);			\
/* Release a reference to `n`, running any cleanup if needed */		\
typedef void (*drv##LIVEOP_RELEASE)(nodetype n)

/* Code generation
 *
 * The live driver template offers flexibility in code organization and
 * generation, allowing for implementations such as:
 * - single-file driver: all live operations are implemented in a single source
 *   file, bundled together in a static ops_table and the live driver uses
 *   UK_FS_TMPL_LIVE_GENERATE_STATIC to generate all ukfs driver code
 * - multi-file driver: common declarations are generated with
 *   UK_FS_TMPL_LIVE_DECL_COMMON and placed in a common header, with actual
 *   implementations generated with UK_FS_TMPL_LIVE_GEN_* and placed in their
 *   respective source files
 * - any variation of the above, using UK_FS_TMPL_LIVE_DECL_* and
 *   UK_FS_TMPL_LIVE_GEN_* as appropriate
 */

/**
 * Define `struct sname` as live ops table for driver `drv`.
 *
 * For use by single-file drivers with `UK_FS_TMPL_LIVE_GENERATE_STATIC`.
 *
 * Requires previous use of `UK_FS_TMPL_LIVE_OPS(drv, *)`.
 *
 * @param drv Driver name
 * @param sname Name of struct to define
 */
#define UK_FS_TMPL_LIVE_OPSTABLE(drv, sname)				\
struct sname {								\
	/* Driver ops */						\
	drv##LIVEOP_VOPEN live_vopen;					\
	/* File ops */							\
	drv##LIVEOP_IO live_read;					\
	drv##LIVEOP_IO live_write;					\
	drv##LIVEOP_MEM live_mem;					\
	drv##LIVEOP_GETSTAT live_getstat;				\
	drv##LIVEOP_SETSTAT live_setstat;				\
	drv##LIVEOP_CTL live_ctl;					\
	/* FS ops */							\
	drv##LIVEOP_FS_STAT live_fs_stat;				\
	drv##LIVEOP_FS_SYNC live_fs_sync;				\
	drv##LIVEOP_FS_LOOKUP live_fs_lookup;				\
	drv##LIVEOP_FS_LISTDIR live_fs_listdir;				\
	drv##LIVEOP_FS_CREATE live_fs_create;				\
	drv##LIVEOP_FS_UNLINK live_fs_unlink;				\
	drv##LIVEOP_FS_RENAME live_fs_rename;				\
	drv##LIVEOP_FS_READLINK live_fs_readlink;			\
	/* Glue ops */							\
	drv##LIVEOP_NKIND live_nodekind;				\
	drv##LIVEOP_STATE live_state;					\
	drv##LIVEOP_ERRNODE live_errnode;				\
	drv##LIVEOP_ACQUIRE live_acquire;				\
	drv##LIVEOP_RELEASE live_release;				\
}

/**
 * Generate all driver code as static functions for single-file drivers.
 *
 * @param drv Driver name
 * @param nodetype Type used for live node references
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param ops_table Table of live operations (see live ops above)
 * @param mode_constraints Open file mode constraints
 */
#define UK_FS_TMPL_LIVE_GENERATE_STATIC(drv, nodetype, nodecmp,		\
					ops_table, mode_constraints)	\
UK_FS_TMPL_LIVE_DECL_COMMON_STATIC(drv, nodetype);			\
\
UK_FS_TMPL_LIVE_GEN_COMMON_STATIC(drv, nodecmp, (ops_table).live_nodekind,\
				  (ops_table).live_acquire,		\
				  (ops_table).live_release,		\
				  (ops_table).live_state);		\
\
UK_FS_TMPL_LIVE_GEN_READ_STATIC(drv, (ops_table).live_read);		\
UK_FS_TMPL_LIVE_GEN_WRITE_STATIC(drv, (ops_table).live_write);		\
UK_FS_TMPL_LIVE_GEN_MEM_STATIC(drv, (ops_table).live_mem);		\
UK_FS_TMPL_LIVE_GEN_GETSTAT_STATIC(drv, (ops_table).live_getstat);	\
UK_FS_TMPL_LIVE_GEN_SETSTAT_STATIC(drv, (ops_table).live_setstat);	\
UK_FS_TMPL_LIVE_GEN_CTL_STATIC(drv, (ops_table).live_ctl);		\
UK_FS_TMPL_LIVE_GEN_FILEOPS_STATIC(drv);				\
\
UK_FS_TMPL_LIVE_GEN_FS_STAT_STATIC(drv, (ops_table).live_fs_stat);	\
UK_FS_TMPL_LIVE_GEN_FS_SYNC_STATIC(drv, (ops_table).live_fs_sync);	\
UK_FS_TMPL_LIVE_GEN_FS_LOOKUP_STATIC(drv, (ops_table).live_fs_lookup,	\
				     (ops_table).live_nodekind,		\
				     (ops_table).live_release);		\
UK_FS_TMPL_LIVE_GEN_FS_MOUNT_STATIC(drv);				\
UK_FS_TMPL_LIVE_GEN_FS_REBIND_STATIC(drv);				\
UK_FS_TMPL_LIVE_GEN_FS_LISTDIR_STATIC(drv, (ops_table).live_fs_listdir);\
UK_FS_TMPL_LIVE_GEN_FS_CREATE_STATIC(drv, (ops_table).live_fs_create,	\
				     (ops_table).live_errnode,		\
				     (ops_table).live_release);		\
UK_FS_TMPL_LIVE_GEN_FS_UNLINK_STATIC(drv, (ops_table).live_fs_unlink);	\
UK_FS_TMPL_LIVE_GEN_FS_RENAME_STATIC(drv, (ops_table).live_fs_rename);	\
UK_FS_TMPL_LIVE_GEN_FS_GRAFT_STATIC(drv);				\
UK_FS_TMPL_LIVE_GEN_FS_READLINK_STATIC(drv, (ops_table).live_fs_readlink);\
UK_FS_TMPL_LIVE_GEN_FSOPS_STATIC(drv, mode_constraints);		\
\
UK_FS_TMPL_LIVE_GEN_VOPEN_STATIC(drv, (ops_table).live_vopen,		\
				 (ops_table).live_errnode,		\
				 (ops_table).live_release)


#define UK_FS_TMPL_LIVE_NODETYPE(drv) drv##_LNODETYPE

#define UK_FS_TMPL_LIVE_FILEOPS(drv) drv##_FILEOPS

#define UK_FS_TMPL_LIVE_FSOPS_REG(drv) drv##_FSOPS_REG
#define UK_FS_TMPL_LIVE_FSOPS_SYM(drv) drv##_FSOPS_SYM
#define UK_FS_TMPL_LIVE_FSOPS_DIR(drv) drv##_FSOPS_DIR
#define UK_FS_TMPL_LIVE_FSOPS_RODIR(drv) drv##_FSOPS_DIR_ROFS

/**
 * Declare the ukfile operations table of driver `drv` with extern linkage.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FILEOPS_EXTERN(drv) \
	UK_FS_TMPL_LIVE_DECL_FILEOPS_ATTR(drv, extern)

/**
 * Declare the ukfile operations table of driver `drv` with static linkage.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FILEOPS_STATIC(drv) \
	UK_FS_TMPL_LIVE_DECL_FILEOPS_ATTR(drv, static)

/**
 * Declare the ukfile operations table of driver `drv` with custom attributes.
 *
 * @param drv Driver name
 * @param attr Variable attributes
 */
#define UK_FS_TMPL_LIVE_DECL_FILEOPS_ATTR(drv, attr) \
attr const struct uk_file_ops UK_FS_TMPL_LIVE_FILEOPS(drv)

/**
 * Declare the ukfs operations tables of driver `drv` with extern linkage.
 *
 * The live driver template creates 4 ops tables for different file types:
 * - regular files
 * - symbolic links
 * - directories
 * - directories of read-only filesystem instances
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FSOPS_EXTERN(drv) \
	UK_FS_TMPL_LIVE_DECL_FSOPS_ATTR(drv, extern)

/**
 * Declare the ukfs operations tables of driver `drv` with static linkage.
 *
 * The live driver template creates 4 ops tables for different file types:
 * - regular files
 * - symbolic links
 * - directories
 * - directories of read-only filesystem instances
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FSOPS_STATIC(drv) \
	UK_FS_TMPL_LIVE_DECL_FSOPS_ATTR(drv, static)

/**
 * Declare the ukfs operations tables of driver `drv` with custom attributes.
 *
 * The live driver template creates 4 ops tables for different file types:
 * - regular files
 * - symbolic links
 * - directories
 * - directories of read-only filesystem instances
 *
 * @param drv Driver name
 * @param attr Variable attributes
 */
#define UK_FS_TMPL_LIVE_DECL_FSOPS_ATTR(drv, attr)			\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_REG(drv);		\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_SYM(drv);		\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_DIR(drv);		\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_RODIR(drv)

/**
 * Declare common live driver boilerplate as regular linkage.
 *
 * For use in a common header.
 *
 * @param drv Driver name
 * @param nodetype Type used for live node references
 */
#define UK_FS_TMPL_LIVE_DECL_COMMON(drv, nodetype)			\
	UK_FS_TMPL_LIVE_DECL_FILEOPS_EXTERN(drv);			\
	UK_FS_TMPL_LIVE_DECL_FSOPS_EXTERN(drv);				\
	UK_FS_TMPL_LIVE_DECL_COMMON_ATTR(drv, nodetype, )

/**
 * Declare common live driver boilerplate as static linkage.
 *
 * For use in single-file drivers (or other static contexts).
 *
 * @param drv Driver name
 * @param nodetype Type used for live node references
 */
#define UK_FS_TMPL_LIVE_DECL_COMMON_STATIC(drv, nodetype)		\
	UK_FS_TMPL_LIVE_DECL_FILEOPS_STATIC(drv);			\
	UK_FS_TMPL_LIVE_DECL_FSOPS_STATIC(drv);				\
	UK_FS_TMPL_LIVE_DECL_COMMON_ATTR(drv, nodetype, static)

/**
 * INTERNAL. Declare common live driver boilerplate with custom attributes.
 *
 * @param drv Driver name
 * @param nodetype Type used for live node references
 * @param attr Function attributes for non-inlines
 */
#define UK_FS_TMPL_LIVE_DECL_COMMON_ATTR(drv, nodetype, attr)		\
\
typedef nodetype UK_FS_TMPL_LIVE_NODETYPE(drv);				\
\
struct drv##_IDATA {							\
	const struct uk_file *mnt;					\
	const struct uk_file *upref;					\
	struct uk_spinlock mntlock;					\
};									\
\
struct drv##_FILE_ALLOC {						\
	UK_RB_ENTRY(drv##_FILE_ALLOC) rb_entry;				\
	struct uk_file f;						\
	struct drv##_IDATA fidata;					\
	uk_file_refcnt fref;						\
};									\
static inline								\
struct drv##_IDATA *drv##_FILE_IDATA(const struct uk_file *f)		\
{									\
	return &__containerof(f, struct drv##_FILE_ALLOC, f)->fidata;	\
}									\
\
static inline								\
UK_FS_TMPL_LIVE_NODETYPE(drv) drv##_FILE_NODE(const struct uk_file *f)	\
{									\
	return (UK_FS_TMPL_LIVE_NODETYPE(drv))(uintptr_t)f->node;	\
}									\
\
UK_RB_HEAD(drv##_FILEMAP, drv##_FILE_ALLOC);				\
\
struct drv##_ISTATE {							\
	struct drv##_FILEMAP map;					\
	struct uk_rwlock lock;						\
	unsigned long mntflags;						\
	struct uk_alloc *al;						\
	__atomic refcnt;						\
};									\
\
static inline								\
struct drv##_ISTATE *drv##_FILE_ISTATE(const struct uk_file *f)	\
{									\
	return (struct drv##_ISTATE *)f->vol;				\
}									\
\
attr struct drv##_ISTATE *drv##_ISTATE_NEW(struct uk_alloc *al);	\
attr void drv##_ISTATE_RELEASE(struct drv##_ISTATE *istate);		\
\
attr const struct uk_file *drv##_FILE_GET(struct drv##_ISTATE *s,	\
					  UK_FS_TMPL_LIVE_NODETYPE(drv) n);\
attr void drv##_FILE_SET(struct drv##_FILE_ALLOC *f);			\
attr struct drv##_FILE_ALLOC *drv##_FILE_FROMNODE(			\
	UK_FS_TMPL_LIVE_NODETYPE(drv) n, struct drv##_ISTATE *istate);	\
attr const struct uk_file *drv##_FILE_IOPEN(struct drv##_ISTATE *s,	\
					    UK_FS_TMPL_LIVE_NODETYPE(drv) n)

/**
 * Generate common live driver boilerplate with regular linkage.
 *
 * For use in dedicated source file.
 * Assumes previous use of UK_FS_TMPL_LIVE_DECL_COMMON.
 *
 * @param drv Driver name
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param live_nodekind Live `nodekind` function (see live ops above)
 * @param live_acquire Live `acquire` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 * @param live_state Live `state` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_COMMON(drv, nodecmp, live_nodekind,		\
				   live_acquire, live_release, live_state) \
	UK_FS_TMPL_LIVE_GEN_COMMON_ATTR(drv, nodecmp, live_nodekind,	\
					live_acquire, live_release,	\
					live_state, )

/**
 * Generate common live driver boilerplate with static linkage.
 *
 * For use in single-file driver.
 * Assumes previous use of UK_FS_TMPL_LIVE_DECL_COMMON_STATIC.
 *
 * @param drv Driver name
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param live_nodekind Live `nodekind` function (see live ops above)
 * @param live_acquire Live `acquire` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 * @param live_state Live `state` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_COMMON_STATIC(drv, nodecmp, live_nodekind,	\
					  live_acquire, live_release,	\
					  live_state)			\
	UK_FS_TMPL_LIVE_GEN_COMMON_ATTR(drv, nodecmp, live_nodekind,	\
					live_acquire, live_release,	\
					live_state, static)

/**
 * INTERNAL. Generate common live driver boilerplate with custom attributes.
 *
 * Assumes previous use of UK_FS_TMPL_LIVE_DECL_COMMON_* as appropriate.
 *
 * @param drv Driver name
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param live_nodekind Live `nodekind` function (see live ops above)
 * @param live_acquire Live `acquire` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 * @param live_state Live `state` function (see live ops above)
 * @param attr Attributes for "exported" functions
 */
#define UK_FS_TMPL_LIVE_GEN_COMMON_ATTR(drv, nodecmp, live_nodekind,	\
					live_acquire, live_release,	\
					live_state, attr)		\
static									\
UK_FS_TMPL_LIVE_NODETYPE(drv) drv##_FILEMAP_KEY(struct drv##_FILE_ALLOC *node)\
{									\
	return drv##_FILE_NODE(&node->f);				\
}									\
\
UK_RB_KEY_GENERATE_STATIC(drv##_FILEMAP, drv##_FILE_ALLOC, rb_entry,	\
			  nodecmp, drv##_FILEMAP_KEY);			\
\
attr struct drv##_ISTATE *drv##_ISTATE_NEW(struct uk_alloc *al)		\
{									\
	struct drv##_ISTATE *istate = uk_malloc(al, sizeof(*istate));	\
									\
	if (istate) {							\
		UK_RB_INIT(&istate->map);				\
		uk_rwlock_init(&istate->lock);				\
		istate->al = al;					\
		uk_refcount_init(&istate->refcnt, 1);			\
	}								\
	return istate;							\
}									\
\
static void drv##_ISTATE_ACQUIRE(struct drv##_ISTATE *istate)		\
{									\
	uk_refcount_acquire(&istate->refcnt);				\
}									\
\
attr void drv##_ISTATE_RELEASE(struct drv##_ISTATE *istate)		\
{									\
	if (uk_refcount_release(&istate->refcnt)) {			\
		UK_ASSERT(UK_RB_EMPTY(&istate->map));			\
		uk_free(istate->al, istate);				\
	}								\
}									\
\
attr const struct uk_file *drv##_FILE_GET(struct drv##_ISTATE *s,	\
					  UK_FS_TMPL_LIVE_NODETYPE(drv) n)\
{									\
	struct drv##_FILE_ALLOC *ret;					\
									\
	uk_rwlock_rlock(&s->lock);					\
	ret = UK_RB_FIND(drv##_FILEMAP, &s->map, n);			\
	if (ret) {							\
		const int gotref = uk_file_try_acquire(&ret->f);	\
									\
		if (unlikely(!gotref))					\
			/* We found file mid-destruction; report not found */\
			ret = NULL;					\
	}								\
	uk_rwlock_runlock(&s->lock);					\
	return ret ? &ret->f : NULL;					\
}									\
\
attr void drv##_FILE_SET(struct drv##_FILE_ALLOC *f)			\
{									\
	struct drv##_ISTATE *s = drv##_FILE_ISTATE(&f->f);		\
	struct drv##_FILE_ALLOC *prev __maybe_unused;			\
									\
	uk_rwlock_wlock(&s->lock);					\
	prev = UK_RB_INSERT(drv##_FILEMAP, &s->map, f);			\
	uk_rwlock_wunlock(&s->lock);					\
	UK_ASSERT(!prev);						\
}									\
\
static void drv##_FILE_CLR(struct drv##_ISTATE *s,			\
			   struct drv##_FILE_ALLOC *f)			\
{									\
	uk_rwlock_wlock(&s->lock);					\
	UK_RB_REMOVE(drv##_FILEMAP, &s->map, f);			\
	uk_rwlock_wunlock(&s->lock);					\
}									\
\
attr const struct uk_file *drv##_FILE_IOPEN(struct drv##_ISTATE *s,	\
					    UK_FS_TMPL_LIVE_NODETYPE(drv) n)\
{									\
	struct drv##_FILE_ALLOC *ret;					\
									\
	for (;;) {							\
		uk_rwlock_wlock(&s->lock);				\
		/* Find `n` if already opened */			\
		if ((ret = UK_RB_FIND(drv##_FILEMAP, &s->map, n))) {	\
			if (unlikely(!uk_file_try_acquire(&ret->f))) {	\
				/* We found file mid-destruction; back off */\
				uk_rwlock_wunlock(&s->lock);		\
				uk_sched_yield();			\
				continue;				\
			}						\
		/* Else try to create new file */			\
		} else if ((ret = drv##_FILE_FROMNODE(n, s))) {		\
			struct drv##_FILE_ALLOC *prev __maybe_unused;	\
									\
			prev = UK_RB_INSERT(drv##_FILEMAP, &s->map, ret);\
			UK_ASSERT(!prev);				\
		}							\
		uk_rwlock_wunlock(&s->lock);				\
		break;							\
	}								\
	return ret ? &ret->f : NULL;					\
}									\
\
static void drv##_FRELEASE(const struct uk_file *f, int what)		\
{									\
	struct drv##_ISTATE *istate = drv##_FILE_ISTATE(f);		\
	struct drv##_FILE_ALLOC *al = __containerof(f,			\
						    struct drv##_FILE_ALLOC,\
						    f);			\
	if (what & UK_FILE_RELEASE_RES) {				\
		drv##_FILE_CLR(istate, al);				\
		UK_ASSERT(!al->fidata.mnt);				\
		if (al->fidata.upref)					\
			uk_file_release(al->fidata.upref);		\
		(live_release)(drv##_FILE_NODE(f));			\
	}								\
	if (what & UK_FILE_RELEASE_OBJ) {				\
		uk_free(istate->al, al);				\
		drv##_ISTATE_RELEASE(istate);				\
	}								\
}									\
\
attr									\
struct drv##_FILE_ALLOC *drv##_FILE_FROMNODE(UK_FS_TMPL_LIVE_NODETYPE(drv) n,\
					     struct drv##_ISTATE *istate)\
{									\
	struct drv##_FILE_ALLOC *al = uk_malloc(istate->al, sizeof(*al));\
	const struct uk_fs_ops *fsops;					\
									\
	if (unlikely(!al))						\
		return NULL;						\
									\
	if ((live_nodekind)(n, UK_FS_TMPL_DIR)) {			\
		if (istate->mntflags & MS_RDONLY)			\
			fsops = &UK_FS_TMPL_LIVE_FSOPS_RODIR(drv);	\
		else							\
			fsops = &UK_FS_TMPL_LIVE_FSOPS_DIR(drv);	\
	} else if ((live_nodekind)(n, UK_FS_TMPL_SYM)) {		\
		fsops = &UK_FS_TMPL_LIVE_FSOPS_SYM(drv);		\
	} else {							\
		fsops = &UK_FS_TMPL_LIVE_FSOPS_REG(drv);		\
	}								\
	(live_acquire)(n);						\
	drv##_ISTATE_ACQUIRE(istate);					\
	al->fref = UK_FILE_REFCNT_INIT_VALUE(al->fref);			\
	al->fidata.mnt = NULL;						\
	al->fidata.upref = NULL;					\
	uk_spin_init(&al->fidata.mntlock);				\
	al->f = (struct uk_file){					\
		.vol = istate,						\
		.node = (void *)(uintptr_t)n,				\
		.ops = &UK_FS_TMPL_LIVE_FILEOPS(drv),			\
		.fsops = fsops,						\
		.refcnt = &al->fref,					\
		.state = (live_state)(n),				\
		._release = drv##_FRELEASE				\
	};								\
	return al;							\
}

/* File ops */

/* read */
#define UK_FS_TMPL_LIVE_OP_READ(drv) drv##_READ

/**
 * Declare the ukfile read operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_READ(drv)					\
ssize_t UK_FS_TMPL_LIVE_OP_READ(drv)(const struct uk_file *f,		\
				     const struct iovec *iov, size_t iovcnt,\
				     size_t off, long flags)

/**
 * Generate the ukfile read operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_read Live `read` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_READ(drv, live_read) \
	UK_FS_TMPL_LIVE_GEN_READ_ATTR(drv, live_read, )

/**
 * Generate the ukfile read operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_read Live `read` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_READ_STATIC(drv, live_read) \
	UK_FS_TMPL_LIVE_GEN_READ_ATTR(drv, live_read, static)

/**
 * INTERNAL. Generate the ukfile read operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_read Live `read` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_READ_ATTR(drv, live_read, attr)		\
attr UK_FS_TMPL_LIVE_DECL_READ(drv)					\
{									\
	return (live_read)(drv##_FILE_NODE(f), iov, iovcnt, off, flags,	\
			   drv##_FILE_ISTATE(f)->mntflags);		\
}

/* write */
#define UK_FS_TMPL_LIVE_OP_WRITE(drv) drv##_WRITE

/**
 * Declare the ukfile write operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_WRITE(drv)					\
ssize_t UK_FS_TMPL_LIVE_OP_WRITE(drv)(const struct uk_file *f,		\
				      const struct iovec *iov, size_t iovcnt,\
				      size_t off, long flags)

/**
 * Generate the ukfile write operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_write Live `write` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_WRITE(drv, live_write) \
	UK_FS_TMPL_LIVE_GEN_WRITE_ATTR(drv, live_write, )

/**
 * Generate the ukfile write operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_write Live `write` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_WRITE_STATIC(drv, live_write) \
	UK_FS_TMPL_LIVE_GEN_WRITE_ATTR(drv, live_write, static)

/**
 * INTERNAL. Generate the ukfile write operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_write Live `write` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_WRITE_ATTR(drv, live_write, attr)		\
attr UK_FS_TMPL_LIVE_DECL_WRITE(drv)					\
{									\
	return (live_write)(drv##_FILE_NODE(f), iov, iovcnt, off, flags,\
			    drv##_FILE_ISTATE(f)->mntflags);		\
}

/* mem */
#define UK_FS_TMPL_LIVE_OP_MEM(drv) drv##_MEM

/**
 * Declare the ukfile mem operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_MEM(drv) \
ssize_t UK_FS_TMPL_LIVE_OP_MEM(drv)(const struct uk_file *f,		\
				    enum uk_file_mem_op op,		\
				    size_t off, size_t len,		\
				    struct iovec *iov, size_t iovcnt)

/**
 * Generate the ukfile mem operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_mem Live `mem` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_MEM(drv, live_mem) \
	UK_FS_TMPL_LIVE_GEN_MEM_ATTR(drv, live_mem, )

/**
 * Generate the ukfile mem operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_iomem Live `mem` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_MEM_STATIC(drv, live_mem) \
	UK_FS_TMPL_LIVE_GEN_MEM_ATTR(drv, live_mem, static)

/**
 * INTERNAL. Generate the ukfile mem operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_iomem Live `mem` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_MEM_ATTR(drv, live_mem, attr)		\
attr UK_FS_TMPL_LIVE_DECL_MEM(drv)					\
{									\
	return (live_mem)(drv##_FILE_NODE(f), op, off, len,		\
			  iov, iovcnt, drv##_FILE_ISTATE(f)->mntflags);	\
}

/* getstat */
#define UK_FS_TMPL_LIVE_OP_GETSTAT(drv) drv##_GETSTAT

/**
 * Declare the ukfile getstat operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_GETSTAT(drv)				\
int UK_FS_TMPL_LIVE_OP_GETSTAT(drv)(const struct uk_file *f,		\
				    unsigned int mask,			\
				    struct uk_statx *arg)

/**
 * Generate the ukfile getstat operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_getstat Live `getstat` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_GETSTAT(drv, live_getstat) \
	UK_FS_TMPL_LIVE_GEN_GETSTAT_ATTR(drv, live_getstat, )

/**
 * Generate the ukfile getstat operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_getstat Live `getstat` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_GETSTAT_STATIC(drv, live_getstat) \
	UK_FS_TMPL_LIVE_GEN_GETSTAT_ATTR(drv, live_getstat, static)

/**
 * INTERNAL. Generate the ukfile getstat operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_getstat Live `getstat` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_GETSTAT_ATTR(drv, live_getstat, attr)	\
attr UK_FS_TMPL_LIVE_DECL_GETSTAT(drv)					\
{									\
	return (live_getstat)(drv##_FILE_NODE(f), mask, arg,		\
			      drv##_FILE_ISTATE(f)->mntflags);		\
}

/* setstat */
#define UK_FS_TMPL_LIVE_OP_SETSTAT(drv) drv##_SETSTAT

/**
 * Declare the ukfile setstat operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_SETSTAT(drv)				\
int UK_FS_TMPL_LIVE_OP_SETSTAT(drv)(const struct uk_file *f,		\
				    unsigned int mask,			\
				    const struct uk_statx *arg)

/**
 * Generate the ukfile setstat operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_setstat Live `setstat` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_SETSTAT(drv, live_setstat) \
	UK_FS_TMPL_LIVE_GEN_SETSTAT_ATTR(drv, live_setstat, )

/**
 * Generate the ukfile setstat operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_setstat Live `setstat` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_SETSTAT_STATIC(drv, live_setstat) \
	UK_FS_TMPL_LIVE_GEN_SETSTAT_ATTR(drv, live_setstat, static)

/**
 * INTERNAL. Generate the ukfile setstat operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_setstat Live `setstat` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_SETSTAT_ATTR(drv, live_setstat, attr)	\
attr UK_FS_TMPL_LIVE_DECL_SETSTAT(drv)					\
{									\
	return (live_setstat)(drv##_FILE_NODE(f), mask, arg,		\
			      drv##_FILE_ISTATE(f)->mntflags);		\
}

/* ctl */
#define UK_FS_TMPL_LIVE_OP_CTL(drv) drv##_CTL

/**
 * Declare the ukfile ctl operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_CTL(drv)					\
int UK_FS_TMPL_LIVE_OP_CTL(drv)(const struct uk_file *f, int fam, int req,\
				uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)

/**
 * Generate the ukfile ctl operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_ctl Live `ctl` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_CTL(drv, live_ctl) \
	UK_FS_TMPL_LIVE_GEN_CTL_ATTR(drv, live_ctl, )

/**
 * Generate the ukfile ctl operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_ctl Live `ctl` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_CTL_STATIC(drv, live_ctl) \
	UK_FS_TMPL_LIVE_GEN_CTL_ATTR(drv, live_ctl, static)

/**
 * INTERNAL. Generate the ukfile ctl operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_ctl Live `ctl` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_CTL_ATTR(drv, live_ctl, attr)		\
attr UK_FS_TMPL_LIVE_DECL_CTL(drv)					\
{									\
	return (live_ctl)(drv##_FILE_NODE(f), fam, req, arg1, arg2, arg3,\
			  drv##_FILE_ISTATE(f)->mntflags);		\
}

/* file ops */

/**
 * Generate the ukfile ops table of `drv` with regular linkage.
 *
 * Assumes previous declaration or definition of all ukfile operations.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FILEOPS(drv) \
	UK_FS_TMPL_LIVE_GEN_FILEOPS_ATTR(drv, )

/**
 * Generate the ukfile ops table of `drv` with static linkage.
 *
 * Assumes previous declaration or definition of all ukfile operations.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FILEOPS_STATIC(drv) \
	UK_FS_TMPL_LIVE_GEN_FILEOPS_ATTR(drv, static)

/**
 * INTERNAL. Generate the ukfile ops table of `drv` with custom attributes.
 *
 * Assumes previous declaration or definition of all ukfile operations.
 *
 * @param drv Driver name
 * @param attr Ops table attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FILEOPS_ATTR(drv, attr)			\
attr const struct uk_file_ops UK_FS_TMPL_LIVE_FILEOPS(drv) = {		\
	.read = UK_FS_TMPL_LIVE_OP_READ(drv),				\
	.write = UK_FS_TMPL_LIVE_OP_WRITE(drv),				\
	.mem = UK_FS_TMPL_LIVE_OP_MEM(drv),				\
	.getstat = UK_FS_TMPL_LIVE_OP_GETSTAT(drv),			\
	.setstat = UK_FS_TMPL_LIVE_OP_SETSTAT(drv),			\
	.ctl = UK_FS_TMPL_LIVE_OP_CTL(drv)				\
}

/* fs stat */
#define UK_FS_TMPL_LIVE_OP_FS_STAT(drv) drv##_FS_STAT

/**
 * Declare the ukfs stat operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_STAT(drv) \
int UK_FS_TMPL_LIVE_OP_FS_STAT(drv)(const struct uk_file *f, struct statfs *buf)

/**
 * Generate the ukfs stat operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_stat Live `fs_stat` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_STAT(drv, live_fs_stat) \
	UK_FS_TMPL_LIVE_GEN_FS_STAT_ATTR(drv, live_fs_stat, )

/**
 * Generate the ukfs stat operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_stat Live `fs_stat` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_STAT_STATIC(drv, live_fs_stat) \
	UK_FS_TMPL_LIVE_GEN_FS_STAT_ATTR(drv, live_fs_stat, static)

/**
 * INTERNAL. Generate the ukfs stat operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_stat Live `fs_stat` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_STAT_ATTR(drv, live_fs_stat, attr)	\
attr UK_FS_TMPL_LIVE_DECL_FS_STAT(drv)					\
{									\
	return (live_fs_stat)(drv##_FILE_NODE(f), buf);			\
}

/* fs sync */
#define UK_FS_TMPL_LIVE_OP_FS_SYNC(drv) drv##_FS_SYNC

/**
 * Declare the ukfs sync operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_SYNC(drv) \
int UK_FS_TMPL_LIVE_OP_FS_SYNC(drv)(const struct uk_file *f)

/**
 * Generate the ukfs sync operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_sync Live `fs_sync` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_SYNC(drv, live_fs_sync) \
	UK_FS_TMPL_LIVE_GEN_FS_SYNC_ATTR(drv, live_fs_sync, )

/**
 * Generate the ukfs sync operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_sync Live `fs_sync` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_SYNC_STATIC(drv, live_fs_sync) \
	UK_FS_TMPL_LIVE_GEN_FS_SYNC_ATTR(drv, live_fs_sync, static)

/**
 * INTERNAL. Generate the ukfs sync operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_sync Live `fs_sync` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_SYNC_ATTR(drv, live_fs_sync, attr)	\
attr UK_FS_TMPL_LIVE_DECL_FS_SYNC(drv)					\
{									\
	return (live_fs_sync)(drv##_FILE_NODE(f));			\
}

/* fs listdir */
#define UK_FS_TMPL_LIVE_OP_FS_LISTDIR(drv) drv##_FS_LISTDIR

/**
 * Declare the ukfs listdir operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_LISTDIR(drv)				\
ssize_t UK_FS_TMPL_LIVE_OP_FS_LISTDIR(drv)(const struct uk_file *f,	\
					   size_t *curp,		\
					   void *buf, size_t len)

/**
 * Generate the ukfs listdir operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_listdir Live `fs_listdir` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_LISTDIR(drv, live_fs_listdir) \
	UK_FS_TMPL_LIVE_GEN_FS_LISTDIR_ATTR(drv, live_fs_listdir, )

/**
 * Generate the ukfs listdir operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_listdir Live `fs_listdir` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_LISTDIR_STATIC(drv, live_fs_listdir) \
	UK_FS_TMPL_LIVE_GEN_FS_LISTDIR_ATTR(drv, live_fs_listdir, static)

/**
 * INTERNAL. Generate the ukfs listdir operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_listdir Live `fs_listdir` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_LISTDIR_ATTR(drv, live_fs_listdir, attr)	\
attr UK_FS_TMPL_LIVE_DECL_FS_LISTDIR(drv)				\
{									\
	return (live_fs_listdir)(drv##_FILE_NODE(f), curp, buf, len,	\
				 drv##_FILE_ISTATE(f)->mntflags);	\
}

/* fs unlink */
#define UK_FS_TMPL_LIVE_OP_FS_UNLINK(drv) drv##_FS_UNLINK

/**
 * Declare the ukfs unlink operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_UNLINK(drv)				\
int UK_FS_TMPL_LIVE_OP_FS_UNLINK(drv)(const struct uk_file *f,		\
				      const char *name, size_t len,	\
				      unsigned int flags)

/**
 * Generate the ukfs unlink operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_unlink Live `fs_unlink` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_UNLINK(drv, live_fs_unlink) \
	UK_FS_TMPL_LIVE_GEN_FS_UNLINK_ATTR(drv, live_fs_unlink, )

/**
 * Generate the ukfs unlink operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_unlink Live `fs_unlink` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_UNLINK_STATIC(drv, live_fs_unlink) \
	UK_FS_TMPL_LIVE_GEN_FS_UNLINK_ATTR(drv, live_fs_unlink, static)

/**
 * INTERNAL. Generate the ukfs unlink operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_unlink Live `fs_unlink` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_UNLINK_ATTR(drv, live_fs_unlink, attr)	\
attr UK_FS_TMPL_LIVE_DECL_FS_UNLINK(drv)				\
{									\
	if (unlikely((flags & UKFS_UNLINK_DIR) &&			\
		     (flags & UKFS_UNLINK_NODIR)))			\
		return -EINVAL;						\
	return (live_fs_unlink)(drv##_FILE_NODE(f), name, len, flags,	\
				drv##_FILE_ISTATE(f)->mntflags);	\
}

/* fs rename */
#define UK_FS_TMPL_LIVE_OP_FS_RENAME(drv) drv##_FS_RENAME

/**
 * Declare the ukfs rename operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_RENAME(drv)				\
int UK_FS_TMPL_LIVE_OP_FS_RENAME(drv)(const struct uk_file *f,		\
				      const char *name, size_t nlen,	\
				      const struct uk_file *dest,	\
				      const char *dname, size_t dlen,	\
				      unsigned int flags)

/**
 * Generate the ukfs rename operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_rename Live `fs_rename` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_RENAME(drv, live_fs_rename) \
	UK_FS_TMPL_LIVE_GEN_FS_RENAME_ATTR(drv, live_fs_rename, )

/**
 * Generate the ukfs rename operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_rename Live `fs_rename` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_RENAME_STATIC(drv, live_fs_rename) \
	UK_FS_TMPL_LIVE_GEN_FS_RENAME_ATTR(drv, live_fs_rename, static)

/**
 * INTERNAL. Generate the ukfs rename operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_rename Live `fs_rename` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_RENAME_ATTR(drv, live_fs_rename, attr)	\
attr UK_FS_TMPL_LIVE_DECL_FS_RENAME(drv)				\
{									\
	return (live_fs_rename)(drv##_FILE_NODE(f), name, nlen,		\
				drv##_FILE_NODE(dest), dname, dlen,	\
				flags, drv##_FILE_ISTATE(f)->mntflags);	\
}

/* fs readlink */
#define UK_FS_TMPL_LIVE_OP_FS_READLINK(drv) drv##_FS_READLINK

/**
 * Declare the ukfs readlink operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_READLINK(drv) \
struct uk_fs_path UK_FS_TMPL_LIVE_OP_FS_READLINK(drv)(const struct uk_file *f)

/**
 * Generate the ukfs readlink operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_readlink Live `fs_readlink` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_READLINK(drv, live_fs_readlink) \
	UK_FS_TMPL_LIVE_GEN_FS_READLINK_ATTR(drv, live_fs_readlink, )

/**
 * Generate the ukfs readlink operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_readlink Live `fs_readlink` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_READLINK_STATIC(drv, live_fs_readlink) \
	UK_FS_TMPL_LIVE_GEN_FS_READLINK_ATTR(drv, live_fs_readlink, static)

/**
 * INTERNAL. Generate the ukfs readlink operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_readlink Live `fs_readlink` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_READLINK_ATTR(drv, live_fs_readlink, attr)\
attr UK_FS_TMPL_LIVE_DECL_FS_READLINK(drv)				\
{									\
	return (live_fs_readlink)(drv##_FILE_NODE(f));			\
}

/* fs lookup */
#define UK_FS_TMPL_LIVE_OP_FS_LOOKUP(drv) drv##_FS_LOOKUP

/**
 * Declare the ukfs lookup operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_LOOKUP(drv)				\
int UK_FS_TMPL_LIVE_OP_FS_LOOKUP(drv)(const struct uk_file *f,		\
				      const char *path, size_t len,	\
				      unsigned int flags,		\
				      union uk_fs_lookup_out *out,	\
				      size_t *nout)

/**
 * Generate the ukfs lookup operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_lookup Live `fs_lookup` function (see live ops above)
 * @param live_nodekind Live `nodekind` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_LOOKUP(drv, live_fs_lookup,		\
				      live_nodekind, live_release)	\
	UK_FS_TMPL_LIVE_GEN_FS_LOOKUP_ATTR(drv, live_fs_lookup,		\
					   live_nodekind, live_release, )

/**
 * Generate the ukfs lookup operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_lookup Live `fs_lookup` function (see live ops above)
 * @param live_nodekind Live `nodekind` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_LOOKUP_STATIC(drv, live_fs_lookup,	\
					     live_nodekind, live_release)\
	UK_FS_TMPL_LIVE_GEN_FS_LOOKUP_ATTR(drv, live_fs_lookup,		\
					   live_nodekind, live_release,	\
					   static)

/**
 * INTERNAL. Generate the ukfs lookup operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_lookup Live `fs_lookup` function (see live ops above)
 * @param live_nodekind Live `nodekind` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_LOOKUP_ATTR(drv, live_fs_lookup,		\
					   live_nodekind, live_release,	\
					   attr)			\
attr UK_FS_TMPL_LIVE_DECL_FS_LOOKUP(drv)				\
{									\
	struct drv##_ISTATE *istate = drv##_FILE_ISTATE(f);		\
	const struct uk_file *d = f;					\
	UK_FS_TMPL_LIVE_NODETYPE(drv) n = drv##_FILE_NODE(f);		\
	size_t cur = 0;							\
	int deep = 0;							\
	int ret;							\
									\
	for (;;) {							\
		size_t prog;						\
		UK_FS_TMPL_LIVE_NODETYPE(drv) nod[2];			\
									\
		/* Check if d is mount point */				\
		if (d && !(flags & UKFS_LOOKUP_IGNMNT)) {		\
			struct drv##_IDATA *idata = drv##_FILE_IDATA(d);\
			const struct uk_file *mnt;			\
									\
			uk_spin_lock(&idata->mntlock);			\
			if ((mnt = idata->mnt))				\
				uk_file_acquire(mnt);			\
			uk_spin_unlock(&idata->mntlock);		\
			if (mnt) {					\
				if (flags & UKFS_LOOKUP_NO_MNTAUX) {	\
					if (deep)			\
						uk_file_release(d);	\
				} else {				\
					if (!deep)			\
						uk_file_acquire(d);	\
					out->aux = d;			\
				}					\
				out->target = mnt;			\
				*nout = cur;				\
				return UKFS_STOP_MNT;			\
			}						\
		}							\
		/* Skip redundant leading '/' */			\
		while (cur < len && path[cur] == '/')			\
			cur++;						\
		/* Return self if path empty */				\
		if (cur == len) {					\
			if (!d) {					\
				d = drv##_FILE_IOPEN(istate, n);	\
				if (unlikely(!d)) {			\
					ret = -ENOMEM;			\
					goto out;			\
				}					\
				(live_release)(n);			\
			} else if (!deep) {				\
				UK_ASSERT(d == f);			\
				uk_file_acquire(d);			\
			}						\
			out->target = d;				\
			return UKFS_SUCCESS;				\
		}							\
		/* d should be dir from now on */			\
		if ((d && !uk_fs_isdir(d)) ||				\
		    (!d && !(live_nodekind)(n, UK_FS_TMPL_DIR))) {	\
			/* Cleanup & err out */				\
			*nout = cur;					\
			ret = UKFS_STOP_END;				\
			goto out;					\
		}							\
		/* Skip '.' */						\
		if (uk_fs_path_isdot(&path[cur], len - cur)) {		\
			cur += 1 + (cur + 1 != len);			\
			continue;					\
		}							\
		/* Handle '..' specially for live files */		\
		if (d) {						\
			const struct uk_file *upref =			\
				drv##_FILE_IDATA(d)->upref;		\
									\
			if ((deep || upref) &&				\
			    uk_fs_path_isdotdot(&path[cur], len - cur)) {\
				/* Stop deep lookup of '..' */		\
				if (deep) {				\
					out->target = d;		\
					*nout = cur;			\
					return UKFS_STOP_NOD;		\
				}					\
				/* Handle '..' on mount point */	\
				if (upref) {				\
					uk_file_acquire(upref);		\
					if (flags & UKFS_LOOKUP_NO_MNTAUX) {\
						if (deep)		\
							uk_file_release(d);\
					} else {			\
						if (!deep)		\
							uk_file_acquire(d);\
						out->aux = d;		\
					}				\
					out->target = upref;		\
					*nout = cur;			\
					return UKFS_STOP_MNT;		\
				}					\
			}						\
		}							\
		/* Do node lookup */					\
		ret = (live_fs_lookup)(n, &path[cur], len - cur,	\
				       nod, out, &prog);		\
		/* Interpret output */					\
		switch (ret) {						\
		case UKFS_SUCCESS:					\
			prog = len - cur;				\
			__fallthrough;					\
		case UKFS_STOP_NOD:					\
			/* Return if no progress is made */		\
			if (!prog) {					\
				UK_ASSERT(nod[0] == n);			\
				(live_release)(nod[0]);			\
				if (!d) {				\
					d = drv##_FILE_IOPEN(istate, n);\
					if (unlikely(!d)) {		\
						ret = -ENOMEM;		\
						goto out;		\
					}				\
					(live_release)(n);		\
				} else if (!deep) {			\
					uk_file_acquire(d);		\
				}					\
				out->target = d;			\
				*nout = cur;				\
				return UKFS_STOP_NOD;			\
			}						\
			/* Progress made, continue lookup */		\
			if (!d)						\
				(live_release)(n);			\
			else if (deep)					\
				uk_file_release(d);			\
			n = nod[0];					\
			d = drv##_FILE_GET(istate, n);			\
			if (d)						\
				(live_release)(n);			\
			cur += prog;					\
			break;						\
									\
		case UKFS_STOP_SYM: /* Convert & return nod[] */	\
		{							\
			const struct uk_file *pf, *tf;			\
									\
			if (!(flags & UKFS_LOOKUP_NO_SYMAUX)) {		\
				/* Convert & return symlink parent */	\
				if (nod[1] == n && d) {			\
					uk_file_acquire(d);		\
					pf = d;				\
				} else {				\
					pf = drv##_FILE_IOPEN(istate, nod[1]);\
				}					\
				(live_release)(nod[1]);			\
				if (unlikely(!pf)) {			\
					(live_release)(nod[0]);		\
					ret = -ENOMEM;			\
					goto out;			\
				}					\
			} else {					\
				/* No parent needed */			\
				(live_release)(nod[1]);			\
				pf = NULL;				\
			}						\
			/* Convert & return symlink node */		\
			tf = drv##_FILE_IOPEN(istate, nod[0]);		\
			(live_release)(nod[0]);				\
			if (unlikely(!tf)) {				\
				if (pf)					\
					uk_file_release(pf);		\
				ret = -ENOMEM;				\
				goto out;				\
			}						\
			/* Writeout */					\
			if (pf)						\
				out->aux = pf;				\
			out->target = tf;				\
			*nout = cur + prog;				\
			goto out;					\
		}							\
									\
		case UKFS_STOP_SPEC:					\
		case UKFS_STOP_FILE: /* Live driver wrote output already */\
		case UKFS_STOP_END: /* Propagate */			\
			*nout = cur + prog;				\
			goto out;					\
									\
		case UKFS_STOP_MNT: /* Cannot happen */			\
			UK_CRASH("Invalid lookup return from live driver\n");\
		default: /* assert < 0; cleanup & return err */		\
			UK_ASSERT(ret < 0);				\
			goto out;					\
		}							\
		deep = 1;						\
	}								\
out:									\
	if (!d)								\
		(live_release)(n);					\
	else if (deep)							\
		uk_file_release(d);					\
	return ret;							\
}

/* fs create */
#define UK_FS_TMPL_LIVE_OP_FS_CREATE(drv) drv##_FS_CREATE

/**
 * Declare the ukfs create operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_CREATE(drv)				\
const struct uk_file *UK_FS_TMPL_LIVE_OP_FS_CREATE(drv)(		\
	const struct uk_file *f, const char *name, size_t len,		\
	unsigned int mode, int flags, union uk_fs_create_target target)

/**
 * Generate the ukfs create operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_create Live `fs_create` function (see live ops above)
 * @param live_errnode Live `errnode` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_CREATE(drv, live_fs_create,		\
				      live_errnode, live_release)	\
	UK_FS_TMPL_LIVE_GEN_FS_CREATE_ATTR(drv, live_fs_create,		\
					   live_errnode, live_release, )

/**
 * Generate the ukfs create operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_create Live `fs_create` function (see live ops above)
 * @param live_errnode Live `errnode` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_FS_CREATE_STATIC(drv, live_fs_create,	\
					     live_errnode, live_release)\
	UK_FS_TMPL_LIVE_GEN_FS_CREATE_ATTR(drv, live_fs_create,		\
					   live_errnode, live_release,	\
					   static)

/**
 * INTERNAL. Generate the ukfs create operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_fs_create Live `fs_create` function (see live ops above)
 * @param live_errnode Live `errnode` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_CREATE_ATTR(drv, live_fs_create,		\
					   live_errnode, live_release,	\
					   attr)			\
attr UK_FS_TMPL_LIVE_DECL_FS_CREATE(drv)				\
{									\
	const unsigned int ftype = mode & S_IFMT;			\
	int err;							\
	UK_FS_TMPL_LIVE_NODETYPE(drv) rnode;				\
	union UK_FS_TMPL_LIVE_CREATE_TARGET(drv) live_target;		\
	const struct uk_file *ret;					\
	struct drv##_ISTATE *istate = drv##_FILE_ISTATE(f);		\
									\
	switch (ftype) {						\
	case S_IFMT:							\
		/* If hardlink, convert target to nodetype */		\
		if (unlikely(target.file->vol != f->vol))		\
			return ERR2PTR(-EXDEV);				\
		live_target.livenode = target.file->node;		\
		break;							\
	case 0:								\
		/* If pseudo-file linking, pass verbatim */		\
		live_target.ukfs.file = target.file;			\
		break;							\
	case S_IFLNK:							\
		live_target.ukfs.path = target.path;			\
		break;							\
	case S_IFCHR:							\
	case S_IFBLK:							\
	case S_IFSOCK:							\
	case S_IFIFO:							\
		live_target.ukfs.special = target.special;		\
		break;							\
	default:							\
		live_target.ukfs = UKFS_NOTARGET;			\
	}								\
									\
	rnode = (live_fs_create)(drv##_FILE_NODE(f), name, len, mode,	\
				 flags, live_target, istate->mntflags);	\
	if (unlikely((err = (live_errnode)(rnode))))			\
		return ERR2PTR(err);					\
	switch (ftype) {						\
	case 0:								\
	case S_IFCHR:							\
	case S_IFBLK:							\
	case S_IFSOCK:							\
	case S_IFIFO:							\
		/* NULL is returned on success */			\
		return NULL;						\
	default:							\
		/* Convert rnode to ukfile & return */			\
		ret = drv##_FILE_IOPEN(istate, rnode);			\
		(live_release)(rnode);					\
		if (unlikely(!ret))					\
			return ERR2PTR(-ENOMEM);			\
		return ret;						\
	}								\
}

/* fs mount */
#define UK_FS_TMPL_LIVE_OP_FS_MOUNT(drv) drv##_FS_MOUNT

/**
 * Declare the ukfs mount operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_MOUNT(drv)				\
int UK_FS_TMPL_LIVE_OP_FS_MOUNT(drv)(const struct uk_file *f,		\
				     const struct uk_file **target)

/**
 * Generate the ukfs mount operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FS_MOUNT(drv) \
	UK_FS_TMPL_LIVE_GEN_FS_MOUNT_ATTR(drv, )

/**
 * Generate the ukfs mount operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FS_MOUNT_STATIC(drv) \
	UK_FS_TMPL_LIVE_GEN_FS_MOUNT_ATTR(drv, static)

/**
 * INTERNAL. Generate the ukfs mount operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_MOUNT_ATTR(drv, attr)			\
attr UK_FS_TMPL_LIVE_DECL_FS_MOUNT(drv)					\
{									\
	struct drv##_IDATA *idata = drv##_FILE_IDATA(f);		\
	const struct uk_file *t = *target;				\
									\
	if (t) {							\
		const struct uk_file *prev = NULL;			\
									\
		if (unlikely(!uk_compare_exchange_n(&idata->mnt, &prev, t)))\
			return -EEXIST;					\
		uk_file_acquire(t);					\
	} else {							\
		uk_spin_lock(&idata->mntlock);				\
		t = uk_exchange_n(&idata->mnt, NULL);			\
		uk_spin_unlock(&idata->mntlock);			\
		if (unlikely(!t))					\
			return -ENOENT;					\
		*target = t;						\
	}								\
	return 0;							\
}

/* fs graft */
#define UK_FS_TMPL_LIVE_OP_FS_GRAFT(drv) drv##_FS_GRAFT

/**
 * Declare the ukfs graft operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_GRAFT(drv)				\
int UK_FS_TMPL_LIVE_OP_FS_GRAFT(drv)(const struct uk_file *f,		\
				     const struct uk_file *ref)

/**
 * Generate the ukfs graft operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FS_GRAFT(drv) \
	UK_FS_TMPL_LIVE_GEN_FS_GRAFT_ATTR(drv, )

/**
 * Generate the ukfs graft operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FS_GRAFT_STATIC(drv) \
	UK_FS_TMPL_LIVE_GEN_FS_GRAFT_ATTR(drv, static)

/**
 * INTERNAL. Generate the ukfs graft operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_GRAFT_ATTR(drv, attr)			\
attr UK_FS_TMPL_LIVE_DECL_FS_GRAFT(drv)					\
{									\
	struct drv##_IDATA *idata = drv##_FILE_IDATA(f);		\
									\
	UK_ASSERT(!idata->upref);					\
	uk_file_acquire(ref);						\
	idata->upref = ref;						\
	return 0;							\
}

/* fs rebind */
#define UK_FS_TMPL_LIVE_OP_FS_REBIND(drv) drv##_FS_REBIND

/**
 * Declare the ukfs rebind operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_FS_REBIND(drv)				\
const struct uk_file *UK_FS_TMPL_LIVE_OP_FS_REBIND(drv)(		\
	const struct uk_file *f, unsigned long flags,			\
	const void *data __unused)

/**
 * Generate the ukfs rebind operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FS_REBIND(drv) \
	UK_FS_TMPL_LIVE_GEN_FS_REBIND_ATTR(drv, )

/**
 * Generate the ukfs rebind operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_GEN_FS_REBIND_STATIC(drv) \
	UK_FS_TMPL_LIVE_GEN_FS_REBIND_ATTR(drv, static)

/**
 * INTERNAL. Generate the ukfs rebind operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FS_REBIND_ATTR(drv, attr)			\
attr UK_FS_TMPL_LIVE_DECL_FS_REBIND(drv)				\
{									\
	UK_FS_TMPL_LIVE_NODETYPE(drv) node = drv##_FILE_NODE(f);	\
	struct drv##_ISTATE *istate = drv##_FILE_ISTATE(f);		\
	struct drv##_ISTATE *newstate = drv##_ISTATE_NEW(istate->al);	\
	struct drv##_FILE_ALLOC *newf;					\
									\
	if (unlikely(!newstate))					\
		return ERR2PTR(-ENOMEM);				\
	newstate->mntflags = flags;					\
	newf = drv##_FILE_FROMNODE(node, newstate);			\
	drv##_ISTATE_RELEASE(newstate);					\
	if (unlikely(!newf))						\
		return ERR2PTR(-ENOMEM);				\
	drv##_FILE_SET(newf);						\
	return &newf->f;						\
}

/* fsops (reg, sym, dir, rodir) */

/**
 * Generate the ukfs ops tables of `drv` with regular linkage.
 *
 * Assumes previous declaration or definition of all ukfs operations.
 *
 * @param drv Driver name
 * @param mode_constraints Open file mode constraints
 */
#define UK_FS_TMPL_LIVE_GEN_FSOPS(drv, mode_constraints) \
	UK_FS_TMPL_LIVE_GEN_FSOPS_ATTR(drv, mode_constraints, )

/**
 * Generate the ukfs ops tables of `drv` with static linkage.
 *
 * Assumes previous declaration or definition of all ukfs operations.
 *
 * @param drv Driver name
 * @param mode_constraints Open file mode constraints
 */
#define UK_FS_TMPL_LIVE_GEN_FSOPS_STATIC(drv, mode_constraints) \
	UK_FS_TMPL_LIVE_GEN_FSOPS_ATTR(drv, mode_constraints, static)

/**
 * INTERNAL. Generate the ukfs ops tables of `drv` with custom attributes.
 *
 * Assumes previous declaration or definition of all ukfs operations.
 *
 * @param drv Driver name
 * @param mode_constraints Open file mode constraints
 * @param attr Ops table attributes
 */
#define UK_FS_TMPL_LIVE_GEN_FSOPS_ATTR(drv, mode_constraints, attr)	\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_REG(drv) = {		\
	.lookup = UK_FS_TMPL_LIVE_OP_FS_LOOKUP(drv),			\
	.readlink = NULL,						\
	.listdir = NULL,						\
	.create = NULL,							\
	.unlink = NULL,							\
	.rename = NULL,							\
	.graft = NULL,							\
	.mount = UK_FS_TMPL_LIVE_OP_FS_MOUNT(drv),			\
	.rebind = UK_FS_TMPL_LIVE_OP_FS_REBIND(drv),			\
	.stat = UK_FS_TMPL_LIVE_OP_FS_STAT(drv),			\
	.sync = UK_FS_TMPL_LIVE_OP_FS_SYNC(drv),			\
	.constraints = (mode_constraints)				\
};									\
\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_SYM(drv) = {		\
	.lookup = UK_FS_TMPL_LIVE_OP_FS_LOOKUP(drv),			\
	.readlink = UK_FS_TMPL_LIVE_OP_FS_READLINK(drv),		\
	.listdir = NULL,						\
	.create = NULL,							\
	.unlink = NULL,							\
	.rename = NULL,							\
	.graft = NULL,							\
	.mount = UK_FS_TMPL_LIVE_OP_FS_MOUNT(drv),			\
	.rebind = UK_FS_TMPL_LIVE_OP_FS_REBIND(drv),			\
	.stat = UK_FS_TMPL_LIVE_OP_FS_STAT(drv),			\
	.sync = UK_FS_TMPL_LIVE_OP_FS_SYNC(drv),			\
	.constraints = (mode_constraints)				\
};									\
\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_DIR(drv) = {		\
	.lookup = UK_FS_TMPL_LIVE_OP_FS_LOOKUP(drv),			\
	.readlink = NULL,						\
	.listdir = UK_FS_TMPL_LIVE_OP_FS_LISTDIR(drv),			\
	.create = UK_FS_TMPL_LIVE_OP_FS_CREATE(drv),			\
	.unlink = UK_FS_TMPL_LIVE_OP_FS_UNLINK(drv),			\
	.rename = UK_FS_TMPL_LIVE_OP_FS_RENAME(drv),			\
	.graft = UK_FS_TMPL_LIVE_OP_FS_GRAFT(drv),			\
	.mount = UK_FS_TMPL_LIVE_OP_FS_MOUNT(drv),			\
	.rebind = UK_FS_TMPL_LIVE_OP_FS_REBIND(drv),			\
	.stat = UK_FS_TMPL_LIVE_OP_FS_STAT(drv),			\
	.sync = UK_FS_TMPL_LIVE_OP_FS_SYNC(drv),			\
	.constraints = (mode_constraints)				\
};									\
\
attr const struct uk_fs_ops UK_FS_TMPL_LIVE_FSOPS_RODIR(drv) = {	\
	.lookup = UK_FS_TMPL_LIVE_OP_FS_LOOKUP(drv),			\
	.readlink = NULL,						\
	.listdir = UK_FS_TMPL_LIVE_OP_FS_LISTDIR(drv),			\
	.create = uk_fs_common_rofs_create,				\
	.unlink = uk_fs_common_rofs_unlink,				\
	.rename = uk_fs_common_rofs_rename,				\
	.graft = UK_FS_TMPL_LIVE_OP_FS_GRAFT(drv),			\
	.mount = UK_FS_TMPL_LIVE_OP_FS_MOUNT(drv),			\
	.rebind = UK_FS_TMPL_LIVE_OP_FS_REBIND(drv),			\
	.stat = UK_FS_TMPL_LIVE_OP_FS_STAT(drv),			\
	.sync = UK_FS_TMPL_LIVE_OP_FS_SYNC(drv),			\
	.constraints = (mode_constraints)				\
}

/* ukfs vopen */
#define UK_FS_TMPL_LIVE_OP_VOPEN(drv) drv##_VOPEN

/**
 * Declare the ukfs vopen operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_LIVE_DECL_VOPEN(drv)					\
const struct uk_file *UK_FS_TMPL_LIVE_OP_VOPEN(drv)(			\
	union uk_fs_vopen_vol vol, unsigned long flags,			\
	union uk_fs_vopen_data data, size_t fmt)

/**
 * Generate the ukfs vopen operation with regular linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_vopen Live `vopen` function (see live ops above)
 * @param live_errnode Live `errnode` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_VOPEN(drv, live_vopen, live_errnode, live_release) \
	UK_FS_TMPL_LIVE_GEN_VOPEN_ATTR(drv, live_vopen, live_errnode,	\
				       live_release, )

/**
 * Generate the ukfs vopen operation with static linkage.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_vopen Live `vopen` function (see live ops above)
 * @param live_errnode Live `errnode` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 */
#define UK_FS_TMPL_LIVE_GEN_VOPEN_STATIC(drv, live_vopen,		\
					 live_errnode, live_release)	\
	UK_FS_TMPL_LIVE_GEN_VOPEN_ATTR(drv, live_vopen, live_errnode,	\
				       live_release, static)

/**
 * INTERNAL. Generate the ukfs vopen operation with custom attributes.
 *
 * Assumes previous declarations of common live boilerplate.
 *
 * @param drv Driver name
 * @param live_vopen Live `vopen` function (see live ops above)
 * @param live_errnode Live `errnode` function (see live ops above)
 * @param live_release Live `release` function (see live ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_LIVE_GEN_VOPEN_ATTR(drv, live_vopen, live_errnode,	\
				       live_release, attr)		\
attr UK_FS_TMPL_LIVE_DECL_VOPEN(drv)					\
{									\
	int err;							\
	struct drv##_ISTATE *istate;					\
	const struct uk_file *ret;					\
	UK_FS_TMPL_LIVE_NODETYPE(drv) rootnode;				\
									\
	rootnode = (live_vopen)(vol, flags, data, fmt);			\
	if (unlikely((err = (live_errnode)(rootnode))))			\
		return ERR2PTR(err);					\
									\
	istate = drv##_ISTATE_NEW(uk_alloc_get_default());		\
	if (istate) {							\
		istate->mntflags = flags;				\
		ret = drv##_FILE_IOPEN(istate, rootnode);		\
		drv##_ISTATE_RELEASE(istate);				\
		if (!ret)						\
			ret = ERR2PTR(-ENOMEM);				\
	} else {							\
		ret = ERR2PTR(-ENOMEM);					\
	}								\
	(live_release)(rootnode);					\
	return ret;							\
}

#endif /* __UKFS_FS_TEMPLATE_LIVE_H__ */
