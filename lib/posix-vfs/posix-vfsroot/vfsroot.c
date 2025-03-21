/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE

/* VFS root file */

#include <sys/stat.h>

#include <uk/atomic.h>
#include <uk/spinlock.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/fs.h>
#include <uk/fs/common-ops.h>
#include <uk/fs/dirent.h>
#include <uk/posix-fd.h>
#include <uk/posix-vfsroot.h>

static const char ROOT_VOLID[] = "root_vol";

static const struct uk_file *rootmnt;
static struct uk_spinlock umount_lock = UK_SPINLOCK_INITIALIZER();

static
int root_getstat(const struct uk_file *f __maybe_unused,
		 unsigned int mask __unused, struct uk_statx *arg)
{
	UK_ASSERT(f->vol == ROOT_VOLID);
	*arg = (struct uk_statx){
		.stx_mask = UK_STATX_NLINK | UK_STATX_TYPE | UK_STATX_MODE |
			    UK_STATX_UID | UK_STATX_GID | UK_STATX_INO,
		.stx_blksize = 4096,
		.stx_nlink = 1,
		.stx_uid = 0,
		.stx_gid = 0,
		.stx_mode = S_IFDIR | 0777,
		.stx_ino = 0,
	};
	return 0;
}

static const struct uk_file_ops rootfops = {
	.read = uk_file_nop_read,
	.write = uk_file_nop_write,
	.getstat = root_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl
};

static const struct statfs rootfs_stat = {
	.f_type = 0x544f4f52 /* ASCII "ROOT" as little-endian u32 */
	/* Rest of fields zero */
};

static
int root_fsstat(const struct uk_file *f __maybe_unused, struct statfs *buf)
{
	UK_ASSERT(f == &uk_posix_vfsroot_file);
	*buf = rootfs_stat;
	return 0;
}

static
int root_lookup(const struct uk_file *f,
		const char *path, size_t len, unsigned int flags,
		union uk_fs_lookup_out *out, size_t *nout)
{
	const struct uk_file *mnt = NULL;
	size_t cur = 0;

	UK_ASSERT(f == &uk_posix_vfsroot_file);
	/* Skip any number of '', '.', and '..' leading components */
	while (cur < len) {
		switch (path[cur]) {
		case '/':
			cur++;
			continue;
		case '.':
			if (cur + 1 == len) {
				cur++;
				break;
			}
			/* cur + 1 < len because cur < len && cur + 1 != len */
			switch (path[cur + 1]) {
			case '/':
				cur += 2;
				continue;
			case '.':
				if (cur + 2 == len) {
					cur += 2;
					break;
				}
				/* cur + 2 < len, as above */
				if (path[cur + 2] == '/') {
					cur += 3;
					continue;
				}
			}
		}
		break;
	}

	if (!(flags & UKFS_LOOKUP_IGNMNT)) {
		uk_spin_lock(&umount_lock);
		mnt = rootmnt;
		if (mnt)
			uk_file_acquire(mnt);
		uk_spin_unlock(&umount_lock);
	}

	if (mnt) {
		if (!(flags & UKFS_LOOKUP_NO_MNTAUX)) {
			uk_file_acquire(f);
			out->aux = f;
		}
		out->target = mnt;
		*nout = cur;
		return UKFS_STOP_MNT;
	}
	if (cur == len) {
		uk_file_acquire(f);
		out->target = f;
		return UKFS_SUCCESS;
	}
	return -ENOENT;
}

static
int root_mount(const struct uk_file *f __maybe_unused,
	       const struct uk_file **target)
{
	const struct uk_file *t = *target;

	UK_ASSERT(f == &uk_posix_vfsroot_file);
	if (t) {
		const struct uk_file *prev = NULL;

		/* We optimistically acquire before writing in to ensure that
		 * the rootmnt ref is counted from the moment it is visible.
		 */
		uk_file_acquire(t);
		if (unlikely(!uk_compare_exchange_n(&rootmnt, &prev, t))) {
			uk_file_release(t);
			return -EEXIST;
		}
	} else {
		uk_spin_lock(&umount_lock);
		t = uk_exchange_n(&rootmnt, NULL);
		uk_spin_unlock(&umount_lock);
		if (unlikely(!t))
			return -ENOENT;
		*target = t;
	}
	return 0;
}

static
ssize_t root_listdir(const struct uk_file *f __maybe_unused, size_t *curp,
		     void *buf, size_t len)
{
	struct uk_fs_dirent *out = buf;
	size_t p = *curp;
	size_t rlen = 0;
	size_t sz;

	UK_ASSERT(f == &uk_posix_vfsroot_file);
	/* Output '.' & '..' dentries pointing to self */
	while (p < 2) {
		sz = UKFS_DIRENT_RECLEN(p ? 2 : 1);
		if (unlikely(sz > len)) {
			if (rlen)
				break;
			return -EINVAL;
		}
		if (!p)
			UKFS_DIRENT_OUT_DOT(out, 0);
		else
			UKFS_DIRENT_OUT_DOTDOT(out, 0);
		rlen += sz;
		len -= sz;
		out = UKFS_DIRENT_NEXT(out, sz);
		p++;
	}
	*curp = p;
	return rlen;
}

static const struct uk_fs_ops rootfsops = {
	.lookup = root_lookup,
	.readlink = NULL,
	.listdir = root_listdir,
	.create = uk_fs_common_rofs_create,
	.unlink = uk_fs_common_rofs_unlink,
	.rename = uk_fs_common_rofs_rename,
	.graft = uk_fs_common_nop_graft,
	.mount = root_mount,
	.rebind = uk_fs_common_nop_rebind,
	.stat = root_fsstat,
	.sync = uk_fs_common_nop_sync,
	.constraints = UKFS_MODE_NOIOLOCK
};

static
struct uk_file_state root_fstate = UK_FILE_STATE_EVENTS_INITIALIZER(root_fstate,
	UKFD_POLLIN | UKFD_POLLOUT);

/* uk_posix_vfsroot_file starts off with a refcount of 3:
 * - this static reference
 * - reference in uk_posix_vfsroot_ofile
 * - .fsroot reference in initial VFS state
 */
static uk_file_refcnt root_refcnt = UK_FILE_REFCNT_INITIALIZER_V(root_refcnt,
								 3);

const struct uk_file uk_posix_vfsroot_file = {
	.vol = ROOT_VOLID,
	.node = NULL,
	.ops = &rootfops,
	.fsops = &rootfsops,
	.refcnt = &root_refcnt,
	.state = &root_fstate,
	._release = uk_file_static_release
};

/* uk_posix_vfsroot_ofile starts off with a refcount of 2:
 * - this static reference
 * - .cwd reference in initial VFS state
 */
struct uk_ofile uk_posix_vfsroot_ofile = {
	.file = &uk_posix_vfsroot_file,
	.lock = UK_MUTEX_INITIALIZER(uk_posix_vfsroot_ofile.lock),
	.mode = O_RDONLY | O_PATH | UKFD_O_NAMED | UKFD_O_NOIOLOCK,
	.refcnt = UK_REFCOUNT_INITIALIZER(2),
	.name = "/"
};
