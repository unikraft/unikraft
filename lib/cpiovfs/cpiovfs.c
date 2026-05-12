/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * cpiovfs: CPIO archive filesystem for Unikraft
 *
 * Mounts a CPIO archive (from initrd) serving file data directly from
 * archive memory without extraction. Supports ephemeral in-memory
 * directories and files (for mount points, temp files, etc.) while
 * archive-backed content remains zero-copy.
 *
 * Copyright (c) 2025, Microsoft Corporation. All rights reserved.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <fcntl.h>

#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/plat/memory.h>
#include <uk/cpio.h>

#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/dentry.h>
#include <vfscore/uio.h>
#include <vfscore/file.h>
#include <vfscore/fs.h>

/*
 * CPIOVFS node: represents a file, directory, or symlink in the archive.
 * File/symlink data points directly into the CPIO archive memory.
 */
struct cpiovfs_node {
	char *name;
	size_t namelen;
	int type;          /* VDIR, VREG, VLNK */
	mode_t mode;
	uint64_t ino;
	const char *data;  /* pointer into CPIO archive (files/symlinks) */
	size_t size;
	struct cpiovfs_node *child; /* first child (directories) */
	struct cpiovfs_node *next;  /* next sibling */
	int ephemeral;     /* 1 = in-memory node (data is malloc'd) */
	size_t alloc;      /* allocated capacity for ephemeral files */
};

#define CPIOVFS_NODE(vp) ((struct cpiovfs_node *)(vp)->v_data)

static uint64_t cpiovfs_ino_counter = 1;

static struct cpiovfs_node *
cpiovfs_alloc_node(const char *name, size_t namelen, int type, mode_t mode)
{
	struct cpiovfs_node *np;

	np = calloc(1, sizeof(*np));
	if (!np)
		return NULL;

	np->name = malloc(namelen + 1);
	if (!np->name) {
		free(np);
		return NULL;
	}
	memcpy(np->name, name, namelen);
	np->name[namelen] = '\0';
	np->namelen = namelen;
	np->type = type;
	np->mode = mode;
	np->ino = cpiovfs_ino_counter++;

	return np;
}

/* Find a child by name in a directory node */
static struct cpiovfs_node *
cpiovfs_find_child(struct cpiovfs_node *dir, const char *name, size_t namelen)
{
	struct cpiovfs_node *c;

	for (c = dir->child; c; c = c->next) {
		if (c->namelen == namelen && !memcmp(c->name, name, namelen))
			return c;
	}
	return NULL;
}

/* Add a child node to a directory */
static struct cpiovfs_node *
cpiovfs_add_child(struct cpiovfs_node *dir,
		  const char *name, size_t namelen,
		  int type, mode_t mode)
{
	struct cpiovfs_node *existing, *np;

	existing = cpiovfs_find_child(dir, name, namelen);
	if (existing)
		return existing;

	np = cpiovfs_alloc_node(name, namelen, type, mode);
	if (!np)
		return NULL;

	np->next = dir->child;
	dir->child = np;
	return np;
}

/*
 * Navigate path components, creating intermediate directories as needed.
 * Returns the parent directory node for the final component.
 */
static struct cpiovfs_node *
cpiovfs_ensure_parents(struct cpiovfs_node *root, const char *path)
{
	struct cpiovfs_node *cur = root;
	const char *p = path;
	const char *slash;
	size_t len;

	while (*p == '/')
		p++;

	while (*p) {
		slash = strchr(p, '/');
		if (!slash)
			break; /* last component = leaf name */

		len = slash - p;
		if (len == 0) {
			p = slash + 1;
			continue;
		}

		cur = cpiovfs_add_child(cur, p, len,
					VDIR, S_IFDIR | 0755);
		if (!cur)
			return NULL;

		p = slash + 1;
	}

	return cur;
}

/* Get the leaf name and its length from a path */
static const char *
cpiovfs_leaf(const char *path, size_t *leaflen)
{
	const char *p, *last_slash;

	/* Strip trailing slash */
	size_t plen = strlen(path);

	while (plen > 0 && path[plen - 1] == '/')
		plen--;

	if (plen == 0) {
		*leaflen = 0;
		return path;
	}

	last_slash = NULL;
	for (p = path; p < path + plen; p++) {
		if (*p == '/')
			last_slash = p;
	}

	if (last_slash) {
		*leaflen = (path + plen) - (last_slash + 1);
		return last_slash + 1;
	}

	*leaflen = plen;
	return path;
}

/*
 * Build the filesystem tree from a CPIO archive.
 * Only metadata is stored; file data pointers reference the archive directly.
 */
static struct cpiovfs_node *
cpiovfs_build_tree(const void *buf, size_t buflen)
{
	struct cpiovfs_node *root, *parent, *np;
	const struct uk_cpio_header *header;
	const void *end;
	uint32_t namesize, filesize, mode;
	const char *filename, *data, *path;
	const char *leaf;
	size_t leaflen;

	root = cpiovfs_alloc_node("", 0, VDIR, S_IFDIR | 0755);
	if (!root)
		return NULL;

	header = (const struct uk_cpio_header *)buf;
	end = (const char *)buf + buflen;

	while ((const void *)header < end && ukcpio_valid_magic(header)) {
		if (UKCPIO_ISLAST(header))
			break;

		namesize = UKCPIO_U32FIELD(header->namesize);
		filesize = UKCPIO_U32FIELD(header->filesize);
		mode = UKCPIO_U32FIELD(header->mode);

		filename = UKCPIO_FILENAME(header);
		data = UKCPIO_DATA(header, namesize);

		/* Skip "." entry */
		if (namesize <= 2 && filename[0] == '.') {
			if (namesize == 1 ||
			    (namesize == 2 && filename[1] == '\0')) {
				header = UKCPIO_NEXT(header,
						     namesize, filesize);
				continue;
			}
		}

		/* Strip leading "./" */
		path = filename;
		if (path[0] == '.' && path[1] == '/')
			path += 2;

		/* Skip empty paths */
		if (!*path) {
			header = UKCPIO_NEXT(header, namesize, filesize);
			continue;
		}

		parent = cpiovfs_ensure_parents(root, path);
		if (!parent) {
			header = UKCPIO_NEXT(header, namesize, filesize);
			continue;
		}

		leaf = cpiovfs_leaf(path, &leaflen);
		if (leaflen == 0) {
			/* Directory path ending in '/' */
			header = UKCPIO_NEXT(header, namesize, filesize);
			continue;
		}

		if (UKCPIO_IS_DIR(mode)) {
			cpiovfs_add_child(parent, leaf, leaflen,
					  VDIR, S_IFDIR | (mode & 0777));
		} else if (UKCPIO_IS_SYMLINK(mode)) {
			np = cpiovfs_add_child(parent, leaf, leaflen,
					       VLNK, S_IFLNK | 0777);
			if (np) {
				np->data = data;
				np->size = filesize;
			}
		} else if (UKCPIO_IS_FILE(mode)) {
			np = cpiovfs_add_child(parent, leaf, leaflen,
					       VREG, S_IFREG | (mode & 0777));
			if (np) {
				np->data = data;
				np->size = filesize;
			}
		}

		header = UKCPIO_NEXT(header, namesize, filesize);
	}

	return root;
}

/* ---- VFS Operations ---- */

static int
cpiovfs_mount(struct mount *mp, const char *dev __unused,
	      int flags __unused, const void *data __unused)
{
	struct ukplat_memregion_desc *initrd;
	struct cpiovfs_node *root;
	int rc;

	rc = ukplat_memregion_find_initrd0(&initrd);
	if (rc < 0 || initrd->len == 0) {
		uk_pr_err("cpiovfs: initrd0 not found\n");
		return EIO;
	}

	uk_pr_info("cpiovfs: mounting initrd @ %p (%lu bytes)\n",
		   (void *)initrd->vbase, (unsigned long)initrd->len);

	root = cpiovfs_build_tree((const void *)initrd->vbase, initrd->len);
	if (!root)
		return ENOMEM;

	mp->m_root->d_vnode->v_data = root;
	mp->m_data = root;
	return 0;
}

static int
cpiovfs_unmount(struct mount *mp, int flags __unused)
{
	vfscore_release_mp_dentries(mp);
	return 0;
}

#define cpiovfs_sync   ((vfsop_sync_t)vfscore_nullop)
#define cpiovfs_vget   ((vfsop_vget_t)vfscore_nullop)
#define cpiovfs_statfs ((vfsop_statfs_t)vfscore_nullop)

/* ---- VNode Operations ---- */

static int
cpiovfs_lookup(struct vnode *dvp, const char *name, struct vnode **vpp)
{
	struct cpiovfs_node *dnp = dvp->v_data;
	struct cpiovfs_node *np;
	struct vnode *vp;
	size_t len;

	*vpp = NULL;

	if (*name == '\0')
		return ENOENT;

	len = strlen(name);
	for (np = dnp->child; np; np = np->next) {
		if (np->namelen == len && !memcmp(name, np->name, len))
			break;
	}
	if (!np)
		return ENOENT;

	if (vfscore_vget(dvp->v_mount, np->ino, &vp)) {
		/* found in cache */
		*vpp = vp;
		return 0;
	}
	if (!vp)
		return ENOMEM;

	vp->v_data = np;
	vp->v_mode = UK_ALLPERMS;
	vp->v_type = np->type;
	vp->v_size = np->size;

	*vpp = vp;
	return 0;
}

static int
cpiovfs_read(struct vnode *vp, struct vfscore_file *fp __unused,
	     struct uio *uio, int ioflag __unused)
{
	struct cpiovfs_node *np = vp->v_data;
	size_t len;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_resid == 0)
		return 0;
	if (uio->uio_offset >= (off_t)np->size)
		return 0;

	if (np->size - (size_t)uio->uio_offset < (size_t)uio->uio_resid)
		len = np->size - (size_t)uio->uio_offset;
	else
		len = (size_t)uio->uio_resid;

	return vfscore_uiomove((void *)(np->data + uio->uio_offset), len, uio);
}

static int
cpiovfs_readdir(struct vnode *vp, struct vfscore_file *fp,
		struct dirent64 *dir)
{
	struct cpiovfs_node *dnp = vp->v_data;
	struct cpiovfs_node *np;
	int i;

	if (fp->f_offset == 0) {
		dir->d_type = DT_DIR;
		strlcpy((char *)&dir->d_name, ".", sizeof(dir->d_name));
	} else if (fp->f_offset == 1) {
		dir->d_type = DT_DIR;
		strlcpy((char *)&dir->d_name, "..", sizeof(dir->d_name));
	} else {
		np = dnp->child;
		if (!np)
			return ENOENT;

		for (i = 0; i != (fp->f_offset - 2); i++) {
			np = np->next;
			if (!np)
				return ENOENT;
		}

		if (np->type == VDIR)
			dir->d_type = DT_DIR;
		else if (np->type == VLNK)
			dir->d_type = DT_LNK;
		else
			dir->d_type = DT_REG;
		strlcpy((char *)&dir->d_name, np->name,
			sizeof(dir->d_name));
	}

	dir->d_fileno = fp->f_offset;
	fp->f_offset++;
	return 0;
}

static int
cpiovfs_getattr(struct vnode *vp, struct vattr *attr)
{
	struct cpiovfs_node *np = vp->v_data;

	attr->va_nodeid = np->ino;
	attr->va_size = np->size;
	attr->va_type = np->type;
	attr->va_mode = np->mode;

	memset(&attr->va_atime, 0, sizeof(struct timespec));
	memset(&attr->va_ctime, 0, sizeof(struct timespec));
	memset(&attr->va_mtime, 0, sizeof(struct timespec));

	return 0;
}

static int
cpiovfs_readlink(struct vnode *vp, struct uio *uio)
{
	struct cpiovfs_node *np = vp->v_data;
	size_t len;

	if (vp->v_type != VLNK)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_resid == 0)
		return 0;
	if (uio->uio_offset >= (off_t)np->size)
		return 0;

	if (np->size - (size_t)uio->uio_offset < (size_t)uio->uio_resid)
		len = np->size - (size_t)uio->uio_offset;
	else
		len = (size_t)uio->uio_resid;

	return vfscore_uiomove((void *)(np->data + uio->uio_offset), len, uio);
}

static int
cpiovfs_ioctl(struct vnode *dvp __unused,
	      struct vfscore_file *fp __unused,
	      unsigned long com, void *data __unused)
{
	if (com == FIONBIO)
		return 0;
	if (IOCTL_CMD_ISTYPE(com, IOCTL_CMD_TYPE_TTY))
		return ENOTTY;
	return ENOTSUP;
}

static int
cpiovfs_inactive(struct vnode *vp __unused)
{
	return 0;
}

/* ---- Ephemeral write operations (in-memory, not backed by CPIO) ---- */

static int
cpiovfs_mkdir_impl(struct vnode *dvp, const char *name, mode_t mode)
{
	struct cpiovfs_node *dnp = dvp->v_data;
	struct cpiovfs_node *np;
	size_t len = strlen(name);

	if (len > 255)
		return ENAMETOOLONG;
	if (!S_ISDIR(mode))
		return EINVAL;

	np = cpiovfs_add_child(dnp, name, len, VDIR, mode);
	if (!np)
		return ENOMEM;
	np->ephemeral = 1;
	return 0;
}

static int
cpiovfs_create_impl(struct vnode *dvp, const char *name, mode_t mode)
{
	struct cpiovfs_node *dnp = dvp->v_data;
	struct cpiovfs_node *np;
	size_t len = strlen(name);

	if (len > 255)
		return ENAMETOOLONG;

	np = cpiovfs_add_child(dnp, name, len, VREG, mode);
	if (!np)
		return ENOMEM;
	np->ephemeral = 1;
	return 0;
}

static int
cpiovfs_write_impl(struct vnode *vp, struct uio *uio, int ioflag __unused)
{
	struct cpiovfs_node *np = vp->v_data;
	size_t off, end, need;
	char *newbuf;

	if (!np->ephemeral)
		return EROFS;
	if (vp->v_type != VREG)
		return EINVAL;
	if (uio->uio_offset < 0)
		return EINVAL;

	off = (size_t)uio->uio_offset;
	end = off + uio->uio_resid;

	if (end > np->alloc) {
		need = end < 4096 ? 4096 : end * 2;
		newbuf = realloc((void *)np->data, need);
		if (!newbuf)
			return ENOMEM;
		memset(newbuf + np->size, 0, need - np->size);
		np->data = newbuf;
		np->alloc = need;
	}

	int rc = vfscore_uiomove((void *)(np->data + off), uio->uio_resid, uio);
	if (rc)
		return rc;

	if (end > np->size) {
		np->size = end;
		vp->v_size = end;
	}
	return 0;
}

static int
cpiovfs_truncate_impl(struct vnode *vp, off_t length)
{
	struct cpiovfs_node *np = vp->v_data;

	if (!np->ephemeral)
		return EROFS;
	if (vp->v_type != VREG)
		return EINVAL;

	if ((size_t)length > np->alloc) {
		size_t need = (size_t)length < 4096 ? 4096 : (size_t)length;
		char *newbuf = realloc((void *)np->data, need);
		if (!newbuf)
			return ENOMEM;
		memset(newbuf + np->size, 0, need - np->size);
		np->data = newbuf;
		np->alloc = need;
	}
	np->size = (size_t)length;
	vp->v_size = (size_t)length;
	return 0;
}

static int
cpiovfs_symlink_impl(struct vnode *dvp, const char *name, const char *link)
{
	struct cpiovfs_node *dnp = dvp->v_data;
	struct cpiovfs_node *np;
	size_t nlen = strlen(name);
	size_t llen = strlen(link);

	if (nlen > 255)
		return ENAMETOOLONG;

	np = cpiovfs_add_child(dnp, name, nlen, VLNK, S_IFLNK | 0777);
	if (!np)
		return ENOMEM;
	np->ephemeral = 1;
	np->data = strndup(link, llen);
	if (!np->data)
		return ENOMEM;
	np->size = llen;
	return 0;
}

static int
cpiovfs_remove_impl(struct vnode *dvp, struct vnode *vp,
		    const char *name __unused)
{
	struct cpiovfs_node *np = vp->v_data;

	if (!np->ephemeral)
		return EROFS;

	struct cpiovfs_node *dnp = dvp->v_data;
	struct cpiovfs_node **pp;
	for (pp = &dnp->child; *pp; pp = &(*pp)->next) {
		if (*pp == np) {
			*pp = np->next;
			if (np->data && np->alloc)
				free((void *)np->data);
			free(np->name);
			free(np);
			return 0;
		}
	}
	return ENOENT;
}

#define cpiovfs_open      ((vnop_open_t)vfscore_vop_nullop)
#define cpiovfs_close     ((vnop_close_t)vfscore_vop_nullop)
#define cpiovfs_seek      ((vnop_seek_t)vfscore_vop_nullop)
#define cpiovfs_fsync     ((vnop_fsync_t)vfscore_vop_nullop)
#define cpiovfs_rename    ((vnop_rename_t)vfscore_vop_erofs)
#define cpiovfs_rmdir     ((vnop_rmdir_t)vfscore_vop_erofs)
#define cpiovfs_setattr   ((vnop_setattr_t)vfscore_vop_nullop)
#define cpiovfs_link      ((vnop_link_t)vfscore_vop_erofs)
#define cpiovfs_fallocate ((vnop_fallocate_t)vfscore_vop_einval)
#define cpiovfs_poll      ((vnop_poll_t)vfscore_vop_einval)

struct vnops cpiovfs_vnops = {
	cpiovfs_open,
	cpiovfs_close,
	cpiovfs_read,
	cpiovfs_write_impl,
	cpiovfs_seek,
	cpiovfs_ioctl,
	cpiovfs_fsync,
	cpiovfs_readdir,
	cpiovfs_lookup,
	cpiovfs_create_impl,
	cpiovfs_remove_impl,
	cpiovfs_rename,
	cpiovfs_mkdir_impl,
	cpiovfs_rmdir,
	cpiovfs_getattr,
	cpiovfs_setattr,
	cpiovfs_inactive,
	cpiovfs_truncate_impl,
	cpiovfs_link,
	(vnop_cache_t) NULL,
	cpiovfs_fallocate,
	cpiovfs_readlink,
	cpiovfs_symlink_impl,
	cpiovfs_poll,
};

struct vfsops cpiovfs_vfsops = {
	cpiovfs_mount,
	cpiovfs_unmount,
	cpiovfs_sync,
	cpiovfs_vget,
	cpiovfs_statfs,
	&cpiovfs_vnops,
};

static struct vfscore_fs_type fs_cpiovfs = {
	.vs_name = "cpiovfs",
	.vs_init = NULL,
	.vs_op = &cpiovfs_vfsops,
};

UK_FS_REGISTER(fs_cpiovfs);
