/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Unikraft filesystem template for node drivers.
 *
 * This header file contains boilerplate code for implementing a Unikraft
 * filesystem driver based on code that natively handles raw references to
 * filesystem nodes.
 * This functionality is provided in the form of code generation macros that
 * define functions conforming to the ukfs live driver template API defined in
 * `uk/fs/template/live.h`. These functions are then fed through the live driver
 * template to generate the final ukfile & ukfs operations.
 *
 * A raw reference to a filesystem node is understood to be a pair of
 * identifiers (dev, node), of driver-specific types, that together uniquely
 * identify a filesystem node in relationship to a "device". A node reference
 * does not track any runtime state such as refcounting, file state, mount point
 * status, or filesystem instances.
 * A node driver is solely responsible for implementing operations focused on
 * filesystem contents, and for signaling events (if so required).
 *
 * Limited support for tracking device and node lifetimes is provided.
 * Devices are created, opened, and returned by vopen() ready to perform
 * operations, with vclose() called when finished.
 * The root node is returned by vopen(), with all other nodes returned by calls
 * to lookup() or create(). Nodes that compare equal refer to the same
 * filesystem object until explicitly closed or forgotten.
 * A node may be "unopened" -- can only perform lookup() -- or "open" -- can
 * perform all operations.
 * A device's open nodes are guaranteed to be closed before `vclose()`; any
 * unopened nodes are implicitly forgotten by `vclose()`.
 *
 * When returning the first occurrence of a node:
 * - vopen() and lookup() return unopened nodes
 * - create() returns open nodes
 * When returning nodes previously output but yet to be closed/forgotten:
 * - lookup() returns nodes as-is
 * - create() ensures nodes are open before return
 *
 * The node driver provides callbacks for node lifetime:
 * - nopen() -- called when opening a node
 * - nclose() -- called on open nodes; the node shall not be referenced until
 *               again returned by the node driver
 * - nforget() -- called on unopened nodes; the node shall not be referenced
 *                until again returned by the node driver
 * Any of these is called at most once for a node value. Multiple "copies",
 * obtained from lookup() or create(), reference a singular object that will be
 * opened or cleaned up exactly once.
 *
 * Thus, a node may go through one of only 3 lifecycles:
 * - create() -> [ops] -> nclose()
 * - lookup()/vopen() -> [lookups] -> nforget()
 * - lookup()/vopen() -> [lookups] -> nopen() -> [ops] -> nclose()
 */

#ifndef __UKFS_FS_TEMPLATE_NODE_H__
#define __UKFS_FS_TEMPLATE_NODE_H__

#include <uk/fs/template/live.h>
#include <uk/mutex.h>

/* Driver-specific types */

#define UK_FS_TMPL_NODE_OPENDEV(drv) drv##_OPENDEV
#define UK_FS_TMPL_NODE_CREATE_TARGET(drv) drv##_NODE_CREATE_TARGET

/**
 * Define the specific types of node driver `drv` with nodes represented by
 * (devtype, nodetype).
 *
 * @param drv Driver name
 * @param devtype Type used for device identifiers
 * @param nodetype Type used for filesystem node identifiers
 */
#define UK_FS_TMPL_NODE_TYPES(drv, devtype, nodetype)			\
/* Result of opening a volume */					\
struct UK_FS_TMPL_NODE_OPENDEV(drv) {					\
	devtype dev; /* Device identifier */				\
	nodetype rootnode; /* Identifier of root filesystem node */	\
};									\
/* Target argument type for node create operation */			\
union UK_FS_TMPL_NODE_CREATE_TARGET(drv) {				\
	nodetype node;							\
	union uk_fs_create_target ukfs;					\
}

/* Node ops declarations
 *
 * These operations are implemented by the node driver and supplied to the
 * appropriate code generation macro(s). In broad terms, these map to
 * ukfile/ukfs operations, but with filesystem nodes referenced by a pair of
 * (dev, node) identifiers. Additionally, the node driver must supply a set of
 * "glue" ops that help the template code interpret and manage node references.
 *
 * The extra argument `mntflags`, when present, contains the standard mount
 * flags active on the filesystem instance that the request is coming from.
 *
 * Live ops that return a `nodetype` should signal error conditions by special
 * values of `nodetype` that are mapped to negative error codes by
 * `node_errnode`.
 * On success, lookup returns nodes unopened, while create returns them open.
 *
 * For best optimizaton, these functions should be implemented statically in
 * the same compilation unit as the UK_FS_TMPL_NODE_GEN_* macro that uses them.
 */

/**
 * Define the node ops function types for driver `drv` with nodes represented by
 * (devtype, nodetype).
 *
 * Requires previous use of `UK_FS_TMPL_NODE_TYPES(drv, devtype, nodetype)`.
 *
 * @param drv Driver name
 * @param devtype Type used for device identifiers
 * @param nodetype Type used for filesystem node identifiers
 */
#define UK_FS_TMPL_NODE_OPS(drv, devtype, nodetype)			\
UK_FS_TMPL_NODE_OPS_COMMON(drv, devtype, nodetype);			\
UK_FS_TMPL_NODE_OPS_POLL(drv, devtype, nodetype)

#define UK_FS_TMPL_NODE_OPS_COMMON(drv, devtype, nodetype)		\
/* Similar to ukfs vopen; return a (dev, rootnode) identifier pair. */	\
/* Device should be returned open, rootnode unopened. */		\
typedef struct UK_FS_TMPL_NODE_OPENDEV(drv) (*drv##NODEOP_VOPEN)(	\
	union uk_fs_vopen_vol vol, unsigned long flags,			\
	union uk_fs_vopen_data data, size_t fmt);			\
\
/* Lifetime management */						\
/* Called to signal closing of a device */				\
typedef void (*drv##NODEOP_VCLOSE)(devtype dev);			\
/* Called to open a node, signaling potential future op calls */	\
typedef int (*drv##NODEOP_NOPEN)(devtype dev, nodetype n,		\
				 struct uk_file_state *st);		\
/* Called to signal closing a node */					\
typedef void (*drv##NODEOP_NCLOSE)(devtype dev, nodetype n);		\
/* Called to signal a node shall no longer be referenced */		\
typedef void (*drv##NODEOP_NFORGET)(devtype dev, nodetype n);		\
\
/* Standard ukfile operations */					\
typedef ssize_t (*drv##NODEOP_IO)(devtype dev, nodetype n,		\
				  const struct iovec *iov, size_t iovcnt,\
				  size_t off, long flags,		\
				  unsigned long mntflags);		\
typedef ssize_t (*drv##NODEOP_MEM)(devtype dev, nodetype n,		\
				   enum uk_file_mem_op op,		\
				   size_t off, size_t len,		\
				   struct iovec *iov, size_t iovcnt,	\
				   unsigned long mntflags);		\
typedef int (*drv##NODEOP_GETSTAT)(devtype dev, nodetype n,		\
				   unsigned int mask,			\
				   struct uk_statx *arg,		\
				   unsigned long mntflags);		\
typedef int (*drv##NODEOP_SETSTAT)(devtype dev, nodetype n,		\
				   unsigned int mask,			\
				   const struct uk_statx *arg,		\
				   unsigned long mntflags);		\
typedef int (*drv##NODEOP_CTL)(devtype dev, nodetype n,			\
			       int fam, int req, uintptr_t arg1,	\
			       uintptr_t arg2, uintptr_t arg3,		\
			       unsigned long mntflags);			\
\
/* Standard ukfs operations */						\
typedef int (*drv##NODEOP_FS_STAT)(devtype dev, struct statfs *buf);	\
typedef int (*drv##NODEOP_FS_SYNC)(devtype dev);			\
typedef int (*drv##NODEOP_FS_LOOKUP)(devtype dev, nodetype n,		\
				     const char *path, size_t len,	\
				     nodetype *out_nodes,		\
				     union uk_fs_lookup_out *out_ukfs,	\
				     size_t *nout);			\
typedef ssize_t (*drv##NODEOP_FS_LISTDIR)(devtype dev, nodetype n,	\
					  size_t *curp,			\
					  void *buf, size_t len,	\
					  unsigned long mntflags);	\
typedef nodetype (*drv##NODEOP_FS_CREATE)(devtype dev, nodetype n,	\
	const char *name, size_t len, unsigned int mode, int flags,	\
	union UK_FS_TMPL_NODE_CREATE_TARGET(drv) target,		\
	unsigned long mntflags);					\
typedef int (*drv##NODEOP_FS_UNLINK)(devtype dev, nodetype n,		\
				     const char *name, size_t len,	\
				     unsigned int flags,		\
				     unsigned long mntflags);		\
typedef int (*drv##NODEOP_FS_RENAME)(devtype dev, nodetype n,		\
				     const char *name, size_t nlen,	\
				     nodetype dest,			\
				     const char *dname, size_t dlen,	\
				     unsigned int flags,		\
				     unsigned long mntflags);		\
typedef struct uk_fs_path (*drv##NODEOP_FS_READLINK)(devtype dev, nodetype n);\
\
/* Glue ops */								\
typedef int (*drv##NODEOP_NKIND)(devtype dev, nodetype n,		\
				 enum uk_fs_tmpl_node_kind kind);	\
typedef int (*drv##NODEOP_ERRNODE)(const nodetype n)

/* If the current build support explicitly polled files, allow for drivers
 * to specify a node_poll operation that is called to get events from files.
 * This is currently the only way node drivers can signal events.
 *
 * If explicitly polled file support is disabled, or if node_poll is NULL,
 * the template code gracefully falls back to no event support.
 */
#if CONFIG_LIBUKFILE_POLLED

#define UK_FS_TMPL_NODE_OPS_POLL(drv, devtype, nodetype)		\
typedef unsigned int (*drv##NODEOP_POLL)(devtype dev, nodetype n,	\
					 unsigned int mask)

#else /* !CONFIG_LIBUKFILE_POLLED */

#define UK_FS_TMPL_NODE_OPS_POLL(drv, devtype, nodetype)

#endif /* !CONFIG_LIBUKFILE_POLLED */

/**
 * Define `struct sname` as node ops table for driver `drv`.
 *
 * For use by single-file drivers with `UK_FS_TMPL_NODE_GENERATE_STATIC`.
 *
 * Requires previous use of `UK_FS_TMPL_NODE_OPS(drv, *)`.
 *
 * @param drv Driver name
 * @param sname Name of struct to define
 */
#if CONFIG_LIBUKFILE_POLLED

#define UK_FS_TMPL_NODE_OPSTABLE(drv, sname)				\
struct sname {								\
	/* Driver ops */						\
	drv##NODEOP_VOPEN node_vopen;					\
	drv##NODEOP_VCLOSE node_vclose;					\
	drv##NODEOP_NOPEN node_nopen;					\
	drv##NODEOP_NCLOSE node_nclose;					\
	drv##NODEOP_NFORGET node_nforget;				\
	/* File ops */							\
	drv##NODEOP_IO node_read;					\
	drv##NODEOP_IO node_write;					\
	drv##NODEOP_MEM node_mem;					\
	drv##NODEOP_GETSTAT node_getstat;				\
	drv##NODEOP_SETSTAT node_setstat;				\
	drv##NODEOP_CTL node_ctl;					\
	/* FS ops */							\
	drv##NODEOP_FS_STAT node_fs_stat;				\
	drv##NODEOP_FS_SYNC node_fs_sync;				\
	drv##NODEOP_FS_LOOKUP node_fs_lookup;				\
	drv##NODEOP_FS_LISTDIR node_fs_listdir;				\
	drv##NODEOP_FS_CREATE node_fs_create;				\
	drv##NODEOP_FS_UNLINK node_fs_unlink;				\
	drv##NODEOP_FS_RENAME node_fs_rename;				\
	drv##NODEOP_FS_READLINK node_fs_readlink;			\
	/* Glue ops */							\
	drv##NODEOP_NKIND node_nodekind;				\
	drv##NODEOP_POLL node_poll;					\
	drv##NODEOP_ERRNODE node_errnode;				\
}

#define UK_FS_TMPL_NODE_OPS_POLLOP(ops) ((ops).node_poll)

#else /* !CONFIG_LIBUKFILE_POLLED */

#define UK_FS_TMPL_NODE_OPSTABLE(drv, sname)				\
struct sname {								\
	/* Driver ops */						\
	drv##NODEOP_VOPEN node_vopen;					\
	drv##NODEOP_VCLOSE node_vclose;					\
	drv##NODEOP_NOPEN node_nopen;					\
	drv##NODEOP_NCLOSE node_nclose;					\
	drv##NODEOP_NFORGET node_nforget;				\
	/* File ops */							\
	drv##NODEOP_IO node_read;					\
	drv##NODEOP_IO node_write;					\
	drv##NODEOP_MEM node_mem;					\
	drv##NODEOP_GETSTAT node_getstat;				\
	drv##NODEOP_SETSTAT node_setstat;				\
	drv##NODEOP_CTL node_ctl;					\
	/* FS ops */							\
	drv##NODEOP_FS_STAT node_fs_stat;				\
	drv##NODEOP_FS_SYNC node_fs_sync;				\
	drv##NODEOP_FS_LOOKUP node_fs_lookup;				\
	drv##NODEOP_FS_LISTDIR node_fs_listdir;				\
	drv##NODEOP_FS_CREATE node_fs_create;				\
	drv##NODEOP_FS_UNLINK node_fs_unlink;				\
	drv##NODEOP_FS_RENAME node_fs_rename;				\
	drv##NODEOP_FS_READLINK node_fs_readlink;			\
	/* Glue ops */							\
	drv##NODEOP_NKIND node_nodekind;				\
	drv##NODEOP_ERRNODE node_errnode;				\
}

#define UK_FS_TMPL_NODE_OPS_POLLOP(ops) NULL

#endif /* !CONFIG_LIBUKFILE_POLLED */

/* Macros for exported names of template-generated functions and data */

/* Node driver device identifier type */
#define UK_FS_TMPL_NODE_DEVTYPE(drv) drv##_DEVTYPE
/* Node driver node identifier type */
#define UK_FS_TMPL_NODE_NODETYPE(drv) drv##_NNODETYPE
/* Intermediate live driver node type */
#define UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) struct drv##_RSTATE *
/* Intermediate live driver node comparison function */
#define UK_FS_TMPL_NODE_LIVE_NODECMP(drv) drv##_RSTATE_CMP
/* Live operations table for a static intermediate live driver */
#define UK_FS_TMPL_NODE_LIVE_OPSTABLE(drv) drv##_LIVEOPS
/* Final driver vopen function */
#define UK_FS_TMPL_NODE_OP_VOPEN(drv) UK_FS_TMPL_LIVE_OP_VOPEN(drv)

/* Code generation
 *
 * The node driver template offers flexibility in code organization and
 * generation, allowing implementations to mix and match.
 */

/**
 * Generate all driver code as static functions for single-file drivers.
 *
 * Outputs `UK_FS_TMPL_NODE_LIVE_OPTSTABLE(drv)` usable with the live template.
 *
 * @param drv Driver name
 * @param devtype Type used for device identifiers
 * @param nodetype Type used for filesystem node identifiers
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param ops_table Table of node operations (see node ops above)
 * @param mode_constraints Open file mode constraints
 */
#define UK_FS_TMPL_NODE_GENERATE_STATIC(drv, devtype, nodetype,		\
					devcmp, nodecmp,		\
					ops_table, mode_constraints)	\
UK_FS_TMPL_NODE_DECL_COMMON_STATIC(drv, devtype, nodetype);		\
UK_FS_TMPL_NODE_GEN_COMMON_STATIC(drv, nodecmp, devcmp,			\
				  (ops_table).node_vclose,		\
				  (ops_table).node_nopen,		\
				  (ops_table).node_nclose,		\
				  UK_FS_TMPL_NODE_OPS_POLLOP(ops_table));\
\
UK_FS_TMPL_LIVE_TYPES(drv, UK_FS_TMPL_NODE_LIVE_NODETYPE(drv));		\
UK_FS_TMPL_LIVE_OPS(drv, UK_FS_TMPL_NODE_LIVE_NODETYPE(drv));		\
\
UK_FS_TMPL_NODE_GEN_LIVE_VOPEN_STATIC(drv, (ops_table).node_vopen,	\
				      (ops_table).node_vclose,		\
				      (ops_table).node_errnode);	\
UK_FS_TMPL_NODE_GEN_LIVE_READ_STATIC(drv, (ops_table).node_read);	\
UK_FS_TMPL_NODE_GEN_LIVE_WRITE_STATIC(drv, (ops_table).node_write);	\
UK_FS_TMPL_NODE_GEN_LIVE_MEM_STATIC(drv, (ops_table).node_mem);		\
UK_FS_TMPL_NODE_GEN_LIVE_GETSTAT_STATIC(drv, (ops_table).node_getstat);	\
UK_FS_TMPL_NODE_GEN_LIVE_SETSTAT_STATIC(drv, (ops_table).node_setstat);	\
UK_FS_TMPL_NODE_GEN_LIVE_CTL_STATIC(drv, (ops_table).node_ctl);		\
UK_FS_TMPL_NODE_GEN_LIVE_FS_STAT_STATIC(drv, (ops_table).node_fs_stat);	\
UK_FS_TMPL_NODE_GEN_LIVE_FS_SYNC_STATIC(drv, (ops_table).node_fs_sync);	\
UK_FS_TMPL_NODE_GEN_LIVE_FS_LOOKUP_STATIC(drv, (ops_table).node_fs_lookup,\
					  (ops_table).node_nforget);	\
UK_FS_TMPL_NODE_GEN_LIVE_FS_LISTDIR_STATIC(drv, (ops_table).node_fs_listdir);\
UK_FS_TMPL_NODE_GEN_LIVE_FS_CREATE_STATIC(drv, (ops_table).node_fs_create,\
					  (ops_table).node_nclose,	\
					  (ops_table).node_errnode);	\
UK_FS_TMPL_NODE_GEN_LIVE_FS_UNLINK_STATIC(drv, (ops_table).node_fs_unlink);\
UK_FS_TMPL_NODE_GEN_LIVE_FS_RENAME_STATIC(drv, (ops_table).node_fs_rename);\
UK_FS_TMPL_NODE_GEN_LIVE_FS_READLINK_STATIC(drv, (ops_table).node_fs_readlink);\
UK_FS_TMPL_NODE_GEN_LIVE_NODEKIND_STATIC(drv, (ops_table).node_nodekind);\
UK_FS_TMPL_NODE_GEN_LIVE_STATE_STATIC(drv);				\
UK_FS_TMPL_NODE_GEN_LIVE_ERRNODE_STATIC(drv);				\
UK_FS_TMPL_NODE_GEN_LIVE_ACQUIRE_STATIC(drv);				\
UK_FS_TMPL_NODE_GEN_LIVE_RELEASE_STATIC(drv);				\
UK_FS_TMPL_NODE_GEN_LIVE_OPSTABLE_STATIC(drv);				\
\
UK_FS_TMPL_LIVE_GENERATE_STATIC(drv,  UK_FS_TMPL_NODE_LIVE_NODETYPE(drv),\
				UK_FS_TMPL_NODE_LIVE_NODECMP(drv),	\
				UK_FS_TMPL_NODE_LIVE_OPSTABLE(drv),	\
				mode_constraints)

/**
 * Declare common node driver boilerplate as regular linkage.
 *
 * For use in a common header.
 *
 * @param drv Driver name
 * @param devtype Type used for device identifiers
 * @param nodetype Type used for filesystem node identifiers
 */
#define UK_FS_TMPL_NODE_DECL_COMMON(drv, devtype, nodetype) \
	UK_FS_TMPL_NODE_DECL_COMMON_ATTR(drv, devtype, nodetype, )

/**
 * Declare common node driver boilerplate as static linkage.
 *
 * For use in single-file drivers (or other static contexts).
 *
 * @param drv Driver name
 * @param devtype Type used for device identifiers
 * @param nodetype Type used for filesystem node identifiers
 */
#define UK_FS_TMPL_NODE_DECL_COMMON_STATIC(drv, devtype, nodetype) \
	UK_FS_TMPL_NODE_DECL_COMMON_ATTR(drv, devtype, nodetype, static)

/**
 * INTERNAL. Declare common node driver boilerplate with custom attributes.
 *
 * @param drv Driver name
 * @param devtype Type used for device identifiers
 * @param nodetype Type used for filesystem node identifiers
 * @param attr Function attributes for non-inlines
 */
#define UK_FS_TMPL_NODE_DECL_COMMON_ATTR(drv, devtype, nodetype, attr)	\
typedef devtype UK_FS_TMPL_NODE_DEVTYPE(drv);				\
typedef nodetype UK_FS_TMPL_NODE_NODETYPE(drv);				\
\
struct drv##_VSTATE;							\
struct drv##_RSTATE {							\
	UK_RB_ENTRY(drv##_RSTATE) rb_entry;				\
	struct uk_file_state fstate;					\
	struct drv##_VSTATE *vol;					\
	UK_FS_TMPL_NODE_NODETYPE(drv) node;				\
	__atomic refcnt;						\
};									\
\
static inline void drv##_RSTATE_ACQUIRE(struct drv##_RSTATE *rs)	\
{									\
	uk_refcount_acquire(&rs->refcnt);				\
}									\
\
static inline int drv##_RSTATE_TRY_ACQUIRE(struct drv##_RSTATE *rs)	\
{									\
	return uk_refcount_acquire_if_not_zero(&rs->refcnt);		\
}									\
\
attr void drv##_RSTATE_RELEASE(struct drv##_RSTATE *rs);		\
attr struct drv##_RSTATE *drv##_RSTATE_GET(struct drv##_VSTATE *vs,	\
					   UK_FS_TMPL_NODE_NODETYPE(drv) n);\
attr struct drv##_RSTATE *drv##_RSTATE_OPEN(struct drv##_VSTATE *vs,	\
					    UK_FS_TMPL_NODE_NODETYPE(drv) n,\
					    bool needs_open);		\
\
UK_RB_HEAD(drv##_LIVEMAP, drv##_RSTATE);				\
struct drv##_VSTATE {							\
	UK_FS_TMPL_NODE_DEVTYPE(drv) dev;				\
	struct drv##_LIVEMAP map;					\
	struct uk_rwlock lock;						\
	UK_RB_ENTRY(drv##_VSTATE) rb_entry;				\
	struct uk_alloc *al;						\
	__atomic refcnt;						\
};									\
\
attr struct drv##_VSTATE *drv##_VSTATE_OPEN(struct uk_alloc *al,	\
					    UK_FS_TMPL_NODE_DEVTYPE(drv) dev);\
attr void drv##_VSTATE_RELEASE(struct drv##_VSTATE *vs)

/**
 * Generate common node driver boilerplate with regular linkage.
 *
 * For use in dedicated source file.
 * Assumes previous use of UK_FS_TMPL_NODE_DECL_COMMON.
 *
 * @param drv Driver name
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param node_vclose Node `vclose` function (see node ops above)
 * @param node_vclose Node `nopen` function (see node ops above)
 * @param node_vclose Node `nclose` function (see node ops above)
 * @param node_vclose Node `poll` function or NULL (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_COMMON(drv, nodecmp, devcmp, node_vclose,	\
				   node_nopen, node_nclose, node_poll)	\
	UK_FS_TMPL_NODE_GEN_COMMON_ATTR(drv, nodecmp, devcmp, node_vclose, \
					node_nopen, node_nclose, node_poll, )

/**
 * Generate common node driver boilerplate with static linkage.
 *
 * For use in single file driver.
 * Assumes previous use of UK_FS_TMPL_NODE_DECL_COMMON_STATIC.
 *
 * @param drv Driver name
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param node_vclose Node `vclose` function (see node ops above)
 * @param node_vclose Node `nopen` function (see node ops above)
 * @param node_vclose Node `nclose` function (see node ops above)
 * @param node_vclose Node `poll` function or NULL (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_COMMON_STATIC(drv, nodecmp, devcmp, node_vclose, \
					  node_nopen, node_nclose, node_poll)\
	UK_FS_TMPL_NODE_GEN_COMMON_ATTR(drv, nodecmp, devcmp, node_vclose, \
					node_nopen, node_nclose, node_poll,\
					static)

/**
 * INTERNAL. Generate common node driver boilerplate with custom attributes.
 *
 * Assumes previous use of UK_FS_TMPL_NODE_DECL_COMMON_* as appropriate.
 *
 * @param drv Driver name
 * @param nodecmp Node comparison function (int cmp(nodetype, nodetype))
 * @param node_vclose Node `vclose` function (see node ops above)
 * @param node_vclose Node `nopen` function (see node ops above)
 * @param node_vclose Node `nclose` function (see node ops above)
 * @param node_vclose Node `poll` function or NULL (see node ops above)
 * @param attr Attributes for "exported" functions
 */
#define UK_FS_TMPL_NODE_GEN_COMMON_ATTR(drv, nodecmp, devcmp, node_vclose,\
					node_nopen, node_nclose, node_poll,\
					attr)				\
\
UK_FS_TMPL_NODE_GEN_FSTATE_INIT(drv, node_poll);			\
\
static int drv##_DEVCMP(UK_FS_TMPL_NODE_DEVTYPE(drv) a,			\
			UK_FS_TMPL_NODE_DEVTYPE(drv) b)			\
{									\
	return (devcmp)(a, b);						\
}									\
\
static UK_FS_TMPL_NODE_DEVTYPE(drv) drv##_DEVKEY(struct drv##_VSTATE *vs)\
{									\
	return vs->dev;							\
}									\
\
UK_RB_HEAD(drv##_DEVMAP, drv##_VSTATE);					\
UK_RB_KEY_GENERATE_STATIC(drv##_DEVMAP, drv##_VSTATE, rb_entry,		\
			  drv##_DEVCMP, drv##_DEVKEY);			\
\
static struct {								\
	struct drv##_DEVMAP devmap;					\
	struct uk_mutex devlk;						\
} drv##_OPENDEVS = {							\
	.devmap = UK_RB_INITIALIZER(drv##_OPENDEVS.devmap),		\
	.devlk = UK_MUTEX_INITIALIZER(drv##_OPENDEVS.devlk)		\
};									\
\
static int UK_FS_TMPL_NODE_LIVE_NODECMP(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) a,				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) b)				\
{									\
	return a < b ? -1 : a == b ? 0 : 1;				\
}									\
\
static UK_FS_TMPL_NODE_NODETYPE(drv)					\
drv##_LIVEMAP_KEY(UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs)		\
{									\
	return rs->node;						\
}									\
\
UK_RB_KEY_GENERATE_STATIC(drv##_LIVEMAP, drv##_RSTATE, rb_entry,	\
			  nodecmp, drv##_LIVEMAP_KEY);			\
\
static struct drv##_VSTATE *drv##_VSTATE_NEW(struct uk_alloc *al,	\
					     UK_FS_TMPL_NODE_DEVTYPE(drv) dev)\
{									\
	struct drv##_VSTATE *vs = uk_malloc(al, sizeof(*vs));		\
									\
	if (unlikely(!vs))						\
		return NULL;						\
	UK_RB_INIT(&vs->map);						\
	uk_rwlock_init(&vs->lock);					\
	vs->dev = dev;							\
	vs->al = al;							\
	uk_refcount_init(&vs->refcnt, 1);				\
	return vs;							\
}									\
\
static int drv##_VSTATE_TRY_ACQUIRE(struct drv##_VSTATE *vs)		\
{									\
	return uk_refcount_acquire_if_not_zero(&vs->refcnt);		\
}									\
\
attr struct drv##_VSTATE *drv##_VSTATE_OPEN(struct uk_alloc *al,	\
					    UK_FS_TMPL_NODE_DEVTYPE(drv) dev)\
{									\
	struct drv##_VSTATE *vs;					\
									\
	for (;;) {							\
		uk_mutex_lock(&drv##_OPENDEVS.devlk);			\
		vs = UK_RB_FIND(drv##_DEVMAP, &drv##_OPENDEVS.devmap,	\
				dev);					\
		if (vs) {						\
			if (unlikely(!drv##_VSTATE_TRY_ACQUIRE(vs))) {	\
				/* We found vstate mid-destruction; back off */\
				uk_mutex_unlock(&drv##_OPENDEVS.devlk);	\
				uk_sched_yield();			\
				continue;				\
			}						\
		} else {						\
			vs = drv##_VSTATE_NEW(al, dev);			\
			UK_RB_INSERT(drv##_DEVMAP, &drv##_OPENDEVS.devmap,\
				     vs);				\
		}							\
		uk_mutex_unlock(&drv##_OPENDEVS.devlk);			\
		break;							\
	}								\
	return vs;							\
}									\
\
static void drv##_VSTATE_ACQUIRE(struct drv##_VSTATE *vs)		\
{									\
	uk_refcount_acquire(&vs->refcnt);				\
}									\
\
attr void drv##_VSTATE_RELEASE(struct drv##_VSTATE *vs)			\
{									\
	UK_CTASSERT(node_vclose);					\
	if (uk_refcount_release(&vs->refcnt)) {				\
		UK_ASSERT(UK_RB_EMPTY(&vs->map));			\
		(node_vclose)(vs->dev);					\
		uk_free(vs->al, vs);					\
	}								\
}									\
\
static									\
struct drv##_RSTATE *drv##_RSTATE_FROMNODE(struct drv##_VSTATE *vs,	\
					   UK_FS_TMPL_NODE_NODETYPE(drv) n,\
					   bool needs_open)		\
{									\
	UK_CTASSERT(node_nopen);					\
									\
	struct drv##_RSTATE *rs = uk_malloc(vs->al, sizeof(*rs));	\
									\
	if (unlikely(!rs))						\
		return ERR2PTR(-ENOMEM);				\
									\
	drv##_FSTATE_INIT(&rs->fstate);					\
	if (needs_open) {						\
		int r = (node_nopen)(vs->dev, n, &rs->fstate);		\
									\
		if (unlikely(r)) {					\
			uk_free(vs->al, rs);				\
			return ERR2PTR(r);				\
		}							\
	}								\
	drv##_VSTATE_ACQUIRE(vs);					\
	rs->vol = vs;							\
	rs->node = n;							\
	rs->refcnt = UK_REFCOUNT_INIT_VALUE(1);				\
	return rs;							\
}									\
\
attr struct drv##_RSTATE *drv##_RSTATE_GET(struct drv##_VSTATE *vs,	\
					   UK_FS_TMPL_NODE_NODETYPE(drv) n)\
{									\
	struct drv##_RSTATE *ret;					\
									\
	uk_rwlock_rlock(&vs->lock);					\
	ret = UK_RB_FIND(drv##_LIVEMAP, &vs->map, n);			\
	if (ret)							\
		if (unlikely(!drv##_RSTATE_TRY_ACQUIRE(ret)))		\
			/* We found rstate mid-destruction; report not found */\
			ret = NULL;					\
	uk_rwlock_runlock(&vs->lock);					\
	return ret;							\
}									\
\
attr struct drv##_RSTATE *drv##_RSTATE_OPEN(struct drv##_VSTATE *vs,	\
					    UK_FS_TMPL_NODE_NODETYPE(drv) n,\
					    bool needs_open)		\
{									\
	struct drv##_RSTATE *ret;					\
									\
	for (;;) {							\
		uk_rwlock_wlock(&vs->lock);				\
		ret = UK_RB_FIND(drv##_LIVEMAP, &vs->map, n);		\
		if (ret) {						\
			if (unlikely(!drv##_RSTATE_TRY_ACQUIRE(ret))) {	\
				/* We found rstate mid-destruction; back off */\
				uk_rwlock_wunlock(&vs->lock);		\
				uk_sched_yield();			\
				continue;				\
			}						\
		} else {						\
			struct drv##_RSTATE *prev __maybe_unused;	\
									\
			ret = drv##_RSTATE_FROMNODE(vs, n, needs_open);	\
			if (unlikely(PTRISERR(ret)))			\
				goto out_unlock;			\
			prev = UK_RB_INSERT(drv##_LIVEMAP, &vs->map, ret);\
			UK_ASSERT(!prev);				\
		}							\
out_unlock:								\
		uk_rwlock_wunlock(&vs->lock);				\
		break;							\
	}								\
	return ret;							\
}									\
\
attr void drv##_RSTATE_RELEASE(struct drv##_RSTATE *rs)			\
{									\
	UK_CTASSERT(node_nclose);					\
	if (uk_refcount_release(&rs->refcnt)) {				\
		struct drv##_VSTATE *vs = rs->vol;			\
									\
		uk_rwlock_wlock(&vs->lock);				\
		UK_RB_REMOVE(drv##_LIVEMAP, &vs->map, rs);		\
		uk_rwlock_wunlock(&vs->lock);				\
									\
		(node_nclose)(vs->dev, rs->node);			\
		uk_free(vs->al, rs);					\
		drv##_VSTATE_RELEASE(vs);				\
	}								\
}

/* We need to initialize file state differently if polled files are enabled */
#if CONFIG_LIBUKFILE_POLLED

#define UK_FS_TMPL_NODE_GEN_FSTATE_INIT(drv, node_poll)			\
static void drv##_FSTATE_INIT(struct uk_file_state *st)			\
{									\
	if (node_poll)							\
		*st = UK_FILE_POLLED_STATE_INIT_VALUE(*st, (node_poll));\
	else								\
		*st = UK_FILE_STATE_INIT_VALUE(*st);			\
}

#else /* !CONFIG_LIBUKFILE_POLLED */

#define UK_FS_TMPL_NODE_GEN_FSTATE_INIT(drv, node_poll)			\
static void drv##_FSTATE_INIT(struct uk_file_state *st)			\
{									\
	*st = UK_FILE_STATE_INIT_VALUE(*st);				\
}

#endif /* !CONFIG_LIBUKFILE_POLLED */

/* Live ops */

/* vopen */

#define UK_FS_TMPL_NODE_OP_LIVE_VOPEN(drv) drv##_LIVE_VOPEN

/**
 * Declare the live template vopen operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_VOPEN(drv) \
UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) UK_FS_TMPL_NODE_OP_LIVE_VOPEN(drv)(	\
	union uk_fs_vopen_vol vol, unsigned long flags,			\
	union uk_fs_vopen_data data, size_t fmt)

/**
 * Generate the live template vopen operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_vopen Node `vopen` function (see node ops above)
 * @param node_vclose Node `vclose` function (see node ops above)
 * @param node_errnode Node `errnode` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_VOPEN(drv, node_vopen, node_vclose,	\
				       node_errnode)			\
	UK_FS_TMPL_NODE_GEN_LIVE_VOPEN_ATTR(drv, node_vopen, node_vclose,\
					    node_errnode, )

/**
 * Generate the live template vopen operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_vopen Node `vopen` function (see node ops above)
 * @param node_vclose Node `vclose` function (see node ops above)
 * @param node_errnode Node `errnode` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_VOPEN_STATIC(drv, node_vopen, node_vclose,\
					      node_errnode)		\
	UK_FS_TMPL_NODE_GEN_LIVE_VOPEN_ATTR(drv, node_vopen, node_vclose,\
					    node_errnode, static)

/**
 * INTERNAL. Generate the live template vopen operation with custom attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_vopen Node `vopen` function (see node ops above)
 * @param node_vclose Node `vclose` function (see node ops above)
 * @param node_errnode Node `errnode` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_VOPEN_ATTR(drv, node_vopen, node_vclose,\
					    node_errnode, attr)		\
attr UK_FS_TMPL_NODE_DECL_LIVE_VOPEN(drv)				\
{									\
	UK_CTASSERT(node_vopen);					\
									\
	int err;							\
	struct drv##_VSTATE *vs;					\
	struct drv##_RSTATE *rs;					\
	struct drv##_OPENDEV od = (node_vopen)(vol, flags, data, fmt);	\
									\
	if (unlikely((err = (node_errnode)(od.rootnode))))		\
		return ERR2PTR(err);					\
									\
	vs = drv##_VSTATE_OPEN(uk_alloc_get_default(), od.dev);		\
	if (unlikely(!vs)) {						\
		(node_vclose)(od.dev);					\
		return ERR2PTR(-ENOMEM);				\
	}								\
									\
	rs = drv##_RSTATE_OPEN(vs, od.rootnode, true);			\
	/* On failure, vs will be cleaned up and rootnode forgotten */	\
	drv##_VSTATE_RELEASE(vs);					\
	return rs;							\
}

/* read */

#define UK_FS_TMPL_NODE_OP_LIVE_READ(drv) drv##_LIVE_READ

/**
 * Declare the live template read operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_READ(drv)				\
ssize_t UK_FS_TMPL_NODE_OP_LIVE_READ(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	const struct iovec *iov, size_t iovcnt, size_t off, long flags,	\
	unsigned long mntflags)

/**
 * Generate the live template read operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_read Node `read` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_READ(drv, node_read) \
	UK_FS_TMPL_NODE_GEN_LIVE_READ_ATTR(drv, node_read, )

/**
 * Generate the live template read operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_read Node `read` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_READ_STATIC(drv, node_read) \
	UK_FS_TMPL_NODE_GEN_LIVE_READ_ATTR(drv, node_read, static)

/**
 * INTERNAL. Generate the live template read operation with custom attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_read Node `read` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_READ_ATTR(drv, node_read, attr)	\
attr UK_FS_TMPL_NODE_DECL_LIVE_READ(drv)				\
{									\
	UK_CTASSERT(node_read);						\
	return (node_read)(rs->vol->dev, rs->node,			\
			   iov, iovcnt, off, flags, mntflags);		\
}

/* write */

#define UK_FS_TMPL_NODE_OP_LIVE_WRITE(drv) drv##_LIVE_WRITE

/**
 * Declare the live template write operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_WRITE(drv)				\
ssize_t UK_FS_TMPL_NODE_OP_LIVE_WRITE(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	const struct iovec *iov, size_t iovcnt, size_t off, long flags,	\
	unsigned long mntflags)

/**
 * Generate the live template write operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_write Node `write` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_WRITE(drv, node_write) \
	UK_FS_TMPL_NODE_GEN_LIVE_WRITE_ATTR(drv, node_write, )

/**
 * Generate the live template write operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_write Node `write` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_WRITE_STATIC(drv, node_write) \
	UK_FS_TMPL_NODE_GEN_LIVE_WRITE_ATTR(drv, node_write, static)

/**
 * INTERNAL. Generate the live template write operation with custom attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_write Node `write` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_WRITE_ATTR(drv, node_write, attr)	\
attr UK_FS_TMPL_NODE_DECL_LIVE_WRITE(drv)				\
{									\
	UK_CTASSERT(node_write);					\
	return (node_write)(rs->vol->dev, rs->node,			\
			    iov, iovcnt, off, flags, mntflags);		\
}

/* mem */

#define UK_FS_TMPL_NODE_OP_LIVE_MEM(drv) drv##_LIVE_MEM

/**
 * Declare the live template mem operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_MEM(drv)				\
ssize_t UK_FS_TMPL_NODE_OP_LIVE_MEM(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	enum uk_file_mem_op op, size_t off, size_t len,			\
	struct iovec *iov, size_t iovcnt, unsigned long mntflags)

/**
 * Generate the live template mem operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_mem Node `mem` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_MEM(drv, node_mem) \
	UK_FS_TMPL_NODE_GEN_LIVE_MEM_ATTR(drv, node_mem, )

/**
 * Generate the live template mem operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_mem Node `mem` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_MEM_STATIC(drv, node_mem) \
	UK_FS_TMPL_NODE_GEN_LIVE_MEM_ATTR(drv, node_mem, static)

/**
 * INTERNAL. Generate the live template mem operation with custom attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_mem Node `mem` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_MEM_ATTR(drv, node_mem, attr)		\
attr UK_FS_TMPL_NODE_DECL_LIVE_MEM(drv)					\
{									\
	UK_CTASSERT(node_mem);						\
	return (node_mem)(rs->vol->dev, rs->node, op,			\
			  off, len, iov, iovcnt, mntflags);		\
}

/* getstat */

#define UK_FS_TMPL_NODE_OP_LIVE_GETSTAT(drv) drv##_LIVE_GETSTAT

/**
 * Declare the live template getstat operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_GETSTAT(drv)				\
int UK_FS_TMPL_NODE_OP_LIVE_GETSTAT(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	unsigned int mask, struct uk_statx *arg, unsigned long mntflags)

/**
 * Generate the live template getstat operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_getstat Node `getstat` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_GETSTAT(drv, node_getstat) \
	UK_FS_TMPL_NODE_GEN_LIVE_GETSTAT_ATTR(drv, node_getstat, )

/**
 * Generate the live template getstat operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_getstat Node `getstat` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_GETSTAT_STATIC(drv, node_getstat) \
	UK_FS_TMPL_NODE_GEN_LIVE_GETSTAT_ATTR(drv, node_getstat, static)

/**
 * INTERNAL. Generate the live template getstat operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_getstat Node `getstat` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_GETSTAT_ATTR(drv, node_getstat, attr)	\
attr UK_FS_TMPL_NODE_DECL_LIVE_GETSTAT(drv)				\
{									\
	UK_CTASSERT(node_getstat);					\
	return (node_getstat)(rs->vol->dev, rs->node, mask, arg, mntflags);\
}

/* setstat */

#define UK_FS_TMPL_NODE_OP_LIVE_SETSTAT(drv) drv##_LIVE_SETSTAT

/**
 * Declare the live template setstat operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_SETSTAT(drv)				\
int UK_FS_TMPL_NODE_OP_LIVE_SETSTAT(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	unsigned int mask, const struct uk_statx *arg, unsigned long mntflags)

/**
 * Generate the live template setstat operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_setstat Node `setstat` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_SETSTAT(drv, node_setstat) \
	UK_FS_TMPL_NODE_GEN_LIVE_SETSTAT_ATTR(drv, node_setstat, )

/**
 * Generate the live template setstat operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_setstat Node `setstat` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_SETSTAT_STATIC(drv, node_setstat) \
	UK_FS_TMPL_NODE_GEN_LIVE_SETSTAT_ATTR(drv, node_setstat, static)

/**
 * INTERNAL. Generate the live template setstat operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_setstat Node `setstat` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_SETSTAT_ATTR(drv, node_setstat, attr)	\
attr UK_FS_TMPL_NODE_DECL_LIVE_SETSTAT(drv)				\
{									\
	UK_CTASSERT(node_setstat);					\
	return (node_setstat)(rs->vol->dev, rs->node, mask, arg, mntflags);\
}

/* ctl */

#define UK_FS_TMPL_NODE_OP_LIVE_CTL(drv) drv##_LIVE_CTL

/**
 * Declare the live template ctl operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_CTL(drv)				\
int UK_FS_TMPL_NODE_OP_LIVE_CTL(drv)(UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,\
				     int fam, int req, uintptr_t arg1,	\
				     uintptr_t arg2, uintptr_t arg3,	\
				     unsigned long mntflags)

/**
 * Generate the live template ctl operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_ctl Node `ctl` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_CTL(drv, node_ctl) \
	UK_FS_TMPL_NODE_GEN_LIVE_CTL_ATTR(drv, node_ctl, )

/**
 * Generate the live template ctl operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_ctl Node `ctl` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_CTL_STATIC(drv, node_ctl) \
	UK_FS_TMPL_NODE_GEN_LIVE_CTL_ATTR(drv, node_ctl, static)

/**
 * INTERNAL. Generate the live template ctl operation with custom attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_ctl Node `ctl` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_CTL_ATTR(drv, node_ctl, attr)		\
attr UK_FS_TMPL_NODE_DECL_LIVE_CTL(drv)					\
{									\
	UK_CTASSERT(node_ctl);						\
	return (node_ctl)(rs->vol->dev, rs->node, fam, req,		\
			  arg1, arg2, arg3, mntflags);			\
}

/* fs stat */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_STAT(drv) drv##_LIVE_FS_STAT

/**
 * Declare the live template fs_stat operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_STAT(drv)				\
int UK_FS_TMPL_NODE_OP_LIVE_FS_STAT(drv)(UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,\
					 struct statfs *buf)

/**
 * Generate the live template fs_stat operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_stat Node `fs_stat` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_STAT(drv, node_fs_stat) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_STAT_ATTR(drv, node_fs_stat, )

/**
 * Generate the live template fs_stat operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_stat Node `fs_stat` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_STAT_STATIC(drv, node_fs_stat) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_STAT_ATTR(drv, node_fs_stat, static)

/**
 * INTERNAL. Generate the live template fs_stat operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_stat Node `fs_stat` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_STAT_ATTR(drv, node_fs_stat, attr)	\
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_STAT(drv)				\
{									\
	UK_CTASSERT(node_fs_stat);					\
	return (node_fs_stat)(rs->vol->dev, buf);			\
}

/* fs sync */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_SYNC(drv) drv##_LIVE_FS_SYNC

/**
 * Declare the live template fs_sync operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_SYNC(drv) \
int UK_FS_TMPL_NODE_OP_LIVE_FS_SYNC(drv)(UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs)

/**
 * Generate the live template fs_sync operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_sync Node `fs_sync` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_SYNC(drv, node_fs_sync) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_SYNC_ATTR(drv, node_fs_sync, )

/**
 * Generate the live template fs_sync operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_sync Node `fs_sync` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_SYNC_STATIC(drv, node_fs_sync) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_SYNC_ATTR(drv, node_fs_sync, static)

/**
 * INTERNAL. Generate the live template fs_sync operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_sync Node `fs_sync` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_SYNC_ATTR(drv, node_fs_sync, attr)	\
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_SYNC(drv)				\
{									\
	UK_CTASSERT(node_fs_sync);					\
	return (node_fs_sync)(rs->vol->dev);				\
}

/* fs lookup */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_LOOKUP(drv) drv##_LIVE_FS_LOOKUP

/**
 * Declare the live template fs_lookup operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_LOOKUP(drv)			\
int UK_FS_TMPL_NODE_OP_LIVE_FS_LOOKUP(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	const char *path, size_t len,					\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) *out_nodes,			\
	union uk_fs_lookup_out *out_ukfs,				\
	size_t *nout)

/**
 * Generate the live template fs_lookup operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_lookup Node `fs_lookup` function (see node ops above)
 * @param node_nforget Node `nforget` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_LOOKUP(drv, node_fs_lookup, node_nforget) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_LOOKUP_ATTR(drv, node_fs_lookup,	\
						node_nforget, )

/**
 * Generate the live template fs_lookup operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_lookup Node `fs_lookup` function (see node ops above)
 * @param node_nforget Node `nforget` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_LOOKUP_STATIC(drv, node_fs_lookup,	\
						  node_nforget)		\
	UK_FS_TMPL_NODE_GEN_LIVE_FS_LOOKUP_ATTR(drv, node_fs_lookup,	\
						node_nforget, static)

/**
 * INTERNAL. Generate the live template fs_lookup operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_lookup Node `fs_lookup` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_LOOKUP_ATTR(drv, node_fs_lookup,	\
						node_nforget, attr)	\
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_LOOKUP(drv)				\
{									\
	UK_CTASSERT(node_nforget);					\
									\
	UK_FS_TMPL_NODE_NODETYPE(drv) n = rs->node;			\
	size_t cur = 0;							\
	bool below = false; /* Set to true on deep lookup of raw node ref */\
									\
	for (;;) {							\
		int ret;						\
		size_t prog;						\
		UK_FS_TMPL_NODE_NODETYPE(drv) nod[2];			\
									\
		if (below && cur < len) {				\
			UK_ASSERT(path[cur] == '/');			\
			cur++;						\
		}							\
		if (below && cur == len) {				\
			UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) out;		\
									\
			out = drv##_RSTATE_OPEN(rs->vol, n, true);	\
			if (unlikely(PTRISERR(out))) {			\
				(node_nforget)(rs->vol->dev, n);	\
				return PTR2ERR(out);			\
			}						\
			out_nodes[0] = out;				\
			return UKFS_SUCCESS;				\
		}							\
		ret = (node_fs_lookup)(rs->vol->dev, n, &path[cur], len - cur,\
				       nod, out_ukfs, &prog);		\
		if (below && (ret != UKFS_STOP_SYM || n != nod[1]))	\
			/* n has no live ref & won't be used again, forget */\
			(node_nforget)(rs->vol->dev, n);		\
		if (unlikely(ret < 0))					\
			return ret;					\
		switch (ret) {						\
		case UKFS_SUCCESS:					\
		{							\
			UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) out;		\
									\
			out = drv##_RSTATE_OPEN(rs->vol, nod[0], true);	\
			if (unlikely(PTRISERR(out))) {			\
				(node_nforget)(rs->vol->dev, nod[0]);	\
				return PTR2ERR(out);			\
			}						\
			out_nodes[0] = out;				\
			return ret;					\
		}							\
		case UKFS_STOP_NOD:					\
		{							\
			UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) out;		\
									\
			/* Non-blocking lookup not supported */		\
			UK_ASSERT(prog);				\
			/* If live ref exists, pass it up */		\
			if ((out = drv##_RSTATE_GET(rs->vol, nod[0]))) {\
				out_nodes[0] = out;			\
				*nout = cur + prog;			\
				return ret;				\
			}						\
			n = nod[0];					\
			cur += prog;					\
			below = true;					\
			break;						\
		}							\
		case UKFS_STOP_SYM: /* Convert & return nodes */	\
		{							\
			UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) prs;		\
			UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) trs;		\
									\
			if (nod[1] == rs->node) {			\
				drv##_RSTATE_ACQUIRE(rs);		\
				prs = rs;				\
			} else {					\
				prs = drv##_RSTATE_OPEN(rs->vol, nod[1], true);\
				if (unlikely(PTRISERR(prs))) {		\
					(node_nforget)(rs->vol->dev, nod[1]);\
					/* Cleanup nod[0] as needed */	\
					trs = drv##_RSTATE_GET(rs->vol,	\
							       nod[0]);	\
					if (trs)			\
						drv##_RSTATE_RELEASE(trs);\
					else				\
						(node_nforget)(rs->vol->dev,\
							       nod[0]);	\
					return PTR2ERR(prs);		\
				}					\
			}						\
			trs = drv##_RSTATE_OPEN(rs->vol, nod[0], true);	\
			if (unlikely(PTRISERR(trs))) {			\
				(node_nforget)(rs->vol->dev, nod[0]);	\
				drv##_RSTATE_RELEASE(prs);		\
				return PTR2ERR(trs);			\
			}						\
			out_nodes[0] = trs;				\
			out_nodes[1] = prs;				\
			*nout = cur + prog;				\
			return ret;					\
		}							\
		case UKFS_STOP_SPEC:					\
		case UKFS_STOP_FILE: /* Node driver wrote output already */\
		case UKFS_STOP_END: /* Propagate */			\
			*nout = cur + prog;				\
			return ret;					\
		case UKFS_STOP_MNT: /* Cannot happen */			\
		default:						\
			UK_CRASH("Invalid lookup return from node driver\n");\
		}							\
	}								\
}

/* fs listdir */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_LISTDIR(drv) drv##_LIVE_FS_LISTDIR

/**
 * Declare the live template fs_listdir operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_LISTDIR(drv)			\
ssize_t UK_FS_TMPL_NODE_OP_LIVE_FS_LISTDIR(drv)(			\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	size_t *curp, void *buf, size_t len, unsigned long mntflags)

/**
 * Generate the live template fs_listdir operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_listdir Node `fs_listdir` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_LISTDIR(drv, node_fs_listdir) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_LISTDIR_ATTR(drv, node_fs_listdir, )

/**
 * Generate the live template fs_listdir operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_listdir Node `fs_listdir` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_LISTDIR_STATIC(drv, node_fs_listdir) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_LISTDIR_ATTR(drv, node_fs_listdir, static)

/**
 * INTERNAL. Generate the live template fs_listdir operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_listdir Node `fs_listdir` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_LISTDIR_ATTR(drv, node_fs_listdir, attr) \
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_LISTDIR(drv)				\
{									\
	UK_CTASSERT(node_fs_listdir);					\
	return (node_fs_listdir)(rs->vol->dev, rs->node, curp,		\
				 buf, len, mntflags);			\
}

/* fs create */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_CREATE(drv) drv##_LIVE_FS_CREATE

/**
 * Declare the live template fs_create operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_CREATE(drv)			\
UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) UK_FS_TMPL_NODE_OP_LIVE_FS_CREATE(drv)( \
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	const char *name, size_t len, unsigned int mode, int flags,	\
	union UK_FS_TMPL_LIVE_CREATE_TARGET(drv) target,		\
	unsigned long mntflags)

/**
 * Generate the live template fs_create operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_create Node `fs_create` function (see node ops above)
 * @param node_fs_errnode Node `fs_errnode` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_CREATE(drv, node_fs_create,		\
					   node_nclose, node_errnode)	\
	UK_FS_TMPL_NODE_GEN_LIVE_FS_CREATE_ATTR(drv, node_fs_create,	\
						node_nclose, node_errnode, )

/**
 * Generate the live template fs_create operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_create Node `fs_create` function (see node ops above)
 * @param node_fs_errnode Node `fs_errnode` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_CREATE_STATIC(drv, node_fs_create,	\
						  node_nclose, node_errnode)\
	UK_FS_TMPL_NODE_GEN_LIVE_FS_CREATE_ATTR(drv, node_fs_create,	\
						node_nclose, node_errnode,\
						static)

/**
 * INTERNAL. Generate the live template fs_create operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_create Node `fs_create` function (see node ops above)
 * @param node_fs_errnode Node `fs_errnode` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_CREATE_ATTR(drv, node_fs_create,	\
						node_nclose, node_errnode,\
						attr)			\
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_CREATE(drv)				\
{									\
	UK_CTASSERT(node_fs_create);					\
									\
	const unsigned int ftype = mode & S_IFMT;			\
	int err;							\
	UK_FS_TMPL_NODE_NODETYPE(drv) rnode;				\
	union UK_FS_TMPL_NODE_CREATE_TARGET(drv) node_target;		\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) ret;				\
									\
	switch(ftype) {							\
	case S_IFMT:							\
		/* Hardlink, convert target */				\
		node_target.node = target.livenode->node;		\
		break;							\
	case 0:								\
		/* Pseudo-file */					\
		node_target.ukfs.file = target.ukfs.file;		\
		break;							\
	case S_IFLNK:							\
		node_target.ukfs.path = target.ukfs.path;		\
		break;							\
	case S_IFCHR:							\
	case S_IFBLK:							\
	case S_IFSOCK:							\
	case S_IFIFO:							\
		node_target.ukfs.special = target.ukfs.special;		\
		break;							\
	default:							\
		/* target ignored */					\
		node_target.ukfs = UKFS_NOTARGET;			\
	}								\
									\
	rnode = (node_fs_create)(rs->vol->dev, rs->node, name, len,	\
				 mode, flags, node_target, mntflags);	\
	if (unlikely((err = (node_errnode)(rnode))))			\
		return ERR2PTR(err);					\
	switch (ftype) {						\
	case 0:								\
	case S_IFCHR:							\
	case S_IFBLK:							\
	case S_IFSOCK:							\
	case S_IFIFO:							\
		/* Any value not errnode signals success; NULL fits */	\
		return NULL;						\
	default:							\
		/* Convert to live ref & return */			\
		ret = drv##_RSTATE_OPEN(rs->vol, rnode, false);		\
		if (unlikely(PTRISERR(ret)))				\
			(node_nclose)(rs->vol->dev, rnode);		\
		return ret;						\
	}								\
}

/* fs unlink */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_UNLINK(drv) drv##_LIVE_FS_UNLINK

/**
 * Declare the live template fs_unlink operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_UNLINK(drv)			\
int UK_FS_TMPL_NODE_OP_LIVE_FS_UNLINK(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	const char *name, size_t len, unsigned int flags,		\
	unsigned long mntflags)

/**
 * Generate the live template fs_unlink operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_unlink Node `fs_unlink` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_UNLINK(drv, node_fs_unlink) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_UNLINK_ATTR(drv, node_fs_unlink, )

/**
 * Generate the live template fs_unlink operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_unlink Node `fs_unlink` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_UNLINK_STATIC(drv, node_fs_unlink) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_UNLINK_ATTR(drv, node_fs_unlink, static)

/**
 * INTERNAL. Generate the live template fs_unlink operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_unlink Node `fs_unlink` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_UNLINK_ATTR(drv, node_fs_unlink, attr) \
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_UNLINK(drv)				\
{									\
	UK_CTASSERT(node_fs_unlink);					\
	return (node_fs_unlink)(rs->vol->dev, rs->node,			\
				name, len, flags, mntflags);		\
}

/* fs rename */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_RENAME(drv) drv##_LIVE_FS_RENAME

/**
 * Declare the live template fs_rename operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_RENAME(drv)			\
int UK_FS_TMPL_NODE_OP_LIVE_FS_RENAME(drv)(				\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,				\
	const char *name, size_t nlen,					\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) dest,			\
	const char *dname, size_t dlen,					\
	unsigned int flags, unsigned long mntflags)

/**
 * Generate the live template fs_rename operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_rename Node `fs_rename` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_RENAME(drv, node_fs_rename) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_RENAME_ATTR(drv, node_fs_rename, )

/**
 * Generate the live template fs_rename operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_rename Node `fs_rename` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_RENAME_STATIC(drv, node_fs_rename) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_RENAME_ATTR(drv, node_fs_rename, static)

/**
 * INTERNAL. Generate the live template fs_rename operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_rename Node `fs_rename` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_RENAME_ATTR(drv, node_fs_rename, attr) \
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_RENAME(drv)				\
{									\
	UK_CTASSERT(node_fs_rename);					\
	return (node_fs_rename)(rs->vol->dev, rs->node, name, nlen,	\
				dest->node, dname, dlen,		\
				flags, mntflags);			\
}

/* fs readlink */

#define UK_FS_TMPL_NODE_OP_LIVE_FS_READLINK(drv) drv##_LIVE_FS_READLINK

/**
 * Declare the live template fs_readlink operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_FS_READLINK(drv)			\
struct uk_fs_path UK_FS_TMPL_NODE_OP_LIVE_FS_READLINK(drv)(		\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs)

/**
 * Generate the live template fs_readlink operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_readlink Node `fs_readlink` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_READLINK(drv, node_fs_readlink) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_READLINK_ATTR(drv, node_fs_readlink, )

/**
 * Generate the live template fs_readlink operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_readlink Node `fs_readlink` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_READLINK_STATIC(drv, node_fs_readlink) \
	UK_FS_TMPL_NODE_GEN_LIVE_FS_READLINK_ATTR(drv, node_fs_readlink, static)

/**
 * INTERNAL. Generate the live template fs_readlink operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_fs_readlink Node `fs_readlink` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_FS_READLINK_ATTR(drv, node_fs_readlink, attr) \
attr UK_FS_TMPL_NODE_DECL_LIVE_FS_READLINK(drv)				\
{									\
	UK_CTASSERT(node_fs_readlink);					\
	return (node_fs_readlink)(rs->vol->dev, rs->node);		\
}

/* Glue ops */

/* nodekind */

#define UK_FS_TMPL_NODE_OP_LIVE_NODEKIND(drv) drv##_LIVE_NODEKIND

/**
 * Declare the live template nodekind operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_NODEKIND(drv)				\
int UK_FS_TMPL_NODE_OP_LIVE_NODEKIND(drv)(				\
	const UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs,			\
	enum uk_fs_tmpl_node_kind kind)

/**
 * Generate the live template nodekind operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_nodekind Node `nodekind` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_NODEKIND(drv, node_nodekind) \
	UK_FS_TMPL_NODE_GEN_LIVE_NODEKIND_ATTR(drv, node_nodekind, )

/**
 * Generate the live template nodekind operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_nodekind Node `nodekind` function (see node ops above)
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_NODEKIND_STATIC(drv, node_nodekind) \
	UK_FS_TMPL_NODE_GEN_LIVE_NODEKIND_ATTR(drv, node_nodekind, static)

/**
 * INTERNAL. Generate the live template nodekind operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param node_nodekind Node `nodekind` function (see node ops above)
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_NODEKIND_ATTR(drv, node_nodekind, attr) \
attr UK_FS_TMPL_NODE_DECL_LIVE_NODEKIND(drv)				\
{									\
	UK_CTASSERT(node_nodekind);					\
	return (node_nodekind)(rs->vol->dev, rs->node, kind);		\
}

/* state */

#define UK_FS_TMPL_NODE_OP_LIVE_STATE(drv) drv##_LIVE_STATE

/**
 * Declare the live template state operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_STATE(drv)				\
struct uk_file_state *UK_FS_TMPL_NODE_OP_LIVE_STATE(drv)(		\
	UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs)

/**
 * Generate the live template state operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_STATE(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_STATE_ATTR(drv, )

/**
 * Generate the live template state operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_STATE_STATIC(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_STATE_ATTR(drv, static)

/**
 * INTERNAL. Generate the live template state operation with custom attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_STATE_ATTR(drv, attr)			\
attr UK_FS_TMPL_NODE_DECL_LIVE_STATE(drv)				\
{									\
	return &rs->fstate;						\
}

/* errnode */

#define UK_FS_TMPL_NODE_OP_LIVE_ERRNODE(drv) drv##_LIVE_ERRNODE

/**
 * Declare the live template errnode operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_ERRNODE(drv)				\
int UK_FS_TMPL_NODE_OP_LIVE_ERRNODE(drv)(				\
	const UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs)

/**
 * Generate the live template errnode operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_ERRNODE(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_ERRNODE_ATTR(drv, )

/**
 * Generate the live template errnode operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_ERRNODE_STATIC(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_ERRNODE_ATTR(drv, static)

/**
 * INTERNAL. Generate the live template errnode operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_ERRNODE_ATTR(drv, attr)		\
attr UK_FS_TMPL_NODE_DECL_LIVE_ERRNODE(drv)				\
{									\
	return PTRISERR(rs) ? PTR2ERR(rs) : 0;				\
}

/* acquire */

#define UK_FS_TMPL_NODE_OP_LIVE_ACQUIRE(drv) drv##_LIVE_ACQUIRE

/**
 * Declare the live template acquire operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_ACQUIRE(drv) \
void UK_FS_TMPL_NODE_OP_LIVE_ACQUIRE(drv)(UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs)

/**
 * Generate the live template acquire operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_ACQUIRE(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_ACQUIRE_ATTR(drv, )

/**
 * Generate the live template acquire operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_ACQUIRE_STATIC(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_ACQUIRE_ATTR(drv, static)

/**
 * INTERNAL. Generate the live template acquire operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_ACQUIRE_ATTR(drv, attr)		\
attr UK_FS_TMPL_NODE_DECL_LIVE_ACQUIRE(drv)				\
{									\
	drv##_RSTATE_ACQUIRE(rs);					\
}

/* release */

#define UK_FS_TMPL_NODE_OP_LIVE_RELEASE(drv) drv##_LIVE_RELEASE

/**
 * Declare the live template release operation of `drv`.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_DECL_LIVE_RELEASE(drv) \
void UK_FS_TMPL_NODE_OP_LIVE_RELEASE(drv)(UK_FS_TMPL_NODE_LIVE_NODETYPE(drv) rs)

/**
 * Generate the live template release operation with regular linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_RELEASE(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_RELEASE_ATTR(drv, )

/**
 * Generate the live template release operation with static linkage.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_RELEASE_STATIC(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_RELEASE_ATTR(drv, static)

/**
 * INTERNAL. Generate the live template release operation with custom
 * attributes.
 *
 * Assumes previous declaration of common node boilerplate.
 *
 * @param drv Driver name
 * @param attr Function attributes
 */
#define UK_FS_TMPL_NODE_GEN_LIVE_RELEASE_ATTR(drv, attr)		\
attr UK_FS_TMPL_NODE_DECL_LIVE_RELEASE(drv)				\
{									\
	drv##_RSTATE_RELEASE(rs);					\
}

/* liveops table */

#define UK_FS_TMPL_NODE_GEN_LIVE_OPSTABLE(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_OPSTABLE_ATTR(drv, )

#define UK_FS_TMPL_NODE_GEN_LIVE_OPSTABLE_STATIC(drv) \
	UK_FS_TMPL_NODE_GEN_LIVE_OPSTABLE_ATTR(drv, static)

#define UK_FS_TMPL_NODE_GEN_LIVE_OPSTABLE_ATTR(drv, attr)		\
UK_FS_TMPL_LIVE_OPSTABLE(drv, drv##_NODE_LIVEOPS);			\
\
attr const struct drv##_NODE_LIVEOPS UK_FS_TMPL_NODE_LIVE_OPSTABLE(drv) = { \
	.live_vopen = UK_FS_TMPL_NODE_OP_LIVE_VOPEN(drv),		\
	.live_read = UK_FS_TMPL_NODE_OP_LIVE_READ(drv),			\
	.live_write = UK_FS_TMPL_NODE_OP_LIVE_WRITE(drv),		\
	.live_mem = UK_FS_TMPL_NODE_OP_LIVE_MEM(drv),			\
	.live_getstat = UK_FS_TMPL_NODE_OP_LIVE_GETSTAT(drv),		\
	.live_setstat = UK_FS_TMPL_NODE_OP_LIVE_SETSTAT(drv),		\
	.live_ctl = UK_FS_TMPL_NODE_OP_LIVE_CTL(drv),			\
	.live_fs_stat = UK_FS_TMPL_NODE_OP_LIVE_FS_STAT(drv),		\
	.live_fs_sync = UK_FS_TMPL_NODE_OP_LIVE_FS_SYNC(drv),		\
	.live_fs_lookup = UK_FS_TMPL_NODE_OP_LIVE_FS_LOOKUP(drv),	\
	.live_fs_listdir = UK_FS_TMPL_NODE_OP_LIVE_FS_LISTDIR(drv),	\
	.live_fs_create = UK_FS_TMPL_NODE_OP_LIVE_FS_CREATE(drv),	\
	.live_fs_unlink = UK_FS_TMPL_NODE_OP_LIVE_FS_UNLINK(drv),	\
	.live_fs_rename = UK_FS_TMPL_NODE_OP_LIVE_FS_RENAME(drv),	\
	.live_fs_readlink = UK_FS_TMPL_NODE_OP_LIVE_FS_READLINK(drv),	\
	.live_nodekind = UK_FS_TMPL_NODE_OP_LIVE_NODEKIND(drv),		\
	.live_state = UK_FS_TMPL_NODE_OP_LIVE_STATE(drv),		\
	.live_errnode = UK_FS_TMPL_NODE_OP_LIVE_ERRNODE(drv),		\
	.live_acquire = UK_FS_TMPL_NODE_OP_LIVE_ACQUIRE(drv),		\
	.live_release = UK_FS_TMPL_NODE_OP_LIVE_RELEASE(drv),		\
}

#endif /* __UKFS_FS_TEMPLATE_NODE_H__ */
