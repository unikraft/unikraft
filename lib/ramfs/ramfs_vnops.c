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
#include <uk/paging.h>
#include <uk/heap.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <vfscore/borrow.h>

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <vfscore/vnode.h>
#include <vfscore/mount.h>
#include <vfscore/uio.h>
#include <vfscore/file.h>

#include "ramfs.h"
#include <dirent.h>
#include <fcntl.h>
#include <vfscore/fs.h>

#include <uk/posix-time.h>

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
	int rc;

	rc = uk_sys_clock_gettime(CLOCK_REALTIME, &now);
	if (unlikely(rc)) {
		/*
		 * We do not consider this a fatal error, but rather
		 * something that might indicate a misconfigured kernel.
		 * Just print the error and ignore.
		 */
		uk_pr_err("Failed to set current time for vnode: %d\n", rc);
		UK_ASSERT(rc < 0);
	}

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

/* Give the buffer up: its memory when it is the file's own (a file's buffer
 * is pages, a link's target a string), and the record of it.
 */
static void
ramfs_release_buf(struct ramfs_node *np)
{
	if (np->rn_buf && np->rn_owns_buf) {
		if (np->rn_type == VLNK)
			free(np->rn_buf);
		else
			uk_heap_pfree(np->rn_buf,
				      UK_PAGING_PAGE_COUNT(np->rn_bufsize));
	}
	np->rn_buf = NULL;
	np->rn_bufsize = 0;
	np->rn_backed = 0;
	np->rn_owns_buf = true;
}

void
ramfs_free_node(struct ramfs_node *np)
{
	ramfs_release_buf(np);
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
			return ENOSPC;
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
		return ENOSPC;
	np->rn_size = 0;

	return 0;
}

static int
ramfs_symlink(struct vnode *dvp, const char *name, const char *link)
{
	struct ramfs_node *np;
	char *target;
	size_t len;

	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

	// Save the link target without the final null, as readlink() wants it.
	len = strlen(link);
	target = strndup(link, len);
	if (target == NULL)
		return ENOSPC;

	np = ramfs_add_node(dvp->v_data, name, VLNK, 0);

	if (np == NULL) {
		free(target);
		return ENOSPC;
	}
	np->rn_buf = target;
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

/* What vfscore_uiomove() takes at most, which counts in int */
#define RAMFS_COPY_MAX		((size_t) (INT_MAX & ~(UK_PAGING_PAGE_SIZE - 1)))

/* The address of the buffer's page @page */
static inline __vaddr_t
ramfs_page_addr(struct ramfs_node *np, unsigned long page)
{
	return (__vaddr_t) np->rn_buf + (page << UK_PAGING_PAGE_SHIFT);
}

/* Give the frames behind the buffer's pages [@from, @to) back, the ones the
 * allocator keeps excepted, and say from which page on some are gone.
 */
static unsigned long
ramfs_release_pages(struct ramfs_node *np, unsigned long from,
		    unsigned long to)
{
	__vaddr_t lowest;

	lowest = uk_heap_release_range(np->rn_buf,
				       UK_PAGING_PAGE_COUNT(np->rn_bufsize),
				       ramfs_page_addr(np, from),
				       ramfs_page_addr(np, to));
	return (lowest - (__vaddr_t) np->rn_buf) >> UK_PAGING_PAGE_SHIFT;
}

/* Give the file a buffer that holds @need bytes, its contents kept. A write
 * asks for room to grow (@grow): half as much again as the buffer has, so
 * that a file written in pieces is copied a bounded number of times over,
 * not once per piece, which took minutes for 50 MB with the whole guest
 * waiting. The room costs address space, not memory: only the contents'
 * pages have memory behind them, the room's is taken as the file grows into
 * it (see ramfs_back()) and given back when a truncate cuts the file
 * (ramfs_release_room()), so a nearly full filesystem fails no earlier than
 * a file grown to its exact size did. The copy of a borrowed buffer that
 * stays within it, and the buffer a deep cut leaves (ramfs_release_room()),
 * take the exact size. The buffer is not zeroed past the contents: nothing
 * reads past the file's size, and what grows the file inside the buffer
 * zeroes the gap it leaves.
 */
static int
ramfs_grow(struct ramfs_node *np, size_t need, bool grow)
{
	size_t exact = UK_PAGING_PAGE_ALIGN_UP(need);
	size_t new_size = exact;
	void *new_buf;
	int rc;

	UK_ASSERT(need >= np->rn_size);
	if (grow)
		new_size = MAX(exact, UK_PAGING_PAGE_ALIGN_UP(
				np->rn_bufsize + np->rn_bufsize / 2));
	rc = uk_heap_palloc_backed(UK_PAGING_PAGE_COUNT(new_size),
				   UK_PAGING_PAGE_COUNT(exact), &new_buf);
	if (rc == -ENOSPC && new_size > exact) {
		/* No run of pages that long, or no frames for the few more the
		 * room's kept pages take: the exact size may fit still
		 */
		new_size = exact;
		rc = uk_heap_palloc_backed(UK_PAGING_PAGE_COUNT(exact),
					   UK_PAGING_PAGE_COUNT(exact),
					   &new_buf);
	}
	if (rc)
		return ENOSPC;
	if (np->rn_size != 0)
		memcpy(new_buf, np->rn_buf, np->rn_size);
	ramfs_release_buf(np);
	np->rn_buf = (char *) new_buf;
	np->rn_bufsize = new_size;
	np->rn_backed = UK_PAGING_PAGE_COUNT(exact);
	return 0;
}

/* Pages taken again at once, at least: a write a page at a time would ask
 * for each.
 */
#define RAMFS_BACK_PAGES	16

/* Have memory behind the buffer up to @upto bytes, where the room past the
 * file's end has none (see ramfs_grow(), ramfs_release_room()): a write into
 * a page without would fault with no frame to take, which the kernel cannot
 * survive. The frames may not be there: ENOSPC, and what the asking backed
 * goes back, but for the pages the allocator keeps anyway.
 */
static int
ramfs_back(struct ramfs_node *np, size_t upto)
{
	unsigned long need = UK_PAGING_PAGE_COUNT(upto);
	unsigned long ahead = MIN(UK_PAGING_PAGE_COUNT(np->rn_bufsize),
				  ALIGN_UP(need, RAMFS_BACK_PAGES));
	int rc;

	if (need <= np->rn_backed)
		return 0;
	rc = uk_heap_back_range(ramfs_page_addr(np, np->rn_backed),
				ramfs_page_addr(np, ahead));
	if (rc && ahead > need) {
		/* No room ahead; the pages the write needs may be there */
		ramfs_release_pages(np, need, ahead);
		ahead = need;
		rc = uk_heap_back_range(ramfs_page_addr(np, np->rn_backed),
					ramfs_page_addr(np, ahead));
	}
	if (rc) {
		ramfs_release_pages(np, np->rn_backed, ahead);
		if (rc != -ENOMEM)
			uk_pr_warn("Could not back a file's pages: %d\n", rc);
		return rc == -ENOMEM ? ENOSPC : EIO;
	}
	np->rn_backed = ahead;
	return 0;
}

/* Room for the file's first @upto bytes, with memory behind them: a buffer
 * of the file's own, with room to grow, when the one it has is borrowed
 * (see ramfs_set_file_data(): it may be read-only, so it is copied before
 * the first change, sized for it) or too small; memory behind the pages of
 * the one it has otherwise. A truncate gets room as a write does: a file
 * extended a step at a time is copied a bounded number of times over
 * either way, and the room costs no memory.
 */
static int
ramfs_reserve(struct ramfs_node *np, size_t upto)
{
	if (np->rn_buf && !np->rn_owns_buf)
		return ramfs_grow(np, MAX(np->rn_size, upto),
				  upto > np->rn_size);
	if (upto > np->rn_bufsize)
		return ramfs_grow(np, upto, true);
	return ramfs_back(np, upto);
}

/* A cut past which a buffer is given up for one of the file's size, its
 * address space given back, when what is left to copy is small: the copy
 * is the price, the address space is only scarce in the extreme
 */
#define RAMFS_CUT_PAGES		64
#define RAMFS_CUT_COPY_PAGES	4096

/* After a truncate cut the file: give the frames behind the room past its
 * end back, but for the few pages the next append would take again at once
 * (a write takes no more, see ramfs_back()), and the buffer itself when the
 * cut is deep, for one the file's size, the old one's address space given
 * back with it. A buffer that cannot be had leaves the old one, its frames
 * given back all the same.
 */
static void
ramfs_release_room(struct ramfs_node *np)
{
	unsigned long keep = UK_PAGING_PAGE_COUNT(np->rn_size) +
			     RAMFS_BACK_PAGES;
	unsigned long pages = UK_PAGING_PAGE_COUNT(np->rn_bufsize);
	bool deep;

	if (!np->rn_buf || !np->rn_owns_buf)
		return;
	if (keep < np->rn_backed)
		np->rn_backed = ramfs_release_pages(np, keep, np->rn_backed);
	deep = pages > keep && pages - keep >= MAX(RAMFS_CUT_PAGES, pages / 2) &&
	       keep <= RAMFS_CUT_COPY_PAGES;
	if (deep)
		ramfs_grow(np, np->rn_size, false);
}

/* Truncate file */
static int
ramfs_truncate(struct vnode *vp, off_t length)
{
	struct ramfs_node *np;
	bool cut;
	int rc;

	uk_pr_debug("truncate %s length=%lld\n", RAMFS_NODE(vp)->rn_name,
		 (long long) length);
	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG || length < 0)
		return EINVAL;
	np = vp->v_data;

	if (length == 0) {
		ramfs_release_buf(np);
	} else if ((size_t) length > np->rn_size) {
		rc = ramfs_reserve(np, length);
		if (rc)
			return rc;
		/* Past its old end the file reads as zeros: not what the
		 * buffer holds, nor what an earlier truncate cut off.
		 */
		memset(np->rn_buf + np->rn_size, 0, length - np->rn_size);
	}
	cut = (size_t) length < np->rn_size;
	np->rn_size = length;
	vp->v_size = length;
	if (cut)
		ramfs_release_room(np);
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
		return ENOSPC;
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
	len = MIN(len, RAMFS_COPY_MAX);

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
	np->rn_backed = 0;
	np->rn_size = size;
	vp->v_size = size;
	np->rn_owns_buf = false;

	return 0;
}

static int
ramfs_write(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct ramfs_node *np =  vp->v_data;
	size_t end_pos, len;
	int rc;

	if (vp->v_type == VDIR)
		return EISDIR;
	if (vp->v_type != VREG)
		return EINVAL;
	if (ioflag & IO_APPEND)
		uio->uio_offset = np->rn_size;
	if (uio->uio_offset < 0)
		return EINVAL;
	if (uio->uio_offset >= LONG_MAX)
		return EFBIG;
	if (uio->uio_resid == 0)
		return 0;
	/* As on Linux, a write is cut short where an offset ends, and to what
	 * vfscore_uiomove() takes, which counts in int. The request is left
	 * as it is: what vfscore reports written is what the copy took.
	 */
	len = MIN((size_t) uio->uio_resid, (size_t) (LONG_MAX - uio->uio_offset));
	len = MIN(len, RAMFS_COPY_MAX);

	end_pos = uio->uio_offset + len;

	if (end_pos > (size_t) vp->v_size ||
	    (np->rn_buf && !np->rn_owns_buf)) {
		/* Room for the write, and the file's end moved to it */
		size_t old_size = np->rn_size;

		rc = ramfs_reserve(np, end_pos);
		if (rc)
			return rc;
		/* The gap between the old end and the write reads as zeros,
		 * not what the buffer holds or a truncate cut off.
		 */
		if ((size_t) uio->uio_offset > old_size)
			memset(np->rn_buf + old_size, 0,
			       uio->uio_offset - old_size);
	}

	set_times_to_now(&(np->rn_mtime), &(np->rn_ctime), NULL);
	rc = vfscore_uiomove(np->rn_buf + uio->uio_offset, len, uio);
	/* The file ends where the copy got to, not where it was to: what
	 * the buffer holds past that is nobody's to read
	 */
	end_pos = uio->uio_offset;
	if (end_pos > np->rn_size) {
		np->rn_size = end_pos;
		vp->v_size = end_pos;
	}
	return rc;
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
			return ENOSPC;

		/* Copy children structure */
		np->rn_child = old_np->rn_child;
		old_np->rn_child = NULL;

		if (old_np->rn_buf) {
			/* Copy file data */
			np->rn_buf = old_np->rn_buf;
			np->rn_size = old_np->rn_size;
			np->rn_bufsize = old_np->rn_bufsize;
			np->rn_backed = old_np->rn_backed;
			np->rn_owns_buf = old_np->rn_owns_buf;
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

static int ramfs_ioctl(struct vnode *dvp,
			 struct vfscore_file *fp __unused,
			 unsigned long com,
			 void *data)
{
	if (com == VFSCORE_IOC_BORROW) {
		const struct vfscore_borrow *b = data;

		return ramfs_set_file_data(dvp, b->data, b->len);
	}

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

#define ramfs_open      ((vnop_open_t)vfscore_vop_nullop)
#define ramfs_close     ((vnop_close_t)vfscore_vop_nullop)
#define ramfs_seek      ((vnop_seek_t)vfscore_vop_nullop)
#define ramfs_fsync     ((vnop_fsync_t)vfscore_vop_nullop)
#define ramfs_link      ((vnop_link_t)vfscore_vop_eperm)
#define ramfs_fallocate ((vnop_fallocate_t)vfscore_vop_einval)
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
