/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Virtiofs FUSE operations interface */

#ifndef __UK_VIRTIOFS_FUSE_H__
#define __UK_VIRTIOFS_FUSE_H__

#include <stdint.h>
#include <sys/uio.h>

#include <uk/virtio_fs.h>

#include "fuse.h"

struct virtiofs_fuse_dev {
	struct uk_virtiofs_dev *dev;
	uint64_t unique;
};

/* Lifecycle messages */

/**
 * Perform FUSE session initialization on `fdev`.
 *
 * Sends the FUSE_INIT message and negotiates protocol version and flags.
 */
int virtiofs_fuse_init(struct virtiofs_fuse_dev *fdev);

/**
 * Destroy the active FUSE session on `fdev`.
 *
 * Sends the FUSE_DESTROY message.
 */
int virtiofs_fuse_destroy(struct virtiofs_fuse_dev *fdev);

/* Priority messages */

int virtiofs_fuse_forget(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, uint64_t nlookup);

int virtiofs_fuse_batch_forget(struct virtiofs_fuse_dev *fdev,
			       const struct fuse_forget_one *arg,
			       uint32_t count);

int virtiofs_fuse_interrupt(struct virtiofs_fuse_dev *fdev, uint64_t unique);

/* Truncated output structures, allowing output messages to be split into the
 * response itself + node attrs (struct fuse_attr), enabling the latter to be
 * written directly into the local node cache, saving a copy.
 */

/* Bit-compatible with struct fuse_entry_out, except for the last field */
struct fuse_entry_out_trunc {
	uint64_t	nodeid;
	uint64_t	generation;
	uint64_t	entry_valid;
	uint64_t	attr_valid;
	uint32_t	entry_valid_nsec;
	uint32_t	attr_valid_nsec;
};

union virtiofs_fuse_entry_outp {
	struct fuse_entry_out *full;
	struct fuse_entry_out_trunc *trunc;
};

/* Bit-compatible with struct fuse_attr_out, except for the last field */
struct fuse_attr_out_trunc {
	uint64_t	attr_valid;
	uint32_t	attr_valid_nsec;
	uint32_t	dummy;
};

union virtiofs_fuse_attr_outp {
	struct fuse_attr_out *full;
	struct fuse_attr_out_trunc *trunc;
};

/* Bit-compatible with struct fuse_statx_out, except for the last field */
struct fuse_statx_out_trunc {
	uint64_t	attr_valid;
	uint32_t	attr_valid_nsec;
	uint32_t	flags;
	uint64_t	spare[2];
};

union virtiofs_fuse_statx_outp {
	struct fuse_statx_out *full;
	struct fuse_statx_out_trunc *trunc;
};

/* Regular messages; in order of appearance in fuse.h */

int virtiofs_fuse_lookup(struct virtiofs_fuse_dev *fdev, uint64_t node,
			 const char *fname, size_t len,
			 union virtiofs_fuse_entry_outp out,
			 struct fuse_attr *out_attr);

int virtiofs_fuse_getattr(struct virtiofs_fuse_dev *fdev,
			  uint64_t node, uint64_t fh, uint32_t getattr_flags,
			  union virtiofs_fuse_attr_outp out,
			  struct fuse_attr *out_attr);

int virtiofs_fuse_setattr(struct virtiofs_fuse_dev *fdev, uint64_t node,
			  const struct fuse_setattr_in *in,
			  union virtiofs_fuse_attr_outp out,
			  struct fuse_attr *out_attr);

ssize_t virtiofs_fuse_readlink(struct virtiofs_fuse_dev *fdev, uint64_t node,
			       char *buf, size_t len);

int virtiofs_fuse_symlink(struct virtiofs_fuse_dev *fdev, uint64_t node,
			  const char *name, size_t namelen,
			  const char *target, size_t targlen,
			  union virtiofs_fuse_entry_outp out,
			  struct fuse_attr *out_attr);

int virtiofs_fuse_mknod(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const char *name, size_t len,
			const struct fuse_mknod_in *in,
			union virtiofs_fuse_entry_outp out,
			struct fuse_attr *out_attr);

int virtiofs_fuse_mkdir(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const char *name, size_t len,
			const struct fuse_mkdir_in *in,
			union virtiofs_fuse_entry_outp out,
			struct fuse_attr *out_attr);

int virtiofs_fuse_unlink(struct virtiofs_fuse_dev *fdev, uint64_t node,
			 const char *name, size_t len);

int virtiofs_fuse_rmdir(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const char *name, size_t len);

int virtiofs_fuse_rename(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, const char *name, size_t nlen,
			 uint64_t dest, const char *dname, size_t dlen);

int virtiofs_fuse_link(struct virtiofs_fuse_dev *fdev, uint64_t node,
		       uint64_t target, const char *name, size_t len,
		       union virtiofs_fuse_entry_outp out,
		       struct fuse_attr *out_attr);

int virtiofs_fuse_open(struct virtiofs_fuse_dev *fdev,
		       uint64_t node, uint32_t flags,
		       struct fuse_open_out *out);

ssize_t virtiofs_fuse_read(struct virtiofs_fuse_dev *fdev, uint64_t node,
			   const struct fuse_read_in *in,
			   const struct iovec *read_iov, size_t iovcnt);

ssize_t virtiofs_fuse_write(struct virtiofs_fuse_dev *fdev, uint64_t node,
			    const struct fuse_write_in *in,
			    const struct iovec *write_iov, size_t iovcnt);

int virtiofs_fuse_statfs(struct virtiofs_fuse_dev *fdev,
			 struct fuse_statfs_out *out);

int virtiofs_fuse_release(struct virtiofs_fuse_dev *fdev,
			  uint64_t node, uint64_t fh);

int virtiofs_fuse_fsync(struct virtiofs_fuse_dev *fdev,
			uint64_t node, uint64_t fh, uint32_t fsync_flags);

/* No xattr support */

int virtiofs_fuse_flush(struct virtiofs_fuse_dev *fdev,
			uint64_t node, uint64_t fh);

ssize_t virtiofs_fuse_readdir(struct virtiofs_fuse_dev *fdev, uint64_t node,
			      uint64_t fh, uint64_t off,
			      void *buf, uint64_t len);

/* No lock ops support */

int virtiofs_fuse_access(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, uint32_t mask);

int virtiofs_fuse_create(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, const char *name, size_t len,
			 const struct fuse_create_in *in,
			 union virtiofs_fuse_entry_outp entry,
			 struct fuse_attr *entry_attr,
			 struct fuse_open_out *out);

/* No bmap support */

/* No ioctl support */

/* No poll/notify_reply */

int virtiofs_fuse_fallocate(struct virtiofs_fuse_dev *fdev,
			    uint64_t node, uint64_t fh,
			    uint64_t offset, uint64_t len, uint32_t mode);

/* No readdirplus */

int virtiofs_fuse_rename2(struct virtiofs_fuse_dev *fdev,
			  uint64_t node, const char *name, size_t nlen,
			  uint64_t dest, const char *dname, size_t dlen,
			  uint32_t flags);

/* No lseek (we keep our own file position) */

ssize_t virtiofs_fuse_copy_file_range(struct virtiofs_fuse_dev *fdev,
				      uint64_t node,
				      const struct fuse_copy_file_range_in *in);

int virtiofs_fuse_setupmapping(struct virtiofs_fuse_dev *fdev, uint64_t node,
			       const struct fuse_setupmapping_in *in);

int virtiofs_fuse_removemapping(struct virtiofs_fuse_dev *fdev,
				const struct fuse_removemapping_one *arg,
				uint32_t count);

int virtiofs_fuse_syncfs(struct virtiofs_fuse_dev *fdev);

/* No tmpfile support */

int virtiofs_fuse_statx(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const struct fuse_statx_in *in,
			union virtiofs_fuse_statx_outp out,
			struct fuse_statx *out_statx);

#endif /* __UK_VIRTIOFS_FUSE_H__ */
