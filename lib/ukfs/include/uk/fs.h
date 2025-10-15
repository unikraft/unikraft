/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Unikraft filesystem interface */

#ifndef __UKFS_FS_H__
#define __UKFS_FS_H__

#include <stdbool.h>
#include <sys/statfs.h>

#include <uk/assert.h>
#include <uk/errptr.h>
#include <uk/file.h>

/* Permissions checking, provided by ukfs for drivers */

/**
 * Check whether `desired` access mode on file with `mode`, `uid`, and `gid` is
 * permitted in the current execution context.
 *
 * Bits for desired mode: traditional RWX bits (R:4, W:2, X:1)
 *
 * Drivers should check permissions on directory lookups only.
 * All other operations have their permissions checked by higher levels.
 */
#if CONFIG_LIBUKFS_PERMISSIONS

/* When permissions are enabled, this is implemented by the VFS interface lib */
int uk_fs_checkperm(int desired, int mode, int uid, int gid);

#else /* !CONFIG_LIBUKFS_PERMISSIONS */

/* Otherwise, drivers use this no-op inline directly */
static inline
int uk_fs_checkperm(int desired __unused, int mode __unused,
		    int uid __unused, int gid __unused)
{
	return 0;
}

#endif /* !CONFIG_LIBUKFS_PERMISSIONS */

/* Filesystem operations, to be implemented by drivers
 *
 * Callers should ensure synchronization and mutual exclusion of FS operations,
 * using the file I/O lock, as required to maintain consistency.
 * Internal driver operations should use and respect these locks to synchronize
 * with external code when required.
 *
 * Drivers wishing to handle synchronization internally must set
 * UKFS_MODE_NOIOLOCK in their mode constraints, in which case callers must not
 * use or rely on the file I/O locks for synchronization.
 */

/**
 * Filesystem-persistent data of a special file (char, block, fifo, socket).
 */
struct uk_fs_specfile {
	unsigned int major;
	unsigned int minor;
	unsigned int mode;
	uint32_t uid;
	uint32_t gid;
};

/* Flags affecting the behavior of lookup; see docstring below for details */
#define UKFS_LOOKUP_IGNMNT    1
#define UKFS_LOOKUP_NO_MNTAUX 2
#define UKFS_LOOKUP_NO_SYMAUX 4

/* (Partial) success return codes from lookup */
#define UKFS_SUCCESS    0
#define UKFS_STOP_NOD   1
#define UKFS_STOP_END   2
#define UKFS_STOP_MNT   3
#define UKFS_STOP_SYM   4
#define UKFS_STOP_SPEC  5
#define UKFS_STOP_FILE  6

/**
 * Output structure for lookup.
 */
union uk_fs_lookup_out {
	struct {
		const struct uk_file *target; /* Main output file */
		const struct uk_file *aux; /* Auxiliary output */
	};
	struct uk_fs_specfile special; /* Special file description */
};

/**
 * Lookup relative `path` of length `len` under filesystem node `f`.
 *
 * Multiple consecutive '/' are treated as a single separator, and trailing '/'
 * are ignored.
 *
 * Target path MAY contain an arbitrary number of subdirectories, and drivers
 * SHOULD walk as much of the path as practical in a single call.
 * Drivers MAY, however, walk a single level, or none at all.
 *
 * When processing multiple path components in a single call, drivers MUST stop
 * when encountering '..' for any but the first component, unless as specified
 * below. This is done to prevent escape from the current filesystem root.
 * A driver MAY omit stopping if it can ensure that the parent directory being
 * looked up cannot be the filesystem root (e.g., it has no active references).
 *
 * If no progress can be made without blocking for I/O, a call MAY return
 * UKFS_STOP_NOD, under the understanding that a future call could progress.
 * In such a case, the driver MUST ensure to set the POLLIN events on the
 * directory in question when progress can again be made without blocking.
 *
 * On success, `out.target` is set to a live reference to the lookup target.
 *
 * On partial lookup, `prog` is set to the position of the first non-matching
 * component of `path`, one of the UKFS_STOP_* constants is returned, and `out`
 * is set accordingly:
 * - UKFS_STOP_MNT: filesystem mount point reached
 *   - occurs when looking up a mount point in its parent dir, as well as when
 *     looking up '..' in a directory that is itself grafted
 *   - `out.target`: mounted node
 *   - `out.aux`: mount point node
 * - UKFS_STOP_SYM: symlink reached
 *   - `out.target`: symlink
 *   - `out.aux`: directory containing the symlink
 * - UKFS_STOP_END: directory component of `path` matches filesystem leaf node
 *   (i.e., not directory or symlink); lookup end
 * - UKFS_STOP_NOD: fs node reached, further lookups possible starting from it;
 *   returned when the driver cannot or will not walk further within that call.
 *   - `out.target`: last matched fs node
 * - UKFS_STOP_SPEC: special file (block, char, socket, fifo) reached
 *   - `out.special`: description of special file
 * - UKFS_STOP_FILE: non-filesystem (pseudo-)file reached
 *   - `out.target`: live (pseudo-)file reference
 *
 * If the target path does not exist, return -ENOENT.
 *
 * Behavior is influenced by bits set in `flags`:
 * - UKFS_LOOKUP_IGNMNT: ignore mounted filesystems and perform lookups on the
 *   referenced mount point itself
 * - UKFS_LOOKUP_NO_MNTAUX: do not output a reference to the mount point node
 *   in `out.aux`
 * - UKFS_LOOKUP_NO_SYMAUX: do not output a reference to the parent directory
 *   of a symlink in `out.aux`
 *
 * @param f Filesystem node under which to perform the lookup
 * @param path Path to lookup; does not need terminating NUL
 * @param len Length of `path`
 * @param flags Flags controlling lookup behavior
 * @param[out] out Output structure
 * @param[out] prog Number of bytes of `path` processed
 *
 * @return
 *  == 0: (UKFS_SUCCESS) Success
 *   < 0: Error; negative errno returned
 *   > 0: Stopped; one of UKFS_STOP_* constants
 */
typedef int (*uk_fs_lookup_func)(const struct uk_file *f,
				 const char *path, size_t len,
				 unsigned int flags,
				 union uk_fs_lookup_out *out,
				 size_t *prog);

/**
 * Filesystem path, length included; used as return value
 */
struct uk_fs_path {
	const char *s; /* NUL-terminated string containing path */
	size_t len; /* Length of string, same as returned by strlen(.s) */
};

/**
 * Get the path pointed to by symlink `f` as efficiently as possible.
 *
 * The returned string is guaranteed a lifetime at least as long as `f`.
 * This operation should avoid any memory allocation or copying.
 * Callers must treat returned memory as immutable.
 * This operation cannot fail.
 *
 * @param f Symlink to read
 *
 * @return
 *  Path that the symlink points to.
 */
typedef struct uk_fs_path (*uk_fs_readlink_func)(const struct uk_file *f);

/**
 * Retrieve the directory entries of directory `f` starting with the entry
 * number in `*curp`, updating `*curp` along the way on success.
 *
 * The ordering of entries is driver-dependent, but guaranteed to be stable
 * while directory state does not change.
 *
 * Output format is the same as used by the Linux `getdents64` syscall.
 * If the call fails or returns 0, `*curp` is left untouched.
 *
 * @param f Directory to list the entries of
 * @param[in,out] curp Pointer to entry index
 * @param[out] buf Buffer to write directory entries in
 * @param len Length of buf
 *
 * @return
 *   > 0: Number of bytes output
 *  == 0: `*curp` beyond end of directory
 *   < 0: Negative error code on failure
 */
typedef ssize_t (*uk_fs_listdir_func)(const struct uk_file *f, size_t *curp,
				      void *buf, size_t len);

/**
 * Union argument for a target to the create operation.
 */
union uk_fs_create_target {
	const char *path;
	const struct uk_file *file;
	const struct uk_fs_specfile *special;
};

/* Convenience macro for when the target argument is not required */
#define UKFS_NOTARGET ((union uk_fs_create_target){ NULL })

/**
 * Create a new filesystem entry with `name` in directory `f` with mode `mode`.
 *
 * If `name` exists, it is atomically replaced with the new file, unless the
 * O_EXCL flag is present in `flags`, in which case the call fails with -EEXIST.
 *
 * If O_TMPFILE flag is set, `name` is ignored and a new anonymous filesystem
 * node is created and returned. This node exists within the same filesystem as
 * `f` and can be linked in the future like any other hardlink.
 *
 * The file type to be created is determined by `mode & S_IFMT`:
 * - S_IFDIR: a new directory; target is ignored
 * - S_IFREG: a new empty file; target is ignored
 * - S_IFLNK: a new symbolic link;
 *   target.path is a NUL-terminated target path for the new symlink
 * - S_IFIFO/S_IFSOCK/S_IFCHR/S_IFBLK: link in a special file;
 *   target.special points to a description of the special file to be created
 * - S_IFMT (all bits set): hard link;
 *   target.file is a file on the same filesystem instance
 * - 0 (all bits clear): link in a live (pseudo-)file
 *   target.file is an arbitrary live file
 * Drivers may support only a subset of file types.
 *
 * @param f Directory under which to create the new file
 * @param name New file name
 * @param len  Length of `name`
 * @param mode New file mode, format according to stat
 * @param flags Flags
 * @param target Target of create operation
 *
 * @return
 *  `mode & S_IFMT` in (S_IFDIR, S_IFREG, S_IFLNK, S_IFMT):
 *    !PTRISERR: Live reference to newly created file (or in case of a hardlink,
 *               a duplicate reference to target)
 *     PTRISERR: Negative error code encoded in return
 *  `mode & S_IFMT` in (S_IFCHR, S_IFBLK, S_IFIFO, S_IFSOCK, 0):
 *         NULL: Success
 *     PTRISERR: Negative error code encoded in return
 */
typedef
const struct uk_file *(*uk_fs_create_func)(const struct uk_file *f,
					   const char *name, size_t len,
					   unsigned int mode, int flags,
					   union uk_fs_create_target target);

/* Flags restricting the behavior of uk_fs_unlink; see docstring for details */
#define UKFS_UNLINK_DIR   1
#define UKFS_UNLINK_EMPTY 2
#define UKFS_UNLINK_NODIR 4

/* Convenience to mimic behavior of rmdir() */
#define UKFS_UNLINK_RMDIR (UKFS_UNLINK_DIR | UKFS_UNLINK_EMPTY)

/**
 * Unlink the entry with `name` from directory `f`.
 *
 * By default attempt to remove all types of files, including directories.
 * If supported, this may remove non-empty directories recursively as well.
 * Values in `flags` restrict this behavior:
 * - UKFS_UNLINK_NODIR: fail if `name` is a directory
 * - UKFS_UNLINK_DIR: fail if `name` is not a directory
 * - UKFS_UNLINK_EMPTY: fail if `name` is a non-empty directory
 * - UKFS_UNLINK_RMDIR: equivalent to DIR and EMPTY together
 *
 * @param f Directory containing `name`
 * @param name Name of filesystem node to unlink
 * @param len Length of `name`
 * @param flags Flags restricting behavior
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code on failure
 */
typedef int (*uk_fs_unlink_func)(const struct uk_file *f,
				 const char *name, size_t len,
				 unsigned int flags);

/**
 * Rename file `name` under directory `f` to `dname` under `dest`.
 *
 * Both source and destination must reside on the same filesystem.
 * If the destination exists, it will be atomically replaced, so that there is
 * no point at which the destination can be seen as missing. However, there may
 * be a window in which both source and destination refer to the file being
 * renamed.
 *
 * If both source and destination refer to the same file (or to hardlinks of the
 * same file), rename does nothing and returns success.
 *
 * If any active references are held to the filesystem node(s) being moved,
 * these must remain valid during and after the `rename()`, continuing to
 * reference the renamed filesystem nodes at their new location.
 *
 * The `flags` argument modifies behavior:
 * - RENAME_EXCHANGE: Atomically exchange source and destination
 * - RENAME_NOREPLACE: Fail if the destination already exists
 * (these may not be specified together)
 *
 * @param f Source directory for the rename
 * @param name Name of source filesystem node
 * @param nlen Length of `name`
 * @param dest Destination directory for the rename
 * @param dname Name of destination filesystem node
 * @param dlen Length of `dname`
 * @param flags Flags controlling behavior
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code on failure
 */
typedef int (*uk_fs_rename_func)(const struct uk_file *f,
				 const char *name, size_t nlen,
				 const struct uk_file *dest,
				 const char *dname, size_t dlen,
				 unsigned int flags);

/**
 * Graft the directory `f` into the filesystem of `ref` in its position.
 *
 * Grafting is the complementary, yet orthogonal, counterpart to mounting, being
 * a runtime-volatile property of a filesystem instance that influences the
 * behavior of lookup: lookups of `f`'s parent should instead return the parent
 * of `ref`.
 *
 * This function is guaranteed to be called at most once on a directory
 * instance, and not concurrent with other operations.
 * Typically this function is called on the root of a new (bind) mount when it
 * is being attached to the filesystem.
 *
 * This operation may fail with -ENOSYS if not supported by the driver.
 *
 * @param f Directory to graft
 * @param ref Graft point; directory to delegate parent lookups to
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code on failure
 */
typedef int (*uk_fs_graft_func)(const struct uk_file *f,
				const struct uk_file *ref);

/**
 * Control the mount point status of filesystem node `f`.
 *
 * The mount point status is a runtime-volatile property of a filesystem
 * instance that influences the behavior of lookup(); it has no correspondence
 * in persistent storage.
 *
 * Operation is determined by `*target`:
 *  != NULL: mount `*target` on `f`
 *  == NULL: unmount `f`, write previous mount in `*target`
 *
 * @param f Filesystem node to perform (un)mount on
 * @param[in,out] target Mount operation target
 *
 * @return
 *  == 0: Success
 *  -EEXIST: mount attempted and `f` is already a mount point
 *  -ENOENT: unmount attempted but `f` is not a mount point
 *  -ENOSYS: mounting not supported
 */
typedef int (*uk_fs_mount_func)(const struct uk_file *f,
				const struct uk_file **target);

/**
 * Create a new filesystem instance for the filesystem containing `f`, using
 * mount options specified in `flags` and/or `data`, and return a reference to
 * the same file as `f` on this new binding.
 *
 * Drivers not wishing to support mounting a volume multiple times, including
 * using bind mounts, may return -ENOSYS.
 *
 * @param f Filesystem node to rebind
 * @param flags New mount flags
 * @param data Driver-specific additional options
 *
 * @return
 *  !PTRISERR: Reference to `f` on the new filesystem instance
 *   PTRISERR: Negative error code encoded in return
 */
typedef const struct uk_file *(*uk_fs_rebind_func)(const struct uk_file *f,
						   unsigned long flags,
						   const void *data);

/**
 * Retrieve information about the filesystem on which `f` resides.
 *
 * Fills in `buf` with fields compatible with Linux's `statfs` syscall.
 *
 * @param f Filesystem node whose filesystem to retrieve information for
 * @param[out] buf Buffer to write filesystem stats in
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error code
 */
typedef int (*uk_fs_stat_func)(const struct uk_file *f, struct statfs *buf);

/**
 * Synchronize the filesystem containing `f` to persistent storage.
 *
 * @param f Filesystem node to perform sync on
 *
 * @return
 *  == 0: Success
 *   < 0: Negative error codes
 */
typedef int (*uk_fs_sync_func)(const struct uk_file *f);

/* Filesystem ops table */
struct uk_fs_ops {
	/* Ops targeting individual files */
	/* Lookup must be implemented by all filesystem-backed files */
	uk_fs_lookup_func lookup;
	/* Symlink ops; must be NULL for non-symlinks */
	uk_fs_readlink_func readlink;
	/* Directory ops; must be NULL for non-directories */
	uk_fs_listdir_func listdir;
	uk_fs_create_func create;
	uk_fs_unlink_func unlink;
	uk_fs_rename_func rename;
	uk_fs_graft_func graft;
	/* Mount ops, implemented by all filesystem-backed files */
	uk_fs_mount_func mount;
	uk_fs_rebind_func rebind;
	/* Ops targeting the underlying filesystem itself */
	uk_fs_stat_func stat;
	uk_fs_sync_func sync;
	/* Constraints on open file mode; these bits must always be set on open
	 * file descriptions that reference this file.
	 */
	unsigned int constraints;
};

/* Chosen to map to similar consts in posix-fd for simplicity & performance */
/* File does not support seeking */
#define UKFS_MODE_NOSEEK   010
/* Driver handles synchronization internally */
#define UKFS_MODE_NOIOLOCK 020

static inline
bool uk_fs_isfs(const struct uk_file *f)
{
	/* Only filesystem nodes have `fsops` set */
	return !!f->fsops;
}

static inline
bool uk_fs_isdir(const struct uk_file *f)
{
	/* Any filesystem node that implements `listdir` is a directory */
	return uk_fs_isfs(f) && f->fsops->listdir;
}

static inline
bool uk_fs_issym(const struct uk_file *f)
{
	/* Any filesystem node that implements `readlink` is a symlink */
	return uk_fs_isfs(f) && f->fsops->readlink;
}

/* Operations inlines */

static inline
int uk_fs_lookupat(const struct uk_file *f, const char *path, size_t len,
		   unsigned int flags, union uk_fs_lookup_out *out,
		   size_t *nout)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->lookup);
	return f->fsops->lookup(f, path, len, flags, out, nout);
}

static inline
struct uk_fs_path uk_fs_readlink(const struct uk_file *f)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->readlink);
	return f->fsops->readlink(f);
}

static inline
ssize_t uk_fs_listdir(const struct uk_file *f, size_t *curp,
		      void *buf, size_t len)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->listdir);
	return f->fsops->listdir(f, curp, buf, len);
}

static inline
const struct uk_file *uk_fs_createat(const struct uk_file *f,
				     const char *name, size_t len,
				     unsigned int mode, int flags,
				     union uk_fs_create_target target)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->create);
	return f->fsops->create(f, name, len, mode, flags, target);
}

static inline
int uk_fs_unlinkat(const struct uk_file *f,
		   const char *name, size_t len, unsigned int flags)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->unlink);
	return f->fsops->unlink(f, name, len, flags);
}

static inline
int uk_fs_renameat(const struct uk_file *f, const char *name, size_t nlen,
		   const struct uk_file *dest, const char *dname, size_t dlen,
		   unsigned int flags)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->rename);
	return f->fsops->rename(f, name, nlen, dest, dname, dlen, flags);
}

static inline
int uk_fs_graft(const struct uk_file *f, const struct uk_file *ref)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->graft);
	return f->fsops->graft(f, ref);
}

static inline
int uk_fs_mountat(const struct uk_file *f, const struct uk_file **target)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->mount);
	return f->fsops->mount(f, target);
}

static inline
const struct uk_file *uk_fs_rebind(const struct uk_file *f,
				   unsigned long flags, const void *data)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->rebind);
	return f->fsops->rebind(f, flags, data);
}

static inline
int uk_fs_stat(const struct uk_file *f, struct statfs *buf)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->stat);
	return f->fsops->stat(f, buf);
}

static inline
int uk_fs_sync(const struct uk_file *f)
{
	UK_ASSERT(f->fsops);
	UK_ASSERT(f->fsops->sync);
	return f->fsops->sync(f);
}

static inline
unsigned int uk_fs_mode_constraints(const struct uk_file *f)
{
	UK_ASSERT(f->fsops);
	return f->fsops->constraints;
}

#endif /* __UKFS_FS_H__ */
