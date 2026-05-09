/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors. */

/*
 * hostfs — VFS ops that forward to the Hyperlight host via __dispatch.
 *
 * Each vnode caches a mount-relative path (hostfs_node::path). vfscore
 * drives the usual POSIX flow; we translate each op into one or more
 * fs_* RPCs and let the host deal with physical storage, sandboxing,
 * and permissions.
 */

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <uk/essentials.h>
#include <uk/print.h>

#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/file.h>
#include <vfscore/fs.h>
#include <vfscore/uio.h>

#include "hostfs.h"

/* djb2 hash for stable inode numbers. Collisions won't break correctness
 * (vfscore's vnode cache double-checks on lookup) but they'd defeat the
 * cache — acceptable tradeoff for a demo filesystem.
 */
static uint64_t hash_path(const char *s)
{
	uint64_t h = 5381;
	for (; *s; s++)
		h = ((h << 5) + h) + (uint8_t)*s;
	return h ? h : 1; /* ino 0 is reserved */
}

/* Join a parent path + "/" + name into child. Returns 0 on success. */
static int join_path(const char *parent, const char *name,
		     char *out, size_t cap)
{
	size_t plen = strlen(parent);
	size_t nlen = strlen(name);
	size_t need = plen + 1 + nlen + 1;
	if (need > cap)
		return -ENAMETOOLONG;
	if (plen == 0) {
		memcpy(out, name, nlen + 1);
	} else {
		memcpy(out, parent, plen);
		out[plen] = '/';
		memcpy(out + plen + 1, name, nlen + 1);
	}
	return 0;
}

/* -------- vnode ops -------- */

static int hostfs_open(struct vfscore_file *fp __unused)
{
	return 0;
}

static int hostfs_close(struct vnode *vp __unused,
			struct vfscore_file *fp __unused)
{
	return 0;
}

static int hostfs_read(struct vnode *vp,
		       struct vfscore_file *fp __unused,
		       struct uio *uio, int ioflag __unused)
{
	struct hostfs_node *np = vp->v_data;
	static char rbuf[HOSTFS_CHUNK];

	if (!np)
		return EIO;
	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_resid == 0)
		return 0;

	while (uio->uio_resid > 0) {
		size_t want = (size_t)uio->uio_resid;
		if (want > sizeof(rbuf))
			want = sizeof(rbuf);

		size_t got = 0;
		int eof = 0;
		int rc = hostfs_rpc_read(np->path,
					 (uint64_t)uio->uio_offset,
					 rbuf, want, &got, &eof);
		if (rc < 0)
			return -rc;
		if (got == 0)
			break;

		int rv = vfscore_uiomove(rbuf, (int)got, uio);
		if (rv)
			return rv;

		if (eof || got < want)
			break;
	}
	return 0;
}

static int hostfs_write(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct hostfs_node *np = vp->v_data;
	static char wbuf[HOSTFS_CHUNK];

	if (!np)
		return EIO;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_offset >= LONG_MAX)
		return EFBIG;
	if (uio->uio_resid == 0)
		return 0;

	while (uio->uio_resid > 0) {
		size_t want = (size_t)uio->uio_resid;
		if (want > sizeof(wbuf))
			want = sizeof(wbuf);

		off_t this_off = uio->uio_offset;
		int rv = vfscore_uiomove(wbuf, (int)want, uio);
		if (rv)
			return rv;

		int rc = hostfs_rpc_write(np->path, (uint64_t)this_off,
					  wbuf, want,
					  (ioflag & IO_APPEND) ? 1 : 0);
		if (rc < 0)
			return -rc;

		/* Grow v_size if we wrote past the old end. */
		if ((uint64_t)this_off + want > (uint64_t)vp->v_size)
			vp->v_size = this_off + want;
	}
	return 0;
}

static int hostfs_lookup(struct vnode *dvp, const char *name,
			 struct vnode **vpp)
{
	struct hostfs_node *dnp = dvp->v_data;
	struct vnode *vp;
	char child[HOSTFS_MAX_PATH];
	int rc;

	uk_pr_info("hostfs_lookup: name=%s\n", name);
	*vpp = NULL;
	if (*name == '\0')
		return ENOENT;
	if (!dnp)
		return EIO;

	rc = join_path(dnp->path, name, child, sizeof(child));
	if (rc < 0)
		return -rc;
	uk_pr_info("hostfs_lookup: child=%s\n", child);

	struct hostfs_stat st = {0};
	rc = hostfs_rpc_stat(child, &st);
	uk_pr_info("hostfs_lookup: stat rc=%d\n", rc);
	if (rc < 0)
		return -rc;

	uint64_t ino = hash_path(child);
	/* vfscore_vget returns non-zero when the vnode was already in the
	 * cache (already populated), and 0 when a fresh vnode was
	 * allocated that we need to fill in.
	 */
	if (vfscore_vget(dvp->v_mount, ino, &vp)) {
		*vpp = vp;
		return 0;
	}
	if (!vp)
		return ENOMEM;

	struct hostfs_node *np = malloc(sizeof(*np));
	if (!np)
		return ENOMEM;
	strlcpy(np->path, child, sizeof(np->path));

	vp->v_data = np;
	vp->v_type = st.is_dir ? VDIR : VREG;
	vp->v_mode = 0777;
	vp->v_size = (off_t)st.size;

	*vpp = vp;
	return 0;
}

static int hostfs_create(struct vnode *dvp, const char *name, mode_t mode)
{
	struct hostfs_node *dnp = dvp->v_data;
	char child[HOSTFS_MAX_PATH];
	int rc;

	if (!S_ISREG(mode))
		return EINVAL;
	rc = join_path(dnp->path, name, child, sizeof(child));
	if (rc < 0)
		return -rc;

	/* Create empty file with a zero-byte write. */
	rc = hostfs_rpc_write(child, 0, "", 0, 0);
	if (rc < 0)
		return -rc;
	return 0;
}

static int hostfs_remove(struct vnode *dvp __unused, struct vnode *vp,
			 const char *name __unused)
{
	struct hostfs_node *np = vp->v_data;
	int rc = hostfs_rpc_unlink(np->path);
	return rc < 0 ? -rc : 0;
}

static int hostfs_mkdir(struct vnode *dvp, const char *name, mode_t mode __unused)
{
	struct hostfs_node *dnp = dvp->v_data;
	char child[HOSTFS_MAX_PATH];
	int rc = join_path(dnp->path, name, child, sizeof(child));
	if (rc < 0)
		return -rc;
	rc = hostfs_rpc_mkdir(child);
	return rc < 0 ? -rc : 0;
}

static int hostfs_rmdir(struct vnode *dvp __unused, struct vnode *vp,
			const char *name __unused)
{
	struct hostfs_node *np = vp->v_data;
	int rc = hostfs_rpc_unlink(np->path);
	return rc < 0 ? -rc : 0;
}

static int hostfs_readdir(struct vnode *vp, struct vfscore_file *fp,
			  struct dirent64 *dir)
{
	struct hostfs_node *np = vp->v_data;
	char name[NAME_MAX + 1];
	int is_dir = 0;

	if (!np)
		return EIO;
	if (fp->f_offset == 0) {
		dir->d_type = DT_DIR;
		strlcpy(dir->d_name, ".", sizeof(dir->d_name));
	} else if (fp->f_offset == 1) {
		dir->d_type = DT_DIR;
		strlcpy(dir->d_name, "..", sizeof(dir->d_name));
	} else {
		size_t idx = (size_t)(fp->f_offset - 2);
		int rc = hostfs_rpc_readdir(np->path, idx,
					    name, sizeof(name), &is_dir);
		if (rc < 0)
			return -rc;
		dir->d_type = is_dir ? DT_DIR : DT_REG;
		strlcpy(dir->d_name, name, sizeof(dir->d_name));
	}
	dir->d_fileno = (ino_t)fp->f_offset;
	fp->f_offset++;
	return 0;
}

static int hostfs_getattr(struct vnode *vp, struct vattr *attr)
{
	struct hostfs_node *np = vp->v_data;
	if (!np)
		return EIO;
	struct hostfs_stat st = {0};
	int rc = hostfs_rpc_stat(np->path, &st);
	if (rc < 0)
		return -rc;

	attr->va_nodeid = vp->v_ino;
	attr->va_size = (off_t)st.size;
	attr->va_type = st.is_dir ? VDIR : VREG;
	attr->va_mode = 0777;
	vp->v_size = attr->va_size;
	vp->v_type = attr->va_type;
	return 0;
}

static int hostfs_setattr(struct vnode *vp __unused, struct vattr *attr __unused)
{
	return 0;
}

static int hostfs_truncate(struct vnode *vp, off_t length)
{
	struct hostfs_node *np = vp->v_data;
	if (!np)
		return EIO;
	int rc = hostfs_rpc_truncate(np->path, (uint64_t)length);
	if (rc < 0)
		return -rc;
	vp->v_size = length;
	return 0;
}

static int hostfs_inactive(struct vnode *vp)
{
	if (vp && vp->v_data) {
		free(vp->v_data);
		vp->v_data = NULL;
	}
	return 0;
}

#define hostfs_seek     ((vnop_seek_t)vfscore_vop_nullop)
#define hostfs_ioctl    ((vnop_ioctl_t)vfscore_vop_einval)
#define hostfs_fsync    ((vnop_fsync_t)vfscore_vop_nullop)
#define hostfs_rename   ((vnop_rename_t)vfscore_vop_einval)
#define hostfs_link     ((vnop_link_t)vfscore_vop_eperm)
#define hostfs_fallocate ((vnop_fallocate_t)vfscore_vop_einval)
#define hostfs_readlink ((vnop_readlink_t)vfscore_vop_einval)
#define hostfs_symlink  ((vnop_symlink_t)vfscore_vop_eperm)
#define hostfs_poll     ((vnop_poll_t)vfscore_vop_einval)

struct vnops hostfs_vnops = {
	hostfs_open,
	hostfs_close,
	hostfs_read,
	hostfs_write,
	hostfs_seek,
	hostfs_ioctl,
	hostfs_fsync,
	hostfs_readdir,
	hostfs_lookup,
	hostfs_create,
	hostfs_remove,
	hostfs_rename,
	hostfs_mkdir,
	hostfs_rmdir,
	hostfs_getattr,
	hostfs_setattr,
	hostfs_inactive,
	hostfs_truncate,
	hostfs_link,
	(vnop_cache_t) NULL,
	hostfs_fallocate,
	hostfs_readlink,
	hostfs_symlink,
	hostfs_poll,
};
