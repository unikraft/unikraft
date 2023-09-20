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
 */

#define _GNU_SOURCE

#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <uk/config.h>
#include <uk/9p.h>
#include <uk/errptr.h>
#include <vfscore/mount.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include <vfscore/file.h>
#include <vfscore/fs.h>

#include "9pfs.h"

static uint32_t uk_9pfs_open_mode_from_posix_flags(int flags)
{
	uint32_t mode = 0;
	uint32_t flags_rw = flags & (UK_FREAD | UK_FWRITE);

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

	if (mode & UK_9P_DMSYMLINK)
		return VLNK;

	return VREG;
}

static int uk_9pfs_vtype_from_mode_l(int mode)
{
	switch (mode & S_IFMT) {
	case S_IFREG:
		return VREG;
	case S_IFDIR:
		return VDIR;
	case S_IFBLK:
		return VBLK;
	case S_IFCHR:
		return VCHR;
	case S_IFLNK:
		return VLNK;
	case S_IFSOCK:
		return VSOCK;
	case S_IFIFO:
		return VFIFO;
	case 0:
		return VNON;
	default:
		return VBAD;
	}
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
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(file->f_dentry->d_mount);
	struct uk_9pdev *dev = md->dev;
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
	if (md->proto == UK_9P_PROTO_2000L)
		rc = uk_9p_lopen(dev, openedfid, vfscore_oflags(file->f_flags));
	else if (md->proto == UK_9P_PROTO_2000U)
		rc = uk_9p_open(
		    dev, openedfid,
		    uk_9pfs_open_mode_from_posix_flags(file->f_flags));
	else
		rc = -EOPNOTSUPP;

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

static int uk_9pfs_lookup(struct vnode *dvp, const char *name,
			  struct vnode **vpp)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(dvp->v_mount);
	struct uk_9pdev *dev = md->dev;
	struct uk_9pfid *dfid = UK_9PFS_VFID(dvp);
	struct uk_9pfid *fid;
	struct vnode *vp;
	int rc;

	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	fid = uk_9p_walk(dev, dfid, name);
	if (PTRISERR(fid)) {
		rc = PTR2ERR(fid);
		goto out;
	}

	if (md->proto == UK_9P_PROTO_2000L) {
		struct uk_9p_attr stat;
		struct uk_9preq *stat_req = uk_9p_getattr(
		    dev, fid, UK_9P_GETATTR_MODE | UK_9P_GETATTR_SIZE, &stat);
		if (PTRISERR(stat_req)) {
			rc = PTR2ERR(stat_req);
			goto out_fid;
		}
		if (!(stat.valid & UK_9P_GETATTR_MODE)
		    || !(stat.valid & UK_9P_GETATTR_SIZE)) {
			rc = -EIO;
			goto out;
		}

		uk_9pdev_req_remove(dev, stat_req);

		if (vfscore_vget(dvp->v_mount, stat.qid.path, &vp)) {
			/* Already in cache. */
			rc = 0;
			*vpp = vp;
			/*
			 * if the vnode already has node data,
			 * it may be reused.
			 */
			if (vp->v_data)
				goto out_fid;
		}

		if (!vp) {
			rc = -ENOMEM;
			goto out_fid;
		}

		vp->v_flags = 0;
		vp->v_mode = stat.mode;
		vp->v_type = uk_9pfs_vtype_from_mode_l(stat.mode);
		vp->v_size = stat.size;

	} else if (md->proto == UK_9P_PROTO_2000U) {
		struct uk_9p_stat stat;
		struct uk_9preq *stat_req = uk_9p_stat(dev, fid, &stat);

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
			/*
			 * if the vnode already has node data,
			 * it may be reused.
			 */
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

	} else {
		rc = -EOPNOTSUPP;
		goto out_fid;
	}

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

static int uk_9pfs_create_generic(struct vnode *dvp, const char *name,
				  mode_t mode)
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

static int uk_9pfs_create(struct vnode *dvp, const char *name, mode_t mode)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(dvp->v_mount);

	if (!S_ISREG(mode))
		return EINVAL;

	if (md->proto == UK_9P_PROTO_2000L) {
		struct uk_9pfid *fid =
		    uk_9p_walk(md->dev, UK_9PFS_VFID(dvp), NULL);
		return -uk_9p_lcreate(md->dev, fid, name,
				UK_9P_DOTL_WRONLY | UK_9P_DOTL_APPEND, mode, 0);

	} else if (md->proto == UK_9P_PROTO_2000U) {
		return uk_9pfs_create_generic(dvp, name, mode);

	} else {
		return EOPNOTSUPP;
	}
}

static int uk_9pfs_remove_generic(struct vnode *dvp, struct vnode *vp)
{
	struct uk_9pdev *dev = UK_9PFS_MD(dvp->v_mount)->dev;
	struct uk_9pfs_node_data *nd = UK_9PFS_ND(vp);

	return -uk_9p_remove(dev, nd->fid);
}

static int uk_9pfs_remove(struct vnode *dvp, struct vnode *vp,
			  const char *name __unused)
{
	struct uk_9pfs_node_data *nd = UK_9PFS_ND(vp);
	int rc = 0;

	if (!nd->nb_open_files)
		rc = uk_9pfs_remove_generic(dvp, vp);
	else
		nd->removed = true;

	return rc;
}

static int uk_9pfs_mkdir(struct vnode *dvp, const char *name, mode_t mode)
{
	if (!S_ISDIR(mode))
		return EINVAL;

	return uk_9pfs_create_generic(dvp, name, mode);
}

static int uk_9pfs_rmdir(struct vnode *dvp, struct vnode *vp,
			 const char *name __unused)
{
	return uk_9pfs_remove_generic(dvp, vp);
}

static int uk_9pfs_readdir(struct vnode *vp, struct vfscore_file *fp,
		struct dirent64 *dir)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(vp->v_mount);
	struct uk_9pdev *dev = md->dev;
	struct uk_9pfs_file_data *fd = UK_9PFS_FD(fp);
	int rc;
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

		if (md->proto == UK_9P_PROTO_2000L)
			fd->readdir_sz = uk_9p_readdir(
			    dev, fd->fid, fp->f_offset, UK_9PFS_READDIR_BUFSZ,
			    fd->readdir_buf);
		else if (md->proto == UK_9P_PROTO_2000U)
			fd->readdir_sz =
			    uk_9p_read(dev, fd->fid, fp->f_offset,
				       UK_9PFS_READDIR_BUFSZ, fd->readdir_buf);
		else
			fd->readdir_sz = -EOPNOTSUPP;

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
		 * 9p2000.L updates f_offset on each entry read.
		 */
		if (md->proto == UK_9P_PROTO_2000U)
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
	if (md->proto == UK_9P_PROTO_2000L) {
		struct uk_9p_qid qid;
		uint64_t offset;
		uint8_t type;
		struct uk_9p_str name;

		rc = uk_9preq_readdirent(&fake_request, &qid, &offset, &type,
					 &name);

		if (rc == -ENOBUFS) {
			/*
			 * Retry with a clean buffer, maybe the stat structure
			 * got chunked and is not whole, although the RFC says
			 * this should not happen.
			 */
			fd->readdir_off = 0;
			fd->readdir_sz = 0;
			goto again;
		}

		/*
		 * Update the readdir() offset to the offset after
		 * deserialization.
		 */
		fd->readdir_off = fake_request.recv.offset;
		fp->f_offset = offset;

		/*
		 * Any other error besides ENOBUFS when deserializing is
		 * considered an IO error.
		 */
		if (rc) {
			rc = -EIO;
			goto out;
		}

		dir->d_type = type;
		dir->d_ino = qid.path;
		strlcpy((char *)&dir->d_name, name.data,
			MIN(sizeof(dir->d_name), name.size + 1U));

	} else if (md->proto == UK_9P_PROTO_2000U) {
		struct uk_9p_stat stat;

		rc = uk_9preq_readstat(&fake_request, &stat);

		if (rc == -ENOBUFS) {
			fd->readdir_off = 0;
			fd->readdir_sz = 0;
			goto again;
		}

		fd->readdir_off = fake_request.recv.offset;

		if (rc) {
			rc = -EIO;
			goto out;
		}

		dir->d_type = uk_9pfs_dttype_from_mode(stat.mode);
		dir->d_ino = uk_9pfs_ino(&stat);
		strlcpy((char *)&dir->d_name, stat.name.data,
			MIN(sizeof(dir->d_name), stat.name.size + 1U));
	}

out:
	return -rc;
}

static int uk_9pfs_read(struct vnode *vp, struct vfscore_file *fp,
			struct uio *uio, int ioflag __unused)
{
	struct uk_9pdev *dev = UK_9PFS_MD(vp->v_mount)->dev;
	struct uk_9pfid *fid = UK_9PFS_FD(fp)->fid;
	struct iovec *iov;
	int64_t bytes;
	int i = 0;

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

	while (i < uio->uio_iovcnt) {
		iov = &uio->uio_iov[i];
		if (!iov->iov_len) {
			i++;
			continue;
		}

		bytes = uk_9p_read(dev, fid, uio->uio_offset,
				   iov->iov_len, iov->iov_base);
		if (unlikely(bytes < 0))
			return -(int)bytes;
		if (!bytes)
			break;

		UK_ASSERT(uio->uio_offset <= __OFF_MAX - bytes);
		UK_ASSERT(uio->uio_resid >= bytes);
		UK_ASSERT(iov->iov_len >= (uint64_t)bytes);

		iov->iov_base = (char *)iov->iov_base + bytes;
		iov->iov_len -= bytes;
		uio->uio_offset += bytes;
		uio->uio_resid -= bytes;
	}

	return 0;
}

static int uk_9pfs_write(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(vp->v_mount);
	struct uk_9pdev *dev = md->dev;
	struct uk_9pfid *fid;
	struct iovec *iov;
	int64_t bytes;
	int rc, i = 0;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_offset >= LONG_MAX)
		return EFBIG;
	if (!uio->uio_resid)
		return 0;

	if (ioflag & IO_APPEND)
		uio->uio_offset = vp->v_size;

	/* Clone vnode fid. */
	fid = uk_9p_walk(dev, UK_9PFS_VFID(vp), NULL);
	if (PTRISERR(fid))
		return -PTR2ERR(fid);

	if (md->proto == UK_9P_PROTO_2000L)
		rc = uk_9p_lopen(dev, fid, O_WRONLY);
	else if (md->proto == UK_9P_PROTO_2000U)
		rc = uk_9p_open(dev, fid, UK_9P_OWRITE);
	else
		rc = -EOPNOTSUPP;

	if (rc < 0)
		goto out;

	while (i < uio->uio_iovcnt) {
		iov = &uio->uio_iov[i];
		if (!iov->iov_len) {
			i++;
			continue;
		}

		bytes = uk_9p_write(dev, fid, uio->uio_offset,
				    iov->iov_len, iov->iov_base);
		if (unlikely(bytes < 0)) {
			rc = (int)bytes;
			break;
		}
		if (!bytes)
			break;

		UK_ASSERT(uio->uio_offset <= __OFF_MAX - bytes);
		UK_ASSERT(uio->uio_resid >= bytes);
		UK_ASSERT(iov->iov_len >= (uint64_t)bytes);

		iov->iov_base = (char *)iov->iov_base + bytes;
		iov->iov_len -= bytes;
		uio->uio_offset += bytes;
		uio->uio_resid -= bytes;
	}

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
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(vp->v_mount);
	struct uk_9pdev *dev = md->dev;
	struct uk_9pfid *fid = UK_9PFS_VFID(vp);
	struct uk_9preq *stat_req;
	int rc = 0;

	if (md->proto == UK_9P_PROTO_2000L) {
		struct uk_9p_attr stat;

		stat_req = uk_9p_getattr(dev, fid, UK_9P_GETATTR_BASIC, &stat);
		if (PTRISERR(stat_req)) {
			rc = PTR2ERR(stat_req);
			goto out;
		}

		uk_9pdev_req_remove(dev, stat_req);

		if ((stat.valid & UK_9P_GETATTR_BASIC) != UK_9P_GETATTR_BASIC) {
			rc = -EIO;
			goto out;
		}

		attr->va_type = stat.mode & S_IFMT;
		attr->va_mode = stat.mode & UK_ALLPERMS;
		attr->va_nlink = stat.nlink;
		attr->va_uid = stat.uid;
		attr->va_gid = stat.gid;
		attr->va_nodeid = vp->v_ino;
		attr->va_atime.tv_sec = stat.atime_sec;
		attr->va_atime.tv_nsec = stat.atime_nsec;
		attr->va_mtime.tv_sec = stat.mtime_sec;
		attr->va_mtime.tv_nsec = stat.mtime_nsec;
		attr->va_ctime.tv_sec = stat.ctime_sec;
		attr->va_ctime.tv_nsec = stat.ctime_nsec;
		attr->va_rdev = stat.rdev;
		attr->va_nblocks = stat.blocks;
		attr->va_size = stat.size;

	} else if (md->proto == UK_9P_PROTO_2000U) {
		struct uk_9p_stat stat;

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
	} else {
		rc = -EOPNOTSUPP;
	}

out:
	return -rc;
}

static int uk_9pfs_do_setattr(struct vnode *vp, struct uk_9pfid *fid,
			      struct vattr *attr)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(vp->v_mount);
	struct uk_9pdev *dev = md->dev;

	if (md->proto == UK_9P_PROTO_2000L) {
		uint32_t valid = 0;
		uint32_t mode = 0;
		uint32_t uid = 0;
		uint32_t gid = 0;
		uint64_t size = 0;
		uint64_t atime_sec = 0, atime_nsec = 0;
		uint64_t mtime_sec = 0, mtime_nsec = 0;

		if (attr->va_mask & AT_MODE) {
			valid |= UK_9P_SETATTR_MODE;
			mode = attr->va_mode & UK_ALLPERMS;
		}
		if (attr->va_mask & AT_UID) {
			valid |= UK_9P_SETATTR_UID;
			uid = attr->va_uid;
		}
		if (attr->va_mask & AT_GID) {
			valid |= UK_9P_SETATTR_GID;
			uid = attr->va_uid;
		}
		if (attr->va_mask & AT_SIZE) {
			valid |= UK_9P_SETATTR_SIZE;
			size = attr->va_size;
		}
		if (attr->va_mask & AT_ATIME) {
			valid |= UK_9P_SETATTR_ATIME | UK_9P_SETATTR_ATIME_SET;
			atime_sec = attr->va_atime.tv_sec;
			atime_nsec = attr->va_atime.tv_nsec;
		}
		if (attr->va_mask & AT_MTIME) {
			valid |= UK_9P_SETATTR_MTIME | UK_9P_SETATTR_MTIME_SET;
			mtime_sec = attr->va_mtime.tv_sec;
			mtime_nsec = attr->va_mtime.tv_nsec;
		}

		return -uk_9p_setattr(dev, fid, valid, mode, uid, gid, size,
				      atime_sec, atime_nsec, mtime_sec,
				      mtime_nsec);

	} else if (md->proto == UK_9P_PROTO_2000U) {
		static const struct uk_9p_stat donttouch_stat = {
			.type = (uint16_t)-1,
			.dev = (uint32_t)-1,
			.qid.type = (uint8_t)-1,
			.qid.version = (uint32_t)-1,
			.qid.path = (uint64_t)-1,
			.mode = (uint32_t)-1,
			.atime = (uint32_t)-1,
			.mtime = (uint32_t)-1,
			.length = (uint64_t)-1,
			.name.size = 0,
			.uid.size = 0,
			.gid.size = 0,
			.muid.size = 0,
			.extension.size = 0,
			.n_uid = (uint32_t)-1,
			.n_gid = (uint32_t)-1,
			.n_muid = (uint32_t)-1,
		};
		struct uk_9p_stat stat = donttouch_stat;
		int rc;

		/* Sending an unmodified donttouch stat will fsync() the file.
		 * Otherwise, the fields which differ from the donttouch stat
		 * will be updated.
		 */

		if (attr->va_mask & AT_ATIME)
			stat.atime = attr->va_atime.tv_sec;

		if (attr->va_mask & AT_MTIME)
			stat.mtime = attr->va_mtime.tv_sec;

		if (attr->va_mask & AT_MODE) {
			stat.mode = attr->va_mode;

			switch (vp->v_type) {
			case VDIR:
				stat.mode |= UK_9P_DMDIR;
				break;
			case VBLK:
			case VCHR:
				stat.mode |= UK_9P_DMDEVICE;
				break;
			case VLNK:
				stat.mode |= UK_9P_DMSYMLINK;
				break;
			case VSOCK:
				stat.mode |= UK_9P_DMSOCKET;
				break;
			case VFIFO:
				stat.mode |= UK_9P_DMNAMEDPIPE;
			}
		}

		if (attr->va_mask & AT_SIZE) {
			UK_ASSERT(vp->v_type == VREG);
			stat.length = attr->va_size;
		}

		rc = uk_9p_wstat(dev, fid, &stat);

		return -rc;
	} else {
		return 0;
	}
}

static int uk_9pfs_setattr(struct vnode *vp, struct vattr *attr)
{
	return uk_9pfs_do_setattr(vp, UK_9PFS_VFID(vp), attr);
}

static int uk_9pfs_fsync(struct vnode *vp, struct vfscore_file *fp)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(vp->v_mount);
	struct uk_9pfid *fid = UK_9PFS_FD(fp)->fid;

	if (md->proto == UK_9P_PROTO_2000L) {
		return -uk_9p_fsync(md->dev, fid);
	} else if (md->proto == UK_9P_PROTO_2000U) {
		return uk_9pfs_do_setattr(vp, fid, &(struct vattr){
			.va_mask = 0
		});
	} else {
		return 0;
	}
}

static int uk_9pfs_truncate(struct vnode *vp, off_t off)
{
	return uk_9pfs_do_setattr(vp, UK_9PFS_VFID(vp), &(struct vattr){
		.va_mask = AT_SIZE,
		.va_size = off,
	});
}

static int uk_9pfs_rename(struct vnode *dvp1, struct vnode *vp1,
			  const char *name1,
			  struct vnode *dvp2, struct vnode *vp2 __unused,
			  const char *name2)
{
	struct uk_9pfs_mount_data *dmd1 = UK_9PFS_MD(dvp1->v_mount);
	struct uk_9pfid *dfid1 = UK_9PFS_VFID(dvp1);
	struct uk_9pfid *fid1 = UK_9PFS_VFID(vp1);
	struct uk_9pfs_mount_data *dmd2 = UK_9PFS_MD(dvp2->v_mount);
	struct uk_9pfid *dfid2 = UK_9PFS_VFID(dvp2);
	int rc = 0;

	if (dmd1->dev != dmd2->dev)
		return EXDEV;

	if (dmd1->proto == UK_9P_PROTO_2000L) {
		rc = uk_9p_renameat(dmd1->dev, dfid1, name1, dfid2, name2);
		if (rc == -EOPNOTSUPP)
			rc = uk_9p_rename(dmd1->dev, fid1, dfid2, name2);
	} else {
		return EINVAL;
	}

	return -rc;
}

static int uk_9pfs_link(struct vnode *dvp, struct vnode *svp, const char *name)
{
	struct uk_9pfs_mount_data *dmd = UK_9PFS_MD(dvp->v_mount);
	struct uk_9pfid *dfid = UK_9PFS_VFID(dvp);
	struct uk_9pfs_mount_data *smd = UK_9PFS_MD(svp->v_mount);
	struct uk_9pfid *sfid = UK_9PFS_VFID(svp);

	if (dmd->dev != smd->dev)
		return EXDEV;

	if (dmd->proto == UK_9P_PROTO_2000L)
		return -uk_9p_link(dmd->dev, dfid, sfid, name);
	else
		return EPERM;
}

static int uk_9pfs_readlink(struct vnode *vp, struct uio *uio)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(vp->v_mount);
	struct uk_9pdev *dev = md->dev;
	struct uk_9pfid *fid = UK_9PFS_VFID(vp);
	struct uk_9p_str target;
	struct iovec *iov;
	struct uk_9preq *req;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VLNK)
		return EINVAL;
	if (uio->uio_offset != 0)
		return EINVAL;

	if (!uio->uio_resid)
		return 0;

	iov = uio->uio_iov;
	while (!iov->iov_len) {
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}

	req = uk_9p_readlink(dev, fid, &target);
	if (PTRISERR(req))
		return -PTR2ERR(req);

	if (uio->uio_offset >= target.size) {
		uk_9pdev_req_remove(dev, req);
		return 0;
	}

	strlcpy((char *)iov->iov_base, target.data + uio->uio_offset,
		MIN(iov->iov_len, (size_t)target.size - uio->uio_offset + 1U));

	uio->uio_resid -= target.size;
	uio->uio_offset += target.size;

	uk_9pdev_req_remove(dev, req);
	return 0;
}

static int uk_9pfs_symlink(struct vnode *dvp, const char *op, const char *np)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(dvp->v_mount);
	struct uk_9pfid *dfid = UK_9PFS_VFID(dvp);
	struct uk_9pfid *fid;

	fid = uk_9p_symlink(md->dev, dfid, op, np, 0);
	if (PTRISERR(fid))
		return -PTR2ERR(fid);

	uk_9pfid_put(fid);
	return 0;
}

static int uk_9pfs_ioctl(struct vnode *dvp, struct vfscore_file *fp,
			 unsigned long com, void *data)
{
	/**
	 * HACK: In binary compatibility mode, Ruby tries to set O_ASYNC,
	 * which Unikraft does not yet support. If the `ioctl` call returns
	 * an error, Ruby stops working, even if it does not depend on the
	 * O_ASYNC being properly set.
	 *
	 * Until proper support, just return 0 in case an `FIONBIO` ioctl
	 * request is done, and ENOTSUP for all other cases.
	 *
	 * Setting `ioctl` to a nullop will not work, since it is used by
	 * interpreted languages (e.g. python3) to check if it should start
	 * the interpretor or just read a file.
	 *
	 * For every `ioctl` request related to a terminal, return ENOTTY.
	 */
	if (com == FIONBIO)
		return 0;
	if (IOCTL_CMD_ISTYPE(com, IOCTL_CMD_TYPE_TTY))
		return ENOTTY;

	return ENOTSUP;
}

#define uk_9pfs_seek		((vnop_seek_t)vfscore_vop_nullop)
#define uk_9pfs_cache		((vnop_cache_t)NULL)
#define uk_9pfs_fallocate	((vnop_fallocate_t)vfscore_vop_nullop)
#define uk_9pfs_poll		((vnop_poll_t)vfscore_vop_einval)

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
	.vop_symlink	= uk_9pfs_symlink,
	.vop_poll	= uk_9pfs_poll,
};
