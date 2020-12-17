/*
 * Copyright (c) 2005-2008, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/prex.h>

#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/dirent.h>
#include <sys/buf.h>

#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "fatfs.h"

/*
 *  Time bits: 15-11 hours (0-23), 10-5 min, 4-0 sec /2
 *  Date bits: 15-9 year - 1980, 8-5 month, 4-0 day
 */
#define TEMP_DATE   0x3021
#define TEMP_TIME   0

#define fatfs_open	((vnop_open_t)vop_nullop)
#define fatfs_close	((vnop_close_t)vop_nullop)
static int fatfs_read	(vnode_t, file_t, void *, size_t, size_t *);
static int fatfs_write	(vnode_t, file_t, void *, size_t, size_t *);
#define fatfs_seek	((vnop_seek_t)vop_nullop)
#define fatfs_ioctl	((vnop_ioctl_t)vop_einval)
#define fatfs_fsync	((vnop_fsync_t)vop_nullop)
static int fatfs_readdir(vnode_t, file_t, struct dirent *);
static int fatfs_lookup	(vnode_t, char *, vnode_t);
static int fatfs_create	(vnode_t, char *, mode_t);
static int fatfs_remove	(vnode_t, vnode_t, char *);
static int fatfs_rename	(vnode_t, vnode_t, char *, vnode_t, vnode_t, char *);
static int fatfs_mkdir	(vnode_t, char *, mode_t);
static int fatfs_rmdir	(vnode_t, vnode_t, char *);
static int fatfs_getattr(vnode_t, struct vattr *);
static int fatfs_setattr(vnode_t, struct vattr *);
static int fatfs_inactive(vnode_t);
static int fatfs_truncate(vnode_t, off_t);

/*
 * vnode operations
 */
struct vnops fatfs_vnops = {
	fatfs_open,		/* open */
	fatfs_close,		/* close */
	fatfs_read,		/* read */
	fatfs_write,		/* write */
	fatfs_seek,		/* seek */
	fatfs_ioctl,		/* ioctl */
	fatfs_fsync,		/* fsync */
	fatfs_readdir,		/* readdir */
	fatfs_lookup,		/* lookup */
	fatfs_create,		/* create */
	fatfs_remove,		/* remove */
	fatfs_rename,		/* remame */
	fatfs_mkdir,		/* mkdir */
	fatfs_rmdir,		/* rmdir */
	fatfs_getattr,		/* getattr */
	fatfs_setattr,		/* setattr */
	fatfs_inactive,		/* inactive */
	fatfs_truncate,		/* truncate */
};

/*
 * Read one cluster to buffer.
 */
static int
fat_read_cluster(struct fatfsmount *fmp, u_long cluster)
{
	u_long sec;
	size_t size;

	sec = cl_to_sec(fmp, cluster);
	size = fmp->sec_per_cl * SEC_SIZE;
	return device_read(fmp->dev, fmp->io_buf, &size, sec);
}

/*
 * Write one cluster from buffer.
 */
static int
fat_write_cluster(struct fatfsmount *fmp, u_long cluster)
{
	u_long sec;
	size_t size;

	sec = cl_to_sec(fmp, cluster);
	size = fmp->sec_per_cl * SEC_SIZE;
	return device_write(fmp->dev, fmp->io_buf, &size, sec);
}

/*
 * Lookup vnode for the specified file/directory.
 * The vnode data will be set properly.
 */
static int
fatfs_lookup(vnode_t dvp, char *name, vnode_t vp)
{
	struct fatfsmount *fmp;
	struct fat_dirent *de;
	struct fatfs_node *np;
	int error;

	if (*name == '\0')
		return ENOENT;

	fmp = vp->v_mount->m_data;
	mutex_lock(&fmp->lock);

	DPRINTF(("fatfs_lookup: name=%s\n", name));

	np = vp->v_data;
	error = fatfs_lookup_node(dvp, name, np);
	if (error) {
		DPRINTF(("fatfs_lookup: failed!! name=%s\n", name));
		mutex_unlock(&fmp->lock);
		return error;
	}
	de = &np->dirent;
	vp->v_type = IS_DIR(de) ? VDIR : VREG;
	fat_attr_to_mode(de->attr, &vp->v_mode);
	vp->v_mode = ALLPERMS;
	vp->v_size = de->size;
	vp->v_blkno = de->cluster;

	DPRINTF(("fatfs_lookup: cl=%d\n", de->cluster));
	mutex_unlock(&fmp->lock);
	return 0;
}

static int
fatfs_read(vnode_t vp, file_t fp, void *buf, size_t size, size_t *result)
{
	struct fatfsmount *fmp;
	int nr_read, nr_copy, buf_pos, error;
	u_long cl, file_pos;

	DPRINTF(("fatfs_read: vp=%x\n", vp));

	*result = 0;
	fmp = vp->v_mount->m_data;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;

	/* Check if current file position is already end of file. */
	file_pos = fp->f_offset;
	if (file_pos >= vp->v_size)
		return 0;

	mutex_lock(&fmp->lock);

	/* Get the actual read size. */
	if (vp->v_size - file_pos < size)
		size = vp->v_size - file_pos;

	/* Seek to the cluster for the file offset */
	error = fat_seek_cluster(fmp, vp->v_blkno, file_pos, &cl);
	if (error)
		goto out;

	/* Read and copy data */
	nr_read = 0;
	buf_pos = file_pos % fmp->cluster_size;
	do {
		if (fat_read_cluster(fmp, cl)) {
			error = EIO;
			goto out;
		}

		nr_copy = fmp->cluster_size;
		if (buf_pos > 0)
			nr_copy -= buf_pos;
		if (buf_pos + size < fmp->cluster_size)
			nr_copy = size;
		memcpy(buf, fmp->io_buf + buf_pos, nr_copy);

		file_pos += nr_copy;
		nr_read += nr_copy;
		size -= nr_copy;
		if (size <= 0)
			break;

		error = fat_next_cluster(fmp, cl, &cl);
		if (error)
			goto out;

		buf = (void *)((u_long)buf + nr_copy);
		buf_pos = 0;
	} while (!IS_EOFCL(fmp, cl));

	fp->f_offset = file_pos;
	*result = nr_read;
	error = 0;
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

static int
fatfs_write(vnode_t vp, file_t fp, void *buf, size_t size, size_t *result)
{
	struct fatfsmount *fmp;
	struct fatfs_node *np;
	struct fat_dirent *de;
	int nr_copy, nr_write, buf_pos, i, cl_size, error;
	u_long file_pos, end_pos;
	u_long cl;

	DPRINTF(("fatfs_write: vp=%x\n", vp));

	*result = 0;
	fmp = vp->v_mount->m_data;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;

	mutex_lock(&fmp->lock);

	/* Check if file position exceeds the end of file. */
	end_pos = vp->v_size;
	file_pos = (fp->f_flags & O_APPEND) ? end_pos : fp->f_offset;
	if (file_pos + size > end_pos) {
		/* Expand the file size before writing to it */
		end_pos = file_pos + size;
		error = fat_expand_file(fmp, vp->v_blkno, end_pos);
		if (error) {
			error = EIO;
			goto out;
		}

		/* Update directory entry */
		np = vp->v_data;
		de = &np->dirent;
		de->size = end_pos;
		error = fatfs_put_node(fmp, np);
		if (error)
			goto out;
		vp->v_size = end_pos;
	}

	/* Seek to the cluster for the file offset */
	error = fat_seek_cluster(fmp, vp->v_blkno, file_pos, &cl);
	if (error)
		goto out;

	buf_pos = file_pos % fmp->cluster_size;
	cl_size = size / fmp->cluster_size + 1;
	nr_write = 0;
	i = 0;
	do {
		/* First and last cluster must be read before write */
		if (i == 0 || i == cl_size) {
			if (fat_read_cluster(fmp, cl)) {
				error = EIO;
				goto out;
			}
		}
		nr_copy = fmp->cluster_size;
		if (buf_pos > 0)
			nr_copy -= buf_pos;
		if (buf_pos + size < fmp->cluster_size)
			nr_copy = size;
		memcpy(fmp->io_buf + buf_pos, buf, nr_copy);

		if (fat_write_cluster(fmp, cl)) {
			error = EIO;
			goto out;
		}
		file_pos += nr_copy;
		nr_write += nr_copy;
		size -= nr_copy;
		if (size <= 0)
			break;

		error = fat_next_cluster(fmp, cl, &cl);
		if (error)
			goto out;

		buf = (void *)((u_long)buf + nr_copy);
		buf_pos = 0;
		i++;
	} while (!IS_EOFCL(fmp, cl));

	fp->f_offset = file_pos;

	/*
	 * XXX: Todo!
	 *    de.time = ?
	 *    de.date = ?
	 *    if (dirent_set(fp, &de))
	 *        return EIO;
	 */
	*result = nr_write;
	error = 0;
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

static int
fatfs_readdir(vnode_t vp, file_t fp, struct dirent *dir)
{
	struct fatfsmount *fmp;
	struct fatfs_node np;
	struct fat_dirent *de;
	int error;

	fmp = vp->v_mount->m_data;
	mutex_lock(&fmp->lock);

	error = fatfs_get_node(vp, fp->f_offset, &np);
	if (error)
		goto out;
	de = &np.dirent;
	fat_restore_name((char *)&de->name, dir->d_name);

	if (de->attr & FA_SUBDIR)
		dir->d_type = DT_DIR;
	else if (de->attr & FA_DEVICE)
		dir->d_type = DT_BLK;
	else
		dir->d_type = DT_REG;

	dir->d_fileno = fp->f_offset;
	dir->d_namlen = strlen(dir->d_name);

	fp->f_offset++;
	error = 0;
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

/*
 * Create empty file.
 */
static int
fatfs_create(vnode_t dvp, char *name, mode_t mode)
{
	struct fatfsmount *fmp;
	struct fatfs_node np;
	struct fat_dirent *de;
	u_long cl;
	int error;

	DPRINTF(("fatfs_create: %s\n", name));

	if (!S_ISREG(mode))
		return EINVAL;

	if (!fat_valid_name(name))
		return EINVAL;

	fmp = dvp->v_mount->m_data;
	mutex_lock(&fmp->lock);

	/* Allocate free cluster for new file. */
	error = fat_alloc_cluster(fmp, 0, &cl);
	if (error)
		goto out;

	de = &np.dirent;
	memset(de, 0, sizeof(struct fat_dirent));
	fat_convert_name(name, (char *)de->name);
	de->cluster = cl;
	de->time = TEMP_TIME;
	de->date = TEMP_DATE;
	fat_mode_to_attr(mode, &de->attr);
	error = fatfs_add_node(dvp, &np);
	if (error)
		goto out;
	error = fat_set_cluster(fmp, cl, fmp->fat_eof);
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

static int
fatfs_remove(vnode_t dvp, vnode_t vp, char *name)
{
	struct fatfsmount *fmp;
	struct fatfs_node np;
	struct fat_dirent *de;
	int error;

	if (*name == '\0')
		return ENOENT;

	fmp = dvp->v_mount->m_data;
	mutex_lock(&fmp->lock);

	error = fatfs_lookup_node(dvp, name, &np);
	if (error)
		goto out;
	de = &np.dirent;
	if (IS_DIR(de)) {
		error = EISDIR;
		goto out;
	}
	if (!IS_FILE(de)) {
		error = EPERM;
		goto out;
	}

	/* Remove clusters */
	error = fat_free_clusters(fmp, de->cluster);
	if (error)
		goto out;

	/* remove directory */
	de->name[0] = 0xe5;
	error = fatfs_put_node(fmp, &np);
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

static int
fatfs_rename(vnode_t dvp1, vnode_t vp1, char *name1,
	     vnode_t dvp2, vnode_t vp2, char *name2)
{
	struct fatfsmount *fmp;
	struct fatfs_node np1;
	struct fat_dirent *de1, *de2;
	int error;

	fmp = dvp1->v_mount->m_data;
	mutex_lock(&fmp->lock);

	error = fatfs_lookup_node(dvp1, name1, &np1);
	if (error)
		goto out;
	de1 = &np1.dirent;

	if (IS_FILE(de1)) {
		/* Remove destination file, first */
		error = fatfs_remove(dvp2, vp1, name2);
		if (error == EIO)
			goto out;

		/* Change file name of directory entry */
		fat_convert_name(name2, (char *)de1->name);

		/* Same directory ? */
		if (dvp1 == dvp2) {
			/* Change the name of existing file */
			error = fatfs_put_node(fmp, &np1);
			if (error)
				goto out;
		} else {
			/* Create new directory entry */
			error = fatfs_add_node(dvp2, &np1);
			if (error)
				goto out;

			/* Remove souce file */
			error = fatfs_remove(dvp1, vp2, name1);
			if (error)
				goto out;
		}
	} else {

		/* remove destination directory */
		error = fatfs_rmdir(dvp2, NULL, name2);
		if (error == EIO)
			goto out;

		/* Change file name of directory entry */
		fat_convert_name(name2, (char *)de1->name);

		/* Same directory ? */
		if (dvp1 == dvp2) {
			/* Change the name of existing directory */
			error = fatfs_put_node(fmp, &np1);
			if (error)
				goto out;
		} else {
			/* Create new directory entry */
			error = fatfs_add_node(dvp2, &np1);
			if (error)
				goto out;

			/* Update "." and ".." for renamed directory */
			if (fat_read_cluster(fmp, de1->cluster)) {
				error = EIO;
				goto out;
			}

			de2 = (struct fat_dirent *)fmp->io_buf;
			de2->cluster = de1->cluster;
			de2->time = TEMP_TIME;
			de2->date = TEMP_DATE;
			de2++;
			de2->cluster = dvp2->v_blkno;
			de2->time = TEMP_TIME;
			de2->date = TEMP_DATE;

			if (fat_write_cluster(fmp, de1->cluster)) {
				error = EIO;
				goto out;
			}

			/* Remove souce directory */
			error = fatfs_rmdir(dvp1, NULL, name1);
			if (error)
				goto out;
		}
	}
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

static int
fatfs_mkdir(vnode_t dvp, char *name, mode_t mode)
{
	struct fatfsmount *fmp;
	struct fatfs_node np;
	struct fat_dirent *de;
	u_long cl;
	int error;

	if (!S_ISDIR(mode))
		return EINVAL;

	if (!fat_valid_name(name))
		return ENOTDIR;

	fmp = dvp->v_mount->m_data;
	mutex_lock(&fmp->lock);

	/* Allocate free cluster for directory data */
	error = fat_alloc_cluster(fmp, 0, &cl);
	if (error)
		goto out;

	memset(&np, 0, sizeof(struct fatfs_node));
	de = &np.dirent;
	fat_convert_name(name, (char *)&de->name);
	de->cluster = cl;
	de->time = TEMP_TIME;
	de->date = TEMP_DATE;
	fat_mode_to_attr(mode, &de->attr);
	error = fatfs_add_node(dvp, &np);
	if (error)
		goto out;

	/* Initialize "." and ".." for new directory */
	memset(fmp->io_buf, 0, fmp->cluster_size);

	de = (struct fat_dirent *)fmp->io_buf;
	memcpy(de->name, ".          ", 11);
	de->attr = FA_SUBDIR;
	de->cluster = cl;
	de->time = TEMP_TIME;
	de->date = TEMP_DATE;
	de++;
	memcpy(de->name, "..         ", 11);
	de->attr = FA_SUBDIR;
	de->cluster = dvp->v_blkno;
	de->time = TEMP_TIME;
	de->date = TEMP_DATE;

	if (fat_write_cluster(fmp, cl)) {
		error = EIO;
		goto out;
	}
	/* Add eof */
	error = fat_set_cluster(fmp, cl, fmp->fat_eof);
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

/*
 * remove can be done only with empty directory
 */
static int
fatfs_rmdir(vnode_t dvp, vnode_t vp, char *name)
{
	struct fatfsmount *fmp;
	struct fatfs_node np;
	struct fat_dirent *de;
	int error;

	if (*name == '\0')
		return ENOENT;

	fmp = dvp->v_mount->m_data;
	mutex_lock(&fmp->lock);

	error = fatfs_lookup_node(dvp, name, &np);
	if (error)
		goto out;

	de = &np.dirent;
	if (!IS_DIR(de)) {
		error = ENOTDIR;
		goto out;
	}

	/* Remove clusters */
	error = fat_free_clusters(fmp, de->cluster);
	if (error)
		goto out;

	/* remove directory */
	de->name[0] = 0xe5;

	error = fatfs_put_node(fmp, &np);
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

static int
fatfs_getattr(vnode_t vp, struct vattr *vap)
{
	/* XXX */
	return 0;
}

static int
fatfs_setattr(vnode_t vp, struct vattr *vap)
{
	/* XXX */
	return 0;
}


static int
fatfs_inactive(vnode_t vp)
{

	free(vp->v_data);
	return 0;
}

static int
fatfs_truncate(vnode_t vp, off_t length)
{
	struct fatfsmount *fmp;
	struct fatfs_node *np;
	struct fat_dirent *de;
	int error;

	fmp = vp->v_mount->m_data;
	mutex_lock(&fmp->lock);

	np = vp->v_data;
	de = &np->dirent;

	if (length == 0) {
		/* Remove clusters */
		error = fat_free_clusters(fmp, de->cluster);
		if (error)
			goto out;
	} else if (length > vp->v_size) {
		error = fat_expand_file(fmp, vp->v_blkno, length);
		if (error) {
			error = EIO;
			goto out;
		}
	}

	/* Update directory entry */
	de->size = length;
	error = fatfs_put_node(fmp, np);
	if (error)
		goto out;
	vp->v_size = length;
 out:
	mutex_unlock(&fmp->lock);
	return error;
}

int
fatfs_init(void)
{
	return 0;
}
