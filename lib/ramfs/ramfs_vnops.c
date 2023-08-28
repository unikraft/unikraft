/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2006-2007, Kohsuke Ohtani
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation.
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

/*
 * rmafs_vnops.c - vnode operations for RAM file system.
 */
#define _GNU_SOURCE

#include <uk/essentials.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/param.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <uk/page.h>
#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/uio.h>
#include <vfscore/file.h>

#include "ramfs.h"
#include <dirent.h>
#include <fcntl.h>
#include <vfscore/fs.h>

/* 16 bits are enough for file mode as defined by POSIX, the rest we can use */
#define RAMFS_MODEMASK 0xffff
#define RAMFS_DELMODE 0x10000
#define RAMFS_IS_DELETED(np) ((np)->rn_mode & RAMFS_DELMODE)
#define RAMFS_MARK_DELETED(np) (np)->rn_mode |= RAMFS_DELMODE

static struct uk_mutex ramfs_lock = UK_MUTEX_INITIALIZER(ramfs_lock);
static uint64_t inode_count = 1; /* inode 0 is reserved to root */

static void
set_times_to_now(struct timespec *time1, struct timespec *time2,
		 struct timespec *time3)
{
	struct timespec now;

	clock_gettime(CLOCK_REALTIME, &now);
	if (time1)
		memcpy(time1, &now, sizeof(struct timespec));
	if (time2)
		memcpy(time2, &now, sizeof(struct timespec));
	if (time3)
		memcpy(time3, &now, sizeof(struct timespec));
}

struct ramfs_node *
ramfs_allocate_node(const char *name, int type, mode_t mode)
{
	struct ramfs_node *np;

	np = malloc(sizeof(struct ramfs_node));
	if (np == NULL)
		return NULL;
	memset(np, 0, sizeof(struct ramfs_node));

	np->rn_namelen = strlen(name);
	np->rn_name = (char *) malloc(np->rn_namelen + 1);
	if (np->rn_name == NULL) {
		free(np);
		return NULL;
	}
	strlcpy(np->rn_name, name, np->rn_namelen + 1);
	np->rn_type = type;
	np->rn_ino = inode_count++;

	mode &= 0777;
	if (type == VDIR)
		np->rn_mode = S_IFDIR|mode;
	else if (type == VLNK)
		np->rn_mode = S_IFLNK|0777;
	else
		np->rn_mode = S_IFREG|mode;

	set_times_to_now(&(np->rn_ctime), &(np->rn_atime), &(np->rn_mtime));
	np->rn_owns_buf = true;

	return np;
}

void
ramfs_free_node(struct ramfs_node *np)
{
	if (np->rn_buf != NULL && np->rn_owns_buf)
		free(np->rn_buf);

	free(np->rn_name);
	free(np);
}

static struct ramfs_node *
ramfs_add_node(struct ramfs_node *dnp, const char *name, int type, mode_t mode)
{
	struct ramfs_node *np, *prev;

	np = ramfs_allocate_node(name, type, mode);
	if (np == NULL)
		return NULL;

	uk_mutex_lock(&ramfs_lock);

	/* Link to the directory list */
	if (dnp->rn_child == NULL) {
		dnp->rn_child = np;
	} else {
		prev = dnp->rn_child;
		while (prev->rn_next != NULL)
			prev = prev->rn_next;
		prev->rn_next = np;
	}

	set_times_to_now(&(dnp->rn_mtime), &(dnp->rn_ctime), NULL);

	uk_mutex_unlock(&ramfs_lock);
	return np;
}

static int
ramfs_remove_node(struct ramfs_node *dnp, struct ramfs_node *np)
{
	struct ramfs_node *prev;

	if (dnp->rn_child == NULL)
		return EBUSY;

	uk_mutex_lock(&ramfs_lock);

	/* Unlink from the directory list */
	if (dnp->rn_child == np) {
		dnp->rn_child = np->rn_next;
	} else {
		for (prev = dnp->rn_child; prev->rn_next != np;
			 prev = prev->rn_next) {
			if (prev->rn_next == NULL) {
				uk_mutex_unlock(&ramfs_lock);
				return ENOENT;
			}
		}
		prev->rn_next = np->rn_next;
	}
	/* Mark node as deleted */
	RAMFS_MARK_DELETED(np);

	set_times_to_now(&(dnp->rn_mtime), &(dnp->rn_ctime), NULL);

	uk_mutex_unlock(&ramfs_lock);
	return 0;
}

static int
ramfs_rename_node(struct ramfs_node *np, const char *name)
{
	size_t len;
	char *tmp;

	len = strlen(name);
	if (len > NAME_MAX)
		return ENAMETOOLONG;

	if (len <= np->rn_namelen) {
		/* Reuse current name buffer */
		strlcpy(np->rn_name, name, np->rn_namelen + 1);
	} else {
		/* Expand name buffer */
		tmp = (char *) malloc(len + 1);
		if (tmp == NULL)
			return ENOMEM;
		strlcpy(tmp, name, len + 1);
		free(np->rn_name);
		np->rn_name = tmp;
	}
	np->rn_namelen = len;
	set_times_to_now(&(np->rn_ctime), NULL, NULL);
	return 0;
}

static int
ramfs_lookup(struct vnode *dvp, const char *name, struct vnode **vpp)
{
	struct ramfs_node *np, *dnp;
	struct vnode *vp;
	size_t len;
	int found;

	*vpp = NULL;

	if (*name == '\0')
		return ENOENT;

	uk_mutex_lock(&ramfs_lock);

	len = strlen(name);
	dnp = dvp->v_data;
	found = 0;
	for (np = dnp->rn_child; np != NULL; np = np->rn_next) {
		if (np->rn_namelen == len &&
			memcmp(name, np->rn_name, len) == 0) {
			found = 1;
			break;
		}
	}
	if (found == 0) {
		uk_mutex_unlock(&ramfs_lock);
		return ENOENT;
	}
	if (vfscore_vget(dvp->v_mount, np->rn_ino, &vp)) {
		/* found in cache */
		*vpp = vp;
		uk_mutex_unlock(&ramfs_lock);
		return 0;
	}
	if (!vp) {
		uk_mutex_unlock(&ramfs_lock);
		return ENOMEM;
	}
	vp->v_data = np;
	vp->v_mode = UK_ALLPERMS;
	vp->v_type = np->rn_type;
	vp->v_size = np->rn_size;

	uk_mutex_unlock(&ramfs_lock);

	*vpp = vp;

	return 0;
}

static int
ramfs_mkdir(struct vnode *dvp, const char *name, mode_t mode)
{
	struct ramfs_node *np;

	uk_pr_debug("mkdir %s\n", name);
	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	if (!S_ISDIR(mode))
		return EINVAL;

	np = ramfs_add_node(dvp->v_data, name, VDIR, mode);
	if (np == NULL)
		return ENOMEM;
	np->rn_size = 0;

	return 0;
}

static int
ramfs_symlink(struct vnode *dvp, const char *name, const char *link)
{
	struct ramfs_node *np;
	size_t len;

	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	np = ramfs_add_node(dvp->v_data, name, VLNK, 0);

	if (np == NULL)
		return ENOMEM;
	// Save the link target without the final null, as readlink() wants it.
	len = strlen(link);

	np->rn_buf = strndup(link, len);
	np->rn_bufsize = np->rn_size = len;

	return 0;
}

static int
ramfs_readlink(struct vnode *vp, struct uio *uio)
{
	struct ramfs_node *np = vp->v_data;
	size_t len;

	if (vp->v_type != VLNK)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_resid == 0)
		return 0;
	if (uio->uio_offset >= (off_t) vp->v_size)
		return 0;
	if (vp->v_size - uio->uio_offset < uio->uio_resid)
		len = vp->v_size - uio->uio_offset;
	else
		len = uio->uio_resid;

	set_times_to_now(&(np->rn_atime), NULL, NULL);
	return vfscore_uiomove(np->rn_buf + uio->uio_offset, len, uio);
}

/* Remove a directory */
static int
ramfs_rmdir(struct vnode *dvp, struct vnode *vp, const char *name __unused)
{
	return ramfs_remove_node(dvp->v_data, vp->v_data);
}

/* Remove a file */
static int
ramfs_remove(struct vnode *dvp, struct vnode *vp,
	     const char *name __maybe_unused)
{
	uk_pr_debug("remove %s in %s\n", name,
		 RAMFS_NODE(dvp)->rn_name);
	return ramfs_remove_node(dvp->v_data, vp->v_data);
}

/* Truncate file */
static int
ramfs_truncate(struct vnode *vp, off_t length)
{
	struct ramfs_node *np;
	void *new_buf;
	size_t new_size;

	uk_pr_debug("truncate %s length=%lld\n", RAMFS_NODE(vp)->rn_name,
		 (long long) length);
	np = vp->v_data;

	if (length == 0) {
		if (np->rn_buf != NULL) {
			if (np->rn_owns_buf)
				free(np->rn_buf);
			np->rn_buf = NULL;
			np->rn_bufsize = 0;
		}
	} else if ((size_t) length > np->rn_bufsize) {
		/* TODO: this could use a page level allocator */
		new_size = round_pgup(length);
		new_buf = malloc(new_size);
		if (!new_buf)
			return EIO;
		if (np->rn_size != 0) {
			memcpy(new_buf, np->rn_buf, vp->v_size);
			if (np->rn_owns_buf)
				free(np->rn_buf);
		}
		np->rn_buf = (char *) new_buf;
		np->rn_bufsize = new_size;
		np->rn_owns_buf = true;
	}
	np->rn_size = length;
	vp->v_size = length;
	set_times_to_now(&(np->rn_mtime), &(np->rn_ctime), NULL);
	return 0;
}

/*
 * Create empty file.
 */
static int
ramfs_create(struct vnode *dvp, const char *name, mode_t mode)
{
	struct ramfs_node *np;

	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	uk_pr_debug("create %s in %s\n", name, RAMFS_NODE(dvp)->rn_name);
	if (!S_ISREG(mode))
		return EINVAL;

	np = ramfs_add_node(dvp->v_data, name, VREG, mode);
	if (np == NULL)
		return ENOMEM;
	return 0;
}

static int
ramfs_read(struct vnode *vp, struct vfscore_file *fp __unused,
	   struct uio *uio, int ioflag __unused)
{
	struct ramfs_node *np =  vp->v_data;
	size_t len;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_resid == 0)
		return 0;

	if (uio->uio_offset >= (off_t) vp->v_size)
		return 0;

	if (vp->v_size - uio->uio_offset < uio->uio_resid)
		len = vp->v_size - uio->uio_offset;
	else
		len = uio->uio_resid;

	set_times_to_now(&(np->rn_atime), NULL, NULL);

	return vfscore_uiomove(np->rn_buf + uio->uio_offset, len, uio);
}

int
ramfs_set_file_data(struct vnode *vp, const void *data, size_t size)
{
	struct ramfs_node *np =  vp->v_data;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (np->rn_buf)
		return EINVAL;

	np->rn_buf = (char *) data;
	np->rn_bufsize = size;
	np->rn_size = size;
	vp->v_size = size;
	np->rn_owns_buf = false;

	return 0;
}

static int
ramfs_write(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct ramfs_node *np =  vp->v_data;

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
		uio->uio_offset = np->rn_size;

	if ((size_t) uio->uio_offset + uio->uio_resid > (size_t) vp->v_size) {
		/* Expand the file size before writing to it */
		off_t end_pos = uio->uio_offset + uio->uio_resid;

		if (end_pos > (off_t) np->rn_bufsize) {
			// XXX: this could use a page level allocator
			size_t new_size = round_pgup(end_pos);
			void *new_buf = calloc(1, new_size);

			if (!new_buf)
				return EIO;
			if (np->rn_size != 0) {
				memcpy(new_buf, np->rn_buf, vp->v_size);
				if (np->rn_owns_buf)
					free(np->rn_buf);
			}
			np->rn_buf = (char *) new_buf;
			np->rn_bufsize = new_size;
		}
		np->rn_size = end_pos;
		vp->v_size = end_pos;
		np->rn_owns_buf = true;
	}

	set_times_to_now(&(np->rn_mtime), &(np->rn_ctime), NULL);
	return vfscore_uiomove(np->rn_buf + uio->uio_offset, uio->uio_resid,
			       uio);
}

static int
ramfs_rename(struct vnode *dvp1, struct vnode *vp1, const char *name1 __unused,
	     struct vnode *dvp2, struct vnode *vp2,
	     const char *name2)
{
	struct ramfs_node *np, *old_np;
	int error;

	if (vp2) {
		/* Remove destination file, first */
		error = ramfs_remove_node(dvp2->v_data, vp2->v_data);
		if (error)
			return error;
	}
	/* Same directory ? */
	if (dvp1 == dvp2) {
		/* Change the name of existing file */
		error = ramfs_rename_node(vp1->v_data, name2);
		if (error)
			return error;
	} else {
		/* Create new file or directory */
		old_np = vp1->v_data;
		np = ramfs_add_node(dvp2->v_data, name2,
				    old_np->rn_type, old_np->rn_mode);
		if (np == NULL)
			return ENOMEM;

		if (old_np->rn_buf) {
			/* Copy file data */
			np->rn_buf = old_np->rn_buf;
			np->rn_size = old_np->rn_size;
			np->rn_bufsize = old_np->rn_bufsize;
			old_np->rn_buf = NULL;
		}
		/* Remove source file */
		ramfs_remove_node(dvp1->v_data, vp1->v_data);
	}
	return 0;
}

/*
 * @vp: vnode of the directory.
 */
static int
ramfs_readdir(struct vnode *vp, struct vfscore_file *fp, struct dirent64 *dir)
{
	struct ramfs_node *np, *dnp;
	int i;

	uk_mutex_lock(&ramfs_lock);

	set_times_to_now(&(((struct ramfs_node *) vp->v_data)->rn_atime),
			 NULL, NULL);

	if (fp->f_offset == 0) {
		dir->d_type = DT_DIR;
		strlcpy((char *) &dir->d_name, ".", sizeof(dir->d_name));
	} else if (fp->f_offset == 1) {
		dir->d_type = DT_DIR;
		strlcpy((char *) &dir->d_name, "..", sizeof(dir->d_name));
	} else {
		dnp = vp->v_data;
		np = dnp->rn_child;
		if (np == NULL) {
			uk_mutex_unlock(&ramfs_lock);
			return ENOENT;
		}

		for (i = 0; i != (fp->f_offset - 2); i++) {
			np = np->rn_next;
			if (np == NULL) {
				uk_mutex_unlock(&ramfs_lock);
				return ENOENT;
			}
		}
		if (np->rn_type == VDIR)
			dir->d_type = DT_DIR;
		else if (np->rn_type == VLNK)
			dir->d_type = DT_LNK;
		else
			dir->d_type = DT_REG;
		strlcpy((char *) &dir->d_name, np->rn_name,
				sizeof(dir->d_name));
	}
	dir->d_fileno = fp->f_offset;
//	dir->d_namelen = strlen(dir->d_name);

	fp->f_offset++;

	uk_mutex_unlock(&ramfs_lock);
	return 0;
}

int
ramfs_init(void)
{
	return 0;
}

static int
ramfs_getattr(struct vnode *vnode, struct vattr *attr)
{
	struct ramfs_node *np = vnode->v_data;

	attr->va_nodeid = vnode->v_ino;
	attr->va_size = vnode->v_size;

	attr->va_type = np->rn_type;

	memcpy(&(attr->va_atime), &(np->rn_atime), sizeof(struct timespec));
	memcpy(&(attr->va_ctime), &(np->rn_ctime), sizeof(struct timespec));
	memcpy(&(attr->va_mtime), &(np->rn_mtime), sizeof(struct timespec));

	attr->va_mode = np->rn_mode & RAMFS_MODEMASK;

	return 0;
}

static int
ramfs_setattr(struct vnode *vnode, struct vattr *attr)
{
	struct ramfs_node *np = vnode->v_data;

	if (attr->va_mask & AT_ATIME) {
		memcpy(&(np->rn_atime), &(attr->va_atime),
		       sizeof(struct timespec));
	}

	if (attr->va_mask & AT_CTIME) {
		memcpy(&(np->rn_ctime), &(attr->va_ctime),
		       sizeof(struct timespec));
	}

	if (attr->va_mask & AT_MTIME) {
		memcpy(&(np->rn_mtime), &(attr->va_mtime),
		       sizeof(struct timespec));
	}

	if (attr->va_mask & AT_MODE)
		np->rn_mode = attr->va_mode & RAMFS_MODEMASK;

	return 0;
}

static int
ramfs_inactive(struct vnode *vp)
{
	struct ramfs_node *np = RAMFS_NODE(vp);
	if (np && RAMFS_IS_DELETED(np))
		ramfs_free_node(vp->v_data);
	return 0;
}

#define ramfs_open      ((vnop_open_t)vfscore_vop_nullop)
#define ramfs_close     ((vnop_close_t)vfscore_vop_nullop)
#define ramfs_seek      ((vnop_seek_t)vfscore_vop_nullop)
#define ramfs_ioctl     ((vnop_ioctl_t)vfscore_vop_einval)
#define ramfs_fsync     ((vnop_fsync_t)vfscore_vop_nullop)
#define ramfs_link      ((vnop_link_t)vfscore_vop_eperm)
#define ramfs_fallocate ((vnop_fallocate_t)vfscore_vop_nullop)
#define ramfs_poll      ((vnop_poll_t)vfscore_vop_einval)

/*
 * vnode operations
 */
struct vnops ramfs_vnops = {
		ramfs_open,             /* open */
		ramfs_close,            /* close */
		ramfs_read,             /* read */
		ramfs_write,            /* write */
		ramfs_seek,             /* seek */
		ramfs_ioctl,            /* ioctl */
		ramfs_fsync,            /* fsync */
		ramfs_readdir,          /* readdir */
		ramfs_lookup,           /* lookup */
		ramfs_create,           /* create */
		ramfs_remove,           /* remove */
		ramfs_rename,           /* remame */
		ramfs_mkdir,            /* mkdir */
		ramfs_rmdir,            /* rmdir */
		ramfs_getattr,          /* getattr */
		ramfs_setattr,          /* setattr */
		ramfs_inactive,         /* inactive */
		ramfs_truncate,         /* truncate */
		ramfs_link,             /* link */
		(vnop_cache_t) NULL,    /* arc */
		ramfs_fallocate,        /* fallocate */
		ramfs_readlink,         /* read link */
		ramfs_symlink,          /* symbolic link */
		ramfs_poll,             /* poll */
};
