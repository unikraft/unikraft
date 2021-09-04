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

#include <dirent.h>
#include <fcntl.h>
#include <vfscore/fs.h>
#include <uk/list.h>

static struct uk_mutex rustfs_lock = UK_MUTEX_INITIALIZER(rustfs_lock);
static uint64_t inode_count = 1; /* inode 0 is reserved to root */
extern struct uk_list_head rustfs_head;

static int
rustfs_shim_lookup(struct vnode *dvp, char *name, struct vnode **vpp)
{
	struct rustfs_node *np, *entry;
	struct vnode *vp;
	size_t len;
	int found;
    int size;
	struct uk_list_head *pos;


	*vpp = NULL;
	np = NULL;

	if (*name == '\0')
		return ENOENT;

	uk_mutex_lock(&rustfs_lock);

	len = strlen(name);
	found = 0;

	// TODO 11: Search in the list the entry with the given name
    found = rustfs_lookup(&np, name, &size);

	if (found == -1) {
		uk_mutex_unlock(&rustfs_lock);
		return ENOENT;
	}

	if (vfscore_vget(dvp->v_mount, inode_count++, &vp)) {
		 /* found in cache */ 
		*vpp = vp;
		uk_mutex_unlock(&rustfs_lock);
		return 0;
	}

	if (!vp) {
		uk_mutex_unlock(&rustfs_lock);
		return ENOMEM;
	}


    vp->v_data = np;
    vp->v_size = size;
	// TODO 12: vp is a vnode, np is a rustfs_node
	// we need to link these toghether
	// we also need to provide the permisions
	// and you can use the UK_ALLPERMS macro
	// and the file size

	uk_mutex_unlock(&rustfs_lock);

	*vpp = vp;

	return 0;
}



/* Remove a file */
static int
rustfs_shim_remove(struct vnode *dvp, struct vnode *vp, char *name __maybe_unused)
{
	return rustfs_remove(vp->v_data);
}



/*
 * Create empty file.
 */
static int
rustfs_shim_create(struct vnode *dvp, char *name, mode_t mode)
{
	struct rustfs_node *np;

	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	if (!S_ISREG(mode))
		return EINVAL;

    int res = rustfs_create(&np, name);

	//Todo 13: Add a new node in the hierarchy

	return res;
}

static int
rustfs_shim_read(struct vnode *vp, struct vfscore_file *fp __unused,
	   struct uio *uio, int ioflag __unused)
{
	struct rustfs_node *np =  vp->v_data;
	size_t len;

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

    return rustfs_read(np, uio);
	// TODO 14: read from file (buffer)
}



static int
rustfs_shim_write(struct vnode *vp, struct uio *uio, int ioflag)
{
	void *np =  vp->v_data;

	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_offset >= LONG_MAX)
		return EFBIG;
	if (uio->uio_resid == 0)
		return 0;

	//if (ioflag & IO_APPEND)
	//	uio->uio_offset = np->rn_size;
    //TODO: add append support

	/*if ((size_t) uio->uio_offset + uio->uio_resid > (size_t) vp->v_size) {

		/* Expand the file size before writing to it */
		/*off_t end_pos = uio->uio_offset + uio->uio_resid;


		if (end_pos > (off_t) np->rn_bufsize) {
			// XXX: this could use a page level allocator
			size_t new_size = round_pgup(end_pos);
			void *new_buf = calloc(1, new_size);

			if (!new_buf)
				return EIO;
			if (np->rn_size != 0) {

				memcpy(new_buf, np->rn_buf, vp->v_size);
					free(np->rn_buf);
			}
			np->rn_buf = (char *) new_buf;
			np->rn_bufsize = new_size;
		}
		np->rn_size = end_pos;
		vp->v_size = end_pos;
	}*/

    return rustfs_write(np, uio);
	// TODO 15: Write to file (buffer)
}








#define rustfs_rename_node ((vnop_rename_node_t)vfscore_vop_nullop)
#define rustfs_set_file_data ((vnop_set_file_data_t)vfscore_vop_nullop)
#define rustfs_rename ((vnop_rename_t)vfscore_vop_nullop)
#define rustfs_open      ((vnop_open_t)vfscore_vop_nullop)
#define rustfs_close     ((vnop_close_t)vfscore_vop_nullop)
#define rustfs_seek      ((vnop_seek_t)vfscore_vop_nullop)
#define rustfs_ioctl     ((vnop_ioctl_t)vfscore_vop_einval)
#define rustfs_fsync     ((vnop_fsync_t)vfscore_vop_nullop)
#define rustfs_inactive  ((vnop_inactive_t)vfscore_vop_nullop)
#define rustfs_readdir ((vnop_readdir_t)vfscore_vop_nullop)
#define rustfs_mkdir ((vnop_mkdir_t)vfscore_vop_nullop)
#define rustfs_rmdir ((vnop_rmdir_t)vfscore_vop_nullop)
#define rustfs_getattr ((vnop_getattr_t)vfscore_vop_nullop)
#define rustfs_setattr	((vnop_setattr_t)vfscore_vop_nullop)
#define rustfs_truncate ((vnop_truncate_t)vfscore_vop_nullop)
#define rustfs_link      ((vnop_link_t)vfscore_vop_eperm)
#define rustfs_cache ((vnop_cache_t)vfscore_vop_nullop)
#define rustfs_fallocate ((vnop_fallocate_t)vfscore_vop_nullop)
#define rustfs_readlink ((vnop_readlink_t)vfscore_vop_nullop)
#define rustfs_symlink ((vnop_symlink_t)vfscore_vop_nullop)

/*
 * vnode operations
 */

// TODO 1 : define a `struct vnops` structure and fill the operations
struct vnops rustfs_vnops = {
		rustfs_open,             /* open */
		rustfs_close,            /* close */
		rustfs_shim_read,             /* read */
		rustfs_shim_write,            /* write */
		rustfs_seek,             /* seek */
		rustfs_ioctl,            /* ioctl */
		rustfs_fsync,            /* fsync */
		rustfs_readdir,          /* readdir */
		rustfs_shim_lookup,           /* lookup */
		rustfs_shim_create,           /* create */
		rustfs_shim_remove,           /* remove */
		rustfs_rename,           /* remame */
		rustfs_mkdir,            /* mkdir */
		rustfs_rmdir,            /* rmdir */
		rustfs_getattr,          /* getattr */
		rustfs_setattr,          /* setattr */
		rustfs_inactive,         /* inactive */
		rustfs_truncate,         /* truncate */
		rustfs_link,             /* link */
		(vnop_cache_t) NULL,    /* arc */
		rustfs_fallocate,        /* fallocate */
		rustfs_readlink,         /* read link */
		rustfs_symlink,          /* symbolic link */
};

