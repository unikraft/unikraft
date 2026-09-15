/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors. */

/*
 * hostfs — vnode operations for host filesystem pass-through.
 *
 * Every file operation is forwarded to the host via hl_hcall_* functions.
 */

#define _GNU_SOURCE

#include <uk/essentials.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/uio.h>
#include <vfscore/file.h>

#include <hyperlight-x86/hcall.h>

#include "hostfs.h"

/* ── Helpers ──────────────────────────────────────────────────────── */

/*
 * Build the host-relative path for a child of a directory node.
 * Result is written into `out` (at most `sz` bytes including NUL).
 */
static void build_path(char *out, size_t sz,
		       const struct hostfs_node *dir, const char *name)
{
	if (dir->hf_path[0] == '\0') {
		/* Root directory — child is just "name". */
		strlcpy(out, name, sz);
	} else {
		snprintf(out, sz, "%s/%s", dir->hf_path, name);
	}
}

/*
 * Call fs_stat(mount_idx, path) and decode the packed result.
 *
 * Returns 0 on success, positive errno on failure.
 * On success fills size, mode, is_dir, is_file.
 */
static int host_stat(int mount_idx, const char *path, uint64_t *size,
		     uint32_t *mode, int *is_dir, int *is_file)
{
	struct hl_param p[2];
	__u8 buf[32];
	__sz len;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = path;
	p[1].str.len = strlen(path);

	if (hl_hcall_vecbytes("fs_stat", p, 2, buf, sizeof(buf), &len) < 0)
		return EIO;

	if (len < 4)
		return EIO;

	__s32 status = (__s32)(buf[0] | (buf[1] << 8) |
			       (buf[2] << 16) | (buf[3] << 24));
	if (status < 0)
		return -status;

	if (len < 18)
		return EIO;

	if (size)
		*size = buf[4] | ((uint64_t)buf[5] << 8) |
			((uint64_t)buf[6] << 16) | ((uint64_t)buf[7] << 24) |
			((uint64_t)buf[8] << 32) | ((uint64_t)buf[9] << 40) |
			((uint64_t)buf[10] << 48) | ((uint64_t)buf[11] << 56);
	if (mode)
		*mode = buf[12] | (buf[13] << 8) |
			(buf[14] << 16) | (buf[15] << 24);
	if (is_dir)
		*is_dir = buf[16];
	if (is_file)
		*is_file = buf[17];

	return 0;
}

/* ── lookup ───────────────────────────────────────────────────────── */

static int
hostfs_lookup(struct vnode *dvp, const char *name, struct vnode **vpp)
{
	struct hostfs_node *dnp = dvp->v_data;
	struct hostfs_node *np;
	struct vnode *vp;
	char path[1024];
	uint64_t size = 0;
	uint32_t mode = 0;
	int is_dir = 0, is_file = 0;
	int err;

	build_path(path, sizeof(path), dnp, name);

	err = host_stat(dnp->hf_mount_idx, path, &size, &mode,
			&is_dir, &is_file);
	if (err)
		return err;

	np = malloc(sizeof(*np));
	if (!np)
		return ENOMEM;

	strlcpy(np->hf_path, path, sizeof(np->hf_path));
	np->hf_type = is_dir ? VDIR : VREG;
	np->hf_mount_idx = dnp->hf_mount_idx;

	/* Allocate a vnode via the VFS layer. Use a monotonic counter
	 * as inode number since hostfs has no real inodes. */
	static uint64_t next_ino = 1;

	if (vfscore_vget(dvp->v_mount, next_ino++, &vp)) {
		/* found in cache — reuse */
		*vpp = vp;
		if (vp->v_data)
			free(np);
		else
			vp->v_data = np;
		vp->v_type = np->hf_type;
		vp->v_mode = mode;
		vp->v_size = size;
		return 0;
	}
	if (!vp) {
		free(np);
		return ENOMEM;
	}

	vp->v_data = np;
	vp->v_type = np->hf_type;
	vp->v_mode = mode;
	vp->v_size = size;
	*vpp = vp;

	return 0;
}

/* ── create ───────────────────────────────────────────────────────── */

static int
hostfs_create(struct vnode *dvp, const char *name, mode_t mode __unused)
{
	struct hostfs_node *dnp = dvp->v_data;
	char path[1024];

	build_path(path, sizeof(path), dnp, name);

	/*
	 * Create by writing zero bytes at offset 0.  The host's
	 * fs_write_bytes will create the file with O_CREAT.
	 */
	struct hl_param p[5];
	__s32 ret;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = dnp->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = path;
	p[1].str.len = strlen(path);
	p[2].type = HL_PV_HLULONG;
	p[2].u64_val = 0; /* offset */
	p[3].type = HL_PV_HLINT;
	p[3].i32_val = 0; /* not append */
	p[4].type = HL_PV_HLVECBYTES;
	p[4].vec.ptr = NULL;
	p[4].vec.len = 0;

	if (hl_hcall_int("fs_write_bytes", p, 5, &ret) < 0)
		return EIO;

	return ret < 0 ? -ret : 0;
}

/* ── read ─────────────────────────────────────────────────────────── */

static int
hostfs_read(struct vnode *vp __unused, struct vfscore_file *fp __unused,
	    struct uio *uio, int ioflag __unused)
{
	struct hostfs_node *np = vp->v_data;

	while (uio->uio_resid > 0) {
		size_t want = (size_t)uio->uio_resid;

		if (want > hostfs_io.chunk)
			want = hostfs_io.chunk;

		struct hl_param p[4];

		p[0].type = HL_PV_HLINT;
		p[0].i32_val = np->hf_mount_idx;
		p[1].type = HL_PV_HLSTRING;
		p[1].str.ptr = np->hf_path;
		p[1].str.len = strlen(np->hf_path);
		p[2].type = HL_PV_HLULONG;
		p[2].u64_val = uio->uio_offset;
		p[3].type = HL_PV_HLULONG;
		p[3].u64_val = want;

		__u8 *buf = hostfs_io.rbuf;
		__sz len;

		if (hl_hcall_vecbytes("fs_read_bytes", p, 4,
				      buf, hostfs_io.result_max, &len) < 0)
			return EIO;

		if (len < 4)
			return EIO;

		__s32 status = (__s32)(buf[0] | (buf[1] << 8) |
				      (buf[2] << 16) | (buf[3] << 24));
		if (status < 0)
			return -status;

		size_t got = len - 4;

		if (got == 0)
			break; /* EOF */

		/* Copy data to the uio buffers. */
		size_t remaining = got;
		const __u8 *src = buf + 4;

		while (remaining > 0 && uio->uio_iovcnt > 0) {
			struct iovec *iov = uio->uio_iov;
			size_t chunk = remaining;

			if (chunk > iov->iov_len)
				chunk = iov->iov_len;

			memcpy(iov->iov_base, src, chunk);
			src += chunk;
			remaining -= chunk;

			iov->iov_base = (char *)iov->iov_base + chunk;
			iov->iov_len -= chunk;
			uio->uio_resid -= chunk;
			uio->uio_offset += chunk;

			if (iov->iov_len == 0) {
				uio->uio_iov++;
				uio->uio_iovcnt--;
			}
		}

		if (got < want)
			break; /* EOF (short read) */
	}

	return 0;
}

/* ── write ────────────────────────────────────────────────────────── */

static int
hostfs_write(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct hostfs_node *np = vp->v_data;
	int append = (ioflag & IO_APPEND) ? 1 : 0;

	while (uio->uio_resid > 0) {
		__u8 *wbuf = hostfs_io.wbuf;
		size_t want = (size_t)uio->uio_resid;

		if (want > hostfs_io.chunk)
			want = hostfs_io.chunk;

		/* Gather from uio into wbuf. */
		size_t filled = 0;

		while (filled < want && uio->uio_iovcnt > 0) {
			struct iovec *iov = uio->uio_iov;
			size_t chunk = want - filled;

			if (chunk > iov->iov_len)
				chunk = iov->iov_len;

			memcpy(wbuf + filled, iov->iov_base, chunk);
			filled += chunk;

			iov->iov_base = (char *)iov->iov_base + chunk;
			iov->iov_len -= chunk;

			if (iov->iov_len == 0) {
				uio->uio_iov++;
				uio->uio_iovcnt--;
			}
		}

		struct hl_param p[5];
		__s32 ret;

		p[0].type = HL_PV_HLINT;
		p[0].i32_val = np->hf_mount_idx;
		p[1].type = HL_PV_HLSTRING;
		p[1].str.ptr = np->hf_path;
		p[1].str.len = strlen(np->hf_path);
		p[2].type = HL_PV_HLULONG;
		p[2].u64_val = uio->uio_offset;
		p[3].type = HL_PV_HLINT;
		p[3].i32_val = append;
		p[4].type = HL_PV_HLVECBYTES;
		p[4].vec.ptr = wbuf;
		p[4].vec.len = filled;

		if (hl_hcall_int("fs_write_bytes", p, 5, &ret) < 0)
			return EIO;

		if (ret < 0)
			return -ret;

		uio->uio_resid -= filled;
		uio->uio_offset += filled;
	}

	/* Update vnode size. */
	if (uio->uio_offset > vp->v_size)
		vp->v_size = uio->uio_offset;

	return 0;
}

/* ── readdir ──────────────────────────────────────────────────────── */

static int
hostfs_readdir(struct vnode *vp, struct vfscore_file *fp,
	       struct dirent64 *dir)
{
	struct hostfs_node *np = vp->v_data;
	struct hl_param p[2];
	__u8 *buf = hostfs_io.rbuf;
	__sz len;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = np->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = np->hf_path;
	p[1].str.len = strlen(np->hf_path);

	if (hl_hcall_vecbytes("fs_list", p, 2, buf, hostfs_io.result_max,
			      &len) < 0)
		return EIO;

	if (len < 8)
		return EIO;

	__s32 status = (__s32)(buf[0] | (buf[1] << 8) |
			       (buf[2] << 16) | (buf[3] << 24));
	if (status < 0)
		return -status;

	uint32_t count = buf[4] | (buf[5] << 8) |
			 (buf[6] << 16) | (buf[7] << 24);

	/*
	 * fp->f_offset tracks which entry we're on (0-based index).
	 * We need to skip entries until we reach fp->f_offset, then
	 * return one entry and increment fp->f_offset.
	 */
	uint32_t target = (uint32_t)fp->f_offset;

	if (target >= count)
		return ENOENT;

	/* Walk the packed entries to find the target. */
	size_t pos = 8;
	uint32_t idx = 0;

	while (idx < count && pos < len) {
		if (pos + 3 > len)
			break;

		__u8 is_dir = buf[pos];
		uint16_t name_len = buf[pos + 1] | (buf[pos + 2] << 8);

		pos += 3;

		if (pos + name_len > len)
			break;

		if (idx == target) {
			size_t copy_len = name_len;

			if (copy_len >= sizeof(dir->d_name))
				copy_len = sizeof(dir->d_name) - 1;

			memcpy(dir->d_name, buf + pos, copy_len);
			dir->d_name[copy_len] = '\0';
			dir->d_type = is_dir ? DT_DIR : DT_REG;
			dir->d_fileno = target + 1;
			fp->f_offset = target + 1;
			return 0;
		}

		pos += name_len;
		idx++;
	}

	return ENOENT;
}

/* ── getattr ──────────────────────────────────────────────────────── */

static int
hostfs_getattr(struct vnode *vp, struct vattr *attr)
{
	struct hostfs_node *np = vp->v_data;
	uint64_t size = 0;
	uint32_t mode = 0;
	int is_dir = 0, is_file = 0;
	int err;

	err = host_stat(np->hf_mount_idx, np->hf_path, &size, &mode,
			&is_dir, &is_file);
	if (err)
		return err;

	memset(attr, 0, sizeof(*attr));
	attr->va_type = is_dir ? VDIR : VREG;
	attr->va_mode = mode;
	attr->va_size = size;
	attr->va_nodeid = vp->v_ino;

	vp->v_size = size;
	vp->v_mode = mode;

	return 0;
}

/* ── setattr ─────────────────────────────────────────────────────── */

static int
hostfs_setattr(struct vnode *vp, struct vattr *attr)
{
	struct hostfs_node *np = vp->v_data;

	if (attr->va_mask & AT_MODE) {
		struct hl_param p[3];
		__s32 ret;

		p[0].type = HL_PV_HLINT;
		p[0].i32_val = np->hf_mount_idx;
		p[1].type = HL_PV_HLSTRING;
		p[1].str.ptr = np->hf_path;
		p[1].str.len = strlen(np->hf_path);
		p[2].type = HL_PV_HLINT;
		p[2].i32_val = (int)attr->va_mode;

		if (hl_hcall_int("fs_chmod", p, 3, &ret) < 0)
			return EIO;

		if (ret < 0)
			return -ret;

		vp->v_mode = attr->va_mode;
	}

	return 0;
}

/* ── mkdir ────────────────────────────────────────────────────────── */

static int
hostfs_mkdir(struct vnode *dvp, const char *name, mode_t mode __unused)
{
	struct hostfs_node *dnp = dvp->v_data;
	char path[1024];
	struct hl_param p[2];
	__s32 ret;

	build_path(path, sizeof(path), dnp, name);

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = dnp->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = path;
	p[1].str.len = strlen(path);

	if (hl_hcall_int("fs_mkdir", p, 2, &ret) < 0)
		return EIO;

	return ret < 0 ? -ret : 0;
}

/* ── remove ───────────────────────────────────────────────────────── */

static int
hostfs_remove(struct vnode *dvp __unused, struct vnode *vp,
	      const char *name __unused)
{
	struct hostfs_node *np = vp->v_data;
	struct hl_param p[2];
	__s32 ret;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = np->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = np->hf_path;
	p[1].str.len = strlen(np->hf_path);

	if (hl_hcall_int("fs_unlink", p, 2, &ret) < 0)
		return EIO;

	return ret < 0 ? -ret : 0;
}

/* ── rmdir ────────────────────────────────────────────────────────── */

static int
hostfs_rmdir(struct vnode *dvp, struct vnode *vp, const char *name)
{
	return hostfs_remove(dvp, vp, name);
}

/* ── rename ───────────────────────────────────────────────────────── */

static int
hostfs_rename(struct vnode *dvp, struct vnode *vp, const char *sname __unused,
	      struct vnode *tdvp, struct vnode *tvp __unused, const char *tname)
{
	struct hostfs_node *dnp = dvp->v_data;
	struct hostfs_node *np = vp->v_data;
	struct hostfs_node *tdnp = tdvp->v_data;
	char new_path[1024];

	build_path(new_path, sizeof(new_path), tdnp, tname);

	struct hl_param p[3];
	__s32 ret;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = dnp->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = np->hf_path;
	p[1].str.len = strlen(np->hf_path);
	p[2].type = HL_PV_HLSTRING;
	p[2].str.ptr = new_path;
	p[2].str.len = strlen(new_path);

	if (hl_hcall_int("fs_rename", p, 3, &ret) < 0)
		return EIO;

	if (ret < 0)
		return -ret;

	/* Update the node's path to reflect the rename. */
	strlcpy(np->hf_path, new_path, sizeof(np->hf_path));

	return 0;
}

/* ── link ─────────────────────────────────────────────────────────── */

static int
hostfs_link(struct vnode *tdvp, struct vnode *svp, const char *name)
{
	struct hostfs_node *snp = svp->v_data;
	struct hostfs_node *tdnp = tdvp->v_data;
	char dst_path[1024];

	build_path(dst_path, sizeof(dst_path), tdnp, name);

	struct hl_param p[3];
	__s32 ret;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = snp->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = snp->hf_path;
	p[1].str.len = strlen(snp->hf_path);
	p[2].type = HL_PV_HLSTRING;
	p[2].str.ptr = dst_path;
	p[2].str.len = strlen(dst_path);

	if (hl_hcall_int("fs_link", p, 3, &ret) < 0)
		return EIO;

	return ret < 0 ? -ret : 0;
}

/* ── symlink ──────────────────────────────────────────────────────── */

static int
hostfs_symlink(struct vnode *dvp, const char *name, const char *oldpath)
{
	struct hostfs_node *dnp = dvp->v_data;
	char link_path[1024];

	build_path(link_path, sizeof(link_path), dnp, name);

	struct hl_param p[3];
	__s32 ret;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = dnp->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = link_path;
	p[1].str.len = strlen(link_path);
	p[2].type = HL_PV_HLSTRING;
	p[2].str.ptr = oldpath;
	p[2].str.len = strlen(oldpath);

	if (hl_hcall_int("fs_symlink", p, 3, &ret) < 0)
		return EIO;

	return ret < 0 ? -ret : 0;
}

/* ── readlink ─────────────────────────────────────────────────────── */

static int
hostfs_readlink(struct vnode *vp, struct uio *uio)
{
	struct hostfs_node *np = vp->v_data;
	struct hl_param p[2];
	__u8 *buf = hostfs_io.rbuf;
	__sz len;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = np->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = np->hf_path;
	p[1].str.len = strlen(np->hf_path);

	if (hl_hcall_vecbytes("fs_readlink", p, 2, buf, hostfs_io.result_max,
			      &len) < 0)
		return EIO;

	if (len < 4)
		return EIO;

	__s32 status = (__s32)(buf[0] | (buf[1] << 8) |
			       (buf[2] << 16) | (buf[3] << 24));
	if (status < 0)
		return -status;

	size_t target_len = len - 4;
	const __u8 *target = buf + 4;

	/* Copy target path bytes into the uio. */
	size_t remaining = target_len;
	const __u8 *src = target;

	while (remaining > 0 && uio->uio_iovcnt > 0) {
		struct iovec *iov = uio->uio_iov;
		size_t chunk = remaining;

		if (chunk > iov->iov_len)
			chunk = iov->iov_len;

		memcpy(iov->iov_base, src, chunk);
		src += chunk;
		remaining -= chunk;

		iov->iov_base = (char *)iov->iov_base + chunk;
		iov->iov_len -= chunk;
		uio->uio_resid -= chunk;
		uio->uio_offset += chunk;

		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
		}
	}

	return 0;
}

/* ── truncate ─────────────────────────────────────────────────────── */

static int
hostfs_truncate(struct vnode *vp, off_t length)
{
	struct hostfs_node *np = vp->v_data;
	struct hl_param p[3];
	__s32 ret;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = np->hf_mount_idx;
	p[1].type = HL_PV_HLSTRING;
	p[1].str.ptr = np->hf_path;
	p[1].str.len = strlen(np->hf_path);
	p[2].type = HL_PV_HLULONG;
	p[2].u64_val = (uint64_t)length;

	if (hl_hcall_int("fs_truncate", p, 3, &ret) < 0)
		return EIO;

	if (ret == 0)
		vp->v_size = length;

	return ret < 0 ? -ret : 0;
}

/* ── inactive ─────────────────────────────────────────────────────── */

static int
hostfs_inactive(struct vnode *vp)
{
	if (vp->v_data) {
		free(vp->v_data);
		vp->v_data = NULL;
	}
	return 0;
}

/* ── Stubs ────────────────────────────────────────────────────────── */

#define hostfs_open      ((vnop_open_t)vfscore_vop_nullop)
#define hostfs_close     ((vnop_close_t)vfscore_vop_nullop)
#define hostfs_seek      ((vnop_seek_t)vfscore_vop_nullop)
#define hostfs_ioctl     ((vnop_ioctl_t)vfscore_vop_einval)
#define hostfs_fsync     ((vnop_fsync_t)vfscore_vop_nullop)
#define hostfs_cache     ((vnop_cache_t)NULL)
#define hostfs_fallocate ((vnop_fallocate_t)vfscore_vop_einval)
#define hostfs_poll      ((vnop_poll_t)vfscore_vop_einval)

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
	hostfs_cache,
	hostfs_fallocate,
	hostfs_readlink,
	hostfs_symlink,
	hostfs_poll,
};
