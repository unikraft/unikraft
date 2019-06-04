/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
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

#define _BSD_SOURCE

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/param.h>

#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include "vfs.h"

static ssize_t
read_link(struct vnode *vp, char *buf, size_t bufsz, ssize_t *sz)
{
	struct iovec iov = {buf, bufsz};
	struct uio   uio = {&iov, 1, 0, (ssize_t) bufsz, UIO_READ};
	int rc;

	*sz = 0;
	vn_lock(vp);
	rc  = VOP_READLINK(vp, &uio);
	vn_unlock(vp);

	if (rc != 0) {
		return (rc);
	}

	*sz = bufsz - uio.uio_resid;
	return (0);
}

int
namei_follow_link(struct dentry *dp, char *node, char *name, char *fp, size_t mountpoint_len)
{
	char link[PATH_MAX];
	char t[PATH_MAX];
	int     error;
	ssize_t sz;
	char    *p;
	int     c;

	error = read_link(dp->d_vnode, link, PATH_MAX, &sz);
	if (error != 0) {
		return (error);
	}
	link[sz] = 0;

	p = fp + mountpoint_len + strlen(node);
	c = strlen(node) - strlen(name) - 1;
	node[c] = 0;

	if (link[0] == '/') {
		strlcat(link, p, PATH_MAX);
		strlcpy(fp, link, PATH_MAX);
	} else {
		strlcpy(t, p, PATH_MAX);
		strlcpy(node, fp, mountpoint_len + c + 1);
		path_conv(node, link, fp);
		strlcat(fp, t, PATH_MAX);
	}
	node[0] = 0;
	name[0] = 0;
	return (0);
}
/*
 * Convert a pathname into a pointer to a dentry
 *
 * @path: full path name.
 * @dpp:  dentry to be returned.
 */
int
namei(const char *path, struct dentry **dpp)
{
	char *p;
	char node[PATH_MAX];
	char name[PATH_MAX];
	char fp[PATH_MAX];
	struct mount *mp;
	struct dentry *dp, *ddp;
	struct vnode *dvp, *vp;
	int error, i;
	int links_followed;
	int need_continue;

	DPRINTF(VFSDB_VNODE, ("namei: path=%s\n", path));

	links_followed = 0;
	strlcpy(fp, path, PATH_MAX);

	do {
		need_continue = 0;
		/*
		 * Convert a full path name to its mount point and
		 * the local node in the file system.
		 */
		if (vfs_findroot(fp, &mp, &p)) {
			return ENOTDIR;
		}
		int mountpoint_len = p - fp - 1;
		strlcpy(node, "/", sizeof(node));
		strlcat(node, p, sizeof(node));
		dp = dentry_lookup(mp, node);
		if (dp) {
			/* vnode is already active. */
			*dpp = dp;
			return 0;
		}
		/*
		 * Find target vnode, started from root directory.
		 * This is done to attach the fs specific data to
		 * the target vnode.
		 */
		ddp = mp->m_root;
		if (!ddp) {
			UK_CRASH("VFS: no root");
		}
		dref(ddp);

		node[0] = '\0';

		while (*p != '\0') {
			/*
			 * Get lower directory/file name.
			 */
			while (*p == '/') {
				p++;
			}

			if (*p == '\0') {
				break;
			}

			for (i = 0; i < PATH_MAX; i++) {
				if (*p == '\0' || *p == '/') {
					break;
				}
				name[i] = *p++;
			}
			name[i] = '\0';

			/*
			 * Get a vnode for the target.
			 */
			strlcat(node, "/", sizeof(node));
			strlcat(node, name, sizeof(node));
			dvp = ddp->d_vnode;
			vn_lock(dvp);
			dp = dentry_lookup(mp, node);
			if (dp == NULL) {
				/* Find a vnode in this directory. */
				error = VOP_LOOKUP(dvp, name, &vp);
				if (error) {
					vn_unlock(dvp);
					drele(ddp);
					return error;
				}

				dp = dentry_alloc(ddp, vp, node);
				vput(vp);

				if (!dp) {
					vn_unlock(dvp);
					drele(ddp);
					return ENOMEM;
				}
			}
			vn_unlock(dvp);
			drele(ddp);
			ddp = dp;

			if (dp->d_vnode->v_type == VLNK) {
				error = namei_follow_link(dp, node, name, fp, mountpoint_len);
				if (error) {
					drele(dp);
					return (error);
				}

				drele(dp);

				p       = fp;
				dp      = NULL;
				ddp     = NULL;
				vp      = NULL;
				dvp     = NULL;
				name[0] = 0;
				node[0] = 0;

				if (++links_followed >= MAXSYMLINKS) {
					return (ELOOP);
				}
				need_continue = 1;
				break;
			}

			if (*p == '/' && ddp->d_vnode->v_type != VDIR) {
				drele(ddp);
				return ENOTDIR;
			}
		}
	} while (need_continue);

	*dpp = dp;
	return 0;
}

/*
 * Convert last component in the path to pointer to dentry
 *
 * @path: full path name
 * @ddp : pointer to dentry of parent
 * @dpp : dentry to be returned
 */
int
namei_last_nofollow(char *path, struct dentry *ddp, struct dentry **dpp)
{
	char          *name;
	int           error;
	struct mount  *mp;
	char          *p;
	struct dentry *dp;
	struct vnode  *dvp;
	struct vnode  *vp;
	char node[PATH_MAX];

	dvp  = NULL;

	if (path[0] != '/') {
		return (ENOTDIR);
	}

	name = strrchr(path, '/');
	if (name == NULL) {
		return (ENOENT);
	}
	name++;

	error = vfs_findroot(path, &mp, &p);
	if (error != 0) {
		return (ENOTDIR);
	}

	strlcpy(node, "/", PATH_MAX);
	strlcat(node, p, PATH_MAX);

	// We want to treat things like /tmp/ the same as /tmp. Best way to do that
	// is to ignore the last character, except when we're stating the root.
	int l = strlen(node) - 1;
	if (l && node[l] == '/') {
		node[l] = '\0';
	}

	dvp = ddp->d_vnode;
	vn_lock(dvp);
	dp = dentry_lookup(mp, node);
	if (dp == NULL) {
		error = VOP_LOOKUP(dvp, name, &vp);
		if (error != 0) {
			goto out;
		}

		dp = dentry_alloc(ddp, vp, node);
		vput(vp);

		if (dp == NULL) {
			error = ENOMEM;
			goto out;
		}
	}

	*dpp  = dp;
	error = 0;
out:
	if (dvp != NULL) {
		vn_unlock(dvp);
	}
	return (error);
}

/*
 * Search a pathname.
 * This is a very central but not so complicated routine. ;-P
 *
 * @path: full path.
 * @dpp:  pointer to dentry for directory.
 * @name: if non-null, pointer to file name in path.
 *
 * This routine returns a locked directory vnode and file name.
 */
int
lookup(char *path, struct dentry **dpp, char **name)
{
	char buf[PATH_MAX];
	char root[] = "/";
	char *file, *dir;
	struct dentry *dp;
	int error;

	DPRINTF(VFSDB_VNODE, ("lookup: path=%s\n", path));

	/*
	 * Get the path for directory.
	 */
	strlcpy(buf, path, sizeof(buf));
	file = strrchr(buf, '/');
	if (!buf[0]) {
		return ENOTDIR;
	}
	if (file == buf) {
		dir = root;
	} else {
		*file = '\0';
		dir = buf;
	}
	/*
	 * Get the vnode for directory
	 */
	if ((error = namei(dir, &dp)) != 0) {
		return error;
	}
	if (dp->d_vnode->v_type != VDIR) {
		drele(dp);
		return ENOTDIR;
	}

	*dpp = dp;

	if (name) {
		/*
		 * Get the file name
		 */
		*name = strrchr(path, '/') + 1;
	}
	return 0;
}

/*
 * vnode_init() is called once (from vfs_init)
 * in initialization.
 */
void
lookup_init(void)
{
	dentry_init();
}
