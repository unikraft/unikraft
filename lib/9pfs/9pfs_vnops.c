/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#define _GNU_SOURCE

#include <fcntl.h>
#include <dirent.h>
#include <uk/config.h>
#include <uk/9p.h>
#include <uk/errptr.h>
#include <vfscore/mount.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include <vfscore/file.h>
#include <vfscore/fs.h>

#include "9pfs.h"

static uint8_t uk_9pfs_open_mode_from_posix_flags(unsigned long flags)
{
	uint8_t mode = 0;
	uint8_t flags_rw = flags & (UK_FREAD | UK_FWRITE);

	if (flags_rw == UK_FREAD)
		mode = UK_9P_OREAD;
	else if (flags_rw == UK_FWRITE)
		mode = UK_9P_OWRITE;
	else if (flags_rw == (UK_FREAD | UK_FWRITE))
		mode = UK_9P_ORDWR;

	if (flags & O_EXCL)
		mode |= UK_9P_OEXCL;
	if (flags & O_TRUNC)
		mode |= UK_9P_OTRUNC;

	return mode;
}

static int uk_9pfs_posix_perm_from_mode(int mode)
{
	int res;

	res = mode & 0777;
	if (mode & UK_9P_DMSETUID)
		res |= S_ISUID;
	if (mode & UK_9P_DMSETGID)
		res |= S_ISGID;
	if (mode & UK_9P_DMSETVTX)
		res |= S_ISVTX;

	return res;
}

static uint32_t uk_9pfs_perm_from_posix_mode(mode_t mode)
{
	int res;

	res = mode & 0777;
	if (S_ISDIR(mode))
		res |= UK_9P_DMDIR;

	return res;
}

static int uk_9pfs_dttype_from_mode(int mode)
{
	if (mode & UK_9P_DMDIR)
		return DT_DIR;
	return DT_REG;
}

static int uk_9pfs_posix_mode_from_mode(int mode)
{
	int res;

	res = uk_9pfs_posix_perm_from_mode(mode);

	if (mode & UK_9P_DMDIR)
		res |= S_IFDIR;
	else
		res |= S_IFREG;

	return res;
}

static int uk_9pfs_vtype_from_mode(int mode)
{
	if (mode & UK_9P_DMDIR)
		return VDIR;
	return VREG;
}

static uint64_t uk_9pfs_ino(struct uk_9p_stat *stat)
{
	return stat->qid.path;
}

int uk_9pfs_allocate_vnode_data(struct vnode *vp, struct uk_9pfid *fid)
{
	struct uk_9pfs_node_data *nd;

	nd = malloc(sizeof(*nd));
	if (nd == NULL)
		return -ENOMEM;

	nd->fid = fid;
	nd->nb_open_files = 0;
	nd->removed = false;
	vp->v_data = nd;

	return 0;
}

void uk_9pfs_free_vnode_data(struct vnode *vp)
{
	struct uk_9pdev *dev = UK_9PFS_MD(vp->v_mount)->dev;
	struct uk_9pfs_node_data *nd = UK_9PFS_ND(vp);

	if (!vp->v_data)
		return;

	if (nd->removed)
		uk_9p_remove(dev, nd->fid);

	uk_9pfid_put(nd->fid);
	free(nd);
	vp->v_data = NULL;
}

static int uk_9pfs_open(struct vfscore_file *file)
{
	struct uk_9pdev *dev = UK_9PFS_MD(file->f_dentry->d_mount)->dev;
	struct uk_9pfid *openedfid;
	struct uk_9pfs_file_data *fd;
	int rc;

	/* Allocate memory for file data. */
	fd = calloc(1, sizeof(*fd));
	if (!fd)
		return ENOMEM;

	/* Clone fid. */
	openedfid = uk_9p_walk(dev, UK_9PFS_VFID(file->f_dentry->d_vnode),
			NULL);
	if (PTRISERR(openedfid)) {
		rc = PTR2ERR(openedfid);
		goto out;
	}

	/* Open cloned fid. */
	rc = uk_9p_open(dev, openedfid,
		uk_9pfs_open_mode_from_posix_flags(file->f_flags));

	if (rc)
		goto out_err;

	fd->fid = openedfid;
	file->f_data = fd;
 	UK_9PFS_ND(file->f_dentry->d_vnode)->nb_open_files++;

	return 0;

out_err:
	uk_9pfid_put(openedfid);
out:
	free(fd);
	return -rc;
}

static int uk_9pfs_close(struct vnode *vn __unused, struct vfscore_file *file)
{
	struct uk_9pfs_file_data *fd = UK_9PFS_FD(file);

	if (fd->readdir_buf)
		free(fd->readdir_buf);

	uk_9pfid_put(fd->fid);
	free(fd);
	UK_9PFS_ND(file->f_dentry->d_vnode)->nb_open_files--;

	return 0;
}

static int uk_9pfs_lookup(struct vnode *dvp, char *name, struct vnode **vpp)
{
	struct uk_9pdev *dev = UK_9PFS_MD(dvp->v_mount)->dev;
	struct uk_9pfid *dfid = UK_9PFS_VFID(dvp);
	struct uk_9pfid *fid;
	struct uk_9p_stat stat;
	struct uk_9preq *stat_req;
	struct vnode *vp;
	int rc;

	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	fid = uk_9p_walk(dev, dfid, name);
	if (PTRISERR(fid)) {
		rc = PTR2ERR(fid);
		goto out;
	}

	stat_req = uk_9p_stat(dev, fid, &stat);
	if (PTRISERR(stat_req)) {
		rc = PTR2ERR(stat_req);
		goto out_fid;
	}

	/* No stat string fields are used below. */
	uk_9pdev_req_remove(dev, stat_req);

	if (vfscore_vget(dvp->v_mount, uk_9pfs_ino(&stat), &vp)) {
		/* Already in cache. */
		rc = 0;
		*vpp = vp;
		/* if the vnode already has node data, it may be reused. */
		if (vp->v_data)
			goto out_fid;
	}

	if (!vp) {
		rc = -ENOMEM;
		goto out_fid;
	}

	vp->v_flags = 0;
	vp->v_mode = uk_9pfs_posix_mode_from_mode(stat.mode);
	vp->v_type = uk_9pfs_vtype_from_mode(stat.mode);
	vp->v_size = stat.length;

	rc = uk_9pfs_allocate_vnode_data(vp, fid);
	if (rc != 0)
		goto out_fid;

	*vpp = vp;

	return 0;

out_fid:
	uk_9pfid_put(fid);
out:
	return -rc;
}

static int uk_9pfs_inactive(struct vnode *vp)
{
	if (vp->v_data)
		uk_9pfs_free_vnode_data(vp);

	return 0;
}

static int uk_9pfs_create_generic(struct vnode *dvp, char *name, mode_t mode)
{
	struct uk_9pdev *dev = UK_9PFS_MD(dvp->v_mount)->dev;
	struct uk_9pfid *fid;
	int rc;

	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	/* Clone parent fid. */
	fid = uk_9p_walk(dev, UK_9PFS_VFID(dvp), NULL);

	rc = uk_9p_create(dev, fid, name, uk_9pfs_perm_from_posix_mode(mode),
			UK_9P_OTRUNC | UK_9P_OWRITE, NULL);

	uk_9pfid_put(fid);
	return -rc;
}

static int uk_9pfs_create(struct vnode *dvp, char *name, mode_t mode)
{
	if (!S_ISREG(mode))
		return EINVAL;

	return uk_9pfs_create_generic(dvp, name, mode);
}

static int uk_9pfs_remove_generic(struct vnode *dvp, struct vnode *vp)
{
	struct uk_9pdev *dev = UK_9PFS_MD(dvp->v_mount)->dev;
	struct uk_9pfs_node_data *nd = UK_9PFS_ND(vp);

	return -uk_9p_remove(dev, nd->fid);
}

static int uk_9pfs_remove(struct vnode *dvp, struct vnode *vp,
		char *name __unused)
{
	struct uk_9pfs_node_data *nd = UK_9PFS_ND(vp);
	int rc = 0;

	if (!nd->nb_open_files)
		rc = uk_9pfs_remove_generic(dvp, vp);
	else
		nd->removed = true;

	return rc;
}

static int uk_9pfs_mkdir(struct vnode *dvp, char *name, mode_t mode)
{
	if (!S_ISDIR(mode))
		return EINVAL;

	return uk_9pfs_create_generic(dvp, name, mode);
}

static int uk_9pfs_rmdir(struct vnode *dvp, struct vnode *vp,
		char *name __unused)
{
	return uk_9pfs_remove_generic(dvp, vp);
}

static int uk_9pfs_readdir(struct vnode *vp, struct vfscore_file *fp,
		struct dirent *dir)
{
	struct uk_9pdev *dev = UK_9PFS_MD(vp->v_mount)->dev;
	struct uk_9pfs_file_data *fd = UK_9PFS_FD(fp);
	int rc;
	struct uk_9p_stat stat;
	struct uk_9preq fake_request;

again:
	if (!fd->readdir_buf) {
		fd->readdir_buf = malloc(UK_9PFS_READDIR_BUFSZ);
		if (!fd->readdir_buf)
			return ENOMEM;

		/* Currently the readdir() buffer is empty. */
		fd->readdir_off = 0;
		fd->readdir_sz = 0;
	}

	if (fd->readdir_off == fd->readdir_sz) {
		fd->readdir_off = 0;
		fd->readdir_sz = uk_9p_read(dev, fd->fid, fp->f_offset,
				UK_9PFS_READDIR_BUFSZ, fd->readdir_buf);
		if (fd->readdir_sz < 0) {
			rc = fd->readdir_sz;
			goto out;
		}

		/* End of directory. */
		if (fd->readdir_sz == 0) {
			rc = -ENOENT;
			goto out;
		}

		/*
		 * Update offset for the next readdir() call which requires
		 * the next chunk of data to be transferred.
		 */
		fp->f_offset += fd->readdir_sz;
	}

	/*
	 * Build a fake request to use the 9P request API to read from the
	 * buffer the stat structure.
	 */
	fake_request.recv.buf = fd->readdir_buf;
	fake_request.recv.size = fd->readdir_sz;
	fake_request.recv.offset = fd->readdir_off;
	fake_request.state = UK_9PREQ_RECEIVED;
	rc = uk_9preq_readstat(&fake_request, &stat);

	if (rc == -ENOBUFS) {
		/*
		 * Retry with a clean buffer, maybe the stat structure got
		 * chunked and is not whole, although the RFC says this should
		 * not happen.
		 */
		fd->readdir_off = 0;
		fd->readdir_sz = 0;
		goto again;
	}

	/* Update the readdir() offset to the offset after deserialization. */
	fd->readdir_off = fake_request.recv.offset;

	/*
	 * Any other error besides ENOBUFS when deserializing is considered
	 * an IO error.
	 */
	if (rc) {
		rc = -EIO;
		goto out;
	}

	dir->d_type = uk_9pfs_dttype_from_mode(stat.mode);
	dir->d_ino = uk_9pfs_ino(&stat);
	strlcpy((char *) &dir->d_name, stat.name.data,
			MIN(sizeof(dir->d_name), stat.name.size + 1U));

out:
	return -rc;
}

static int uk_9pfs_read(struct vnode *vp, struct vfscore_file *fp,
			struct uio *uio, int ioflag __unused)
{
	struct uk_9pdev *dev = UK_9PFS_MD(vp->v_mount)->dev;
	struct uk_9pfid *fid = UK_9PFS_FD(fp)->fid;
	struct iovec *iov;
	int rc;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_offset >= (off_t) vp->v_size)
		return 0;

	if (!uio->uio_resid)
		return 0;

	iov = uio->uio_iov;
	while (!iov->iov_len) {
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}

	rc = uk_9p_read(dev, fid, uio->uio_offset,
			   iov->iov_len, iov->iov_base);
	if (rc < 0)
		return -rc;

	iov->iov_base = (char *)iov->iov_base + rc;
	iov->iov_len -= rc;
	uio->uio_resid -= rc;
	uio->uio_offset += rc;

	return 0;
}

static int uk_9pfs_write(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct uk_9pdev *dev = UK_9PFS_MD(vp->v_mount)->dev;
	struct uk_9pfid *fid;
	struct iovec *iov;
	int rc;

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

	if (ioflag & IO_APPEND)
		uio->uio_offset = vp->v_size;

	/* Clone vnode fid. */
	fid = uk_9p_walk(dev, UK_9PFS_VFID(vp), NULL);
	if (PTRISERR(fid))
		return -PTR2ERR(fid);

	rc = uk_9p_open(dev, fid, UK_9P_OWRITE);
	if (rc < 0)
		goto out;

	if (!uio->uio_resid)
		return 0;

	iov = uio->uio_iov;
	while (!iov->iov_len) {
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}

	rc = uk_9p_write(dev, fid, uio->uio_offset,
			    iov->iov_len, iov->iov_base);
	if (rc < 0)
		return -rc;

	iov->iov_base = (char *)iov->iov_base + rc;
	iov->iov_len -= rc;
	uio->uio_resid -= rc;
	uio->uio_offset += rc;

	if (rc < 0)
		goto out;

	rc = 0;

	/*
	 * If the uio offset after completion of the write requests is bigger
	 * than the vnode's associated size, then the size must be updated
	 * accordingly.
	 */
	if (uio->uio_offset > vp->v_size)
		vp->v_size = uio->uio_offset;

out:
	uk_9pfid_put(fid);
	return -rc;
}

static int uk_9pfs_getattr(struct vnode *vp, struct vattr *attr)
{
	struct uk_9pdev *dev = UK_9PFS_MD(vp->v_mount)->dev;
	struct uk_9pfid *fid = UK_9PFS_VFID(vp);
	struct uk_9p_stat stat;
	struct uk_9preq *stat_req;
	int rc = 0;

	stat_req = uk_9p_stat(dev, fid, &stat);
	if (PTRISERR(stat_req)) {
		rc = PTR2ERR(stat_req);
		goto out;
	}

	/* No stat string fields are used below. */
	uk_9pdev_req_remove(dev, stat_req);

	attr->va_type = uk_9pfs_vtype_from_mode(stat.mode);
	attr->va_mode = uk_9pfs_posix_mode_from_mode(stat.mode);
	attr->va_nodeid = vp->v_ino;
	attr->va_size = stat.length;

	attr->va_atime.tv_sec = stat.atime;
	attr->va_atime.tv_nsec = 0;
	attr->va_mtime.tv_sec = stat.mtime;
	attr->va_mtime.tv_nsec = 0;
	attr->va_ctime.tv_sec = 0;
	attr->va_ctime.tv_nsec = 0;

out:
	return -rc;
}

#define uk_9pfs_seek		((vnop_seek_t)vfscore_vop_nullop)
#define uk_9pfs_ioctl		((vnop_ioctl_t)vfscore_vop_einval)
#define uk_9pfs_fsync		((vnop_fsync_t)vfscore_vop_nullop)
#define uk_9pfs_setattr		((vnop_setattr_t)vfscore_vop_nullop)
#define uk_9pfs_truncate	((vnop_truncate_t)vfscore_vop_nullop)
#define uk_9pfs_link		((vnop_link_t)vfscore_vop_eperm)
#define uk_9pfs_cache		((vnop_cache_t)NULL)
#define uk_9pfs_readlink	((vnop_readlink_t)vfscore_vop_einval)
#define uk_9pfs_symlink		((vnop_symlink_t)vfscore_vop_eperm)
#define uk_9pfs_fallocate	((vnop_fallocate_t)vfscore_vop_nullop)
#define uk_9pfs_rename		((vnop_rename_t)vfscore_vop_einval)

struct vnops uk_9pfs_vnops = {
	.vop_open	= uk_9pfs_open,
	.vop_close	= uk_9pfs_close,
	.vop_read	= uk_9pfs_read,
	.vop_write	= uk_9pfs_write,
	.vop_seek	= uk_9pfs_seek,
	.vop_ioctl	= uk_9pfs_ioctl,
	.vop_fsync	= uk_9pfs_fsync,
	.vop_readdir	= uk_9pfs_readdir,
	.vop_lookup	= uk_9pfs_lookup,
	.vop_create	= uk_9pfs_create,
	.vop_remove	= uk_9pfs_remove,
	.vop_rename	= uk_9pfs_rename,
	.vop_mkdir	= uk_9pfs_mkdir,
	.vop_rmdir	= uk_9pfs_rmdir,
	.vop_getattr	= uk_9pfs_getattr,
	.vop_setattr	= uk_9pfs_setattr,
	.vop_inactive	= uk_9pfs_inactive,
	.vop_truncate	= uk_9pfs_truncate,
	.vop_link	= uk_9pfs_link,
	.vop_cache	= uk_9pfs_cache,
	.vop_fallocate	= uk_9pfs_fallocate,
	.vop_readlink	= uk_9pfs_readlink,
	.vop_symlink	= uk_9pfs_symlink
};
