/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Robert Hrusecky <roberth@cs.utexas.edu>
 *          Omar Jamil <omarj2898@gmail.com>
 *          Sachin Beldona <sachinbeldona@utexas.edu>
 *          Andrei Tatar <andrei@unikraft.io>
 *
 * Copyright (c) 2017, The University of Texas at Austin. All rights reserved.
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

#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#include <uk/arch/paging.h>
#include <uk/assert.h>
#include <uk/compat_list.h>
#include <uk/print.h>
#include <uk/cpio.h>
#include <uk/essentials.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <uk/allocregion.h>
#include <uk/arch/limits.h>
#include <unistd.h>
#include <utime.h>

/* Raw filesystem syscalls; not provided by headers */
int uk_syscall_do_open(const char *, int, mode_t);
int uk_syscall_do_close(int);
ssize_t uk_syscall_do_write(int, const void *, size_t);
int uk_syscall_do_chmod(const char *, mode_t);
int uk_syscall_do_utime(const char *, const struct utimbuf *);
int uk_syscall_do_mkdir(const char *, mode_t);
int uk_syscall_do_symlink(const char *, const char *);
int uk_syscall_do_link(const char *, const char *);
int uk_syscall_do_stat(const char *, struct stat *);
int uk_syscall_do_unlinkat(int, const char *, int);
int uk_syscall_do_rename(const char *, const char *);

struct cpio_ilist_elm {
	uint32_t ino;
	UK_SLIST_ENTRY(struct cpio_ilist_elm) next;
	char path[];
};

struct cpio_ilist {
	struct uk_allocregion *ar;
	UK_SLIST_HEAD(ilist_head, struct cpio_ilist_elm) elms;
};

static inline
struct cpio_ilist_elm *cpio_ilist_elm_create(struct cpio_ilist *ilist,
					     uint32_t ino,
					     const char *path,
					     size_t pathlen)
{
	struct cpio_ilist_elm *elm;

	elm = uk_allocregion_bump(ilist->ar, sizeof(*elm) + pathlen);
	if (unlikely(!elm))
		return NULL;

	elm->ino = ino;
	/* Includes terminating null byte */
	memcpy(elm->path, path, pathlen);

	return elm;
}

static inline
struct cpio_ilist_elm *cpio_ilist_elm_upto(struct cpio_ilist *ilist,
					   uint64_t ino)
{
	struct cpio_ilist_elm *elm, *next_elm;

	if (UK_SLIST_EMPTY(&ilist->elms) ||
	    UK_SLIST_FIRST(&ilist->elms)->ino > ino)
		return NULL;

	/* Search in the ascendingly sorted list */
	UK_SLIST_FOREACH_SAFE(elm, &ilist->elms, next, next_elm) {
		/*
		 * If we already encountered this multi-hardlink inode
		 * just return it.
		 */
		if (elm->ino == ino)
			break;

		/*
		 * If we have reached the last element
		 * OR
		 * If current element's inode is not the one we are looking for
		 * and the next element's inode is bigger, then we haven't
		 * encountered the inode we are looking for and this is the
		 * first time we are seeing it so return current element to be
		 * able to insert right after it and maintain the list
		 * ascendingly sorted.
		 */
		if (!next_elm || next_elm->ino > ino)
			break;
	}

	return elm;
}

static int
try_rm_nonempty_dir(const char *path)
{
	/* rm -rf is nontrivial and costly, so we rename for speed */
	/* TODO: revisit if efficient recursive dir removal is available */
	int r;
	char newpath[PATH_MAX];
	char *newend = newpath + strlcpy(newpath, path, PATH_MAX);

	if (unlikely(newend - newpath + 2 > PATH_MAX)) {
		uk_pr_err("Cannot rename %s, path too long\n", path);
		return -ENAMETOOLONG;
	}
	strcpy(newend, ".0");
	r = uk_syscall_do_rename(path, newpath);
	uk_pr_info("Rename '%s' to '%s': %d\n", path, newpath, r);
	return r;
}

static int
try_remove(const char *path)
{
	int r;

	r = uk_syscall_do_unlinkat(AT_FDCWD, path, 0);
	uk_pr_info("Unlink %s: %d\n", path, r);
	if (!r || r == -ENOENT)
		return 0;

	r = uk_syscall_do_unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
	uk_pr_info("Rmdir %s: %d\n", path, r);
	if (!r || r == -ENOENT)
		return 0;
	return try_rm_nonempty_dir(path);
}

static int
write_file(int fd, const char *contents, size_t len)
{
	while (len) {
		ssize_t written = uk_syscall_do_write(fd, contents, len);

		if (unlikely(written < 0))
			return written;
		UK_ASSERT(len >= (size_t)written);
		contents += written;
		len -= written;
	}
	return 0;
}

static enum ukcpio_error
extract_file(const char *path, const char *contents, size_t len,
	     mode_t mode, uint32_t mtime)
{
	int ret = UKCPIO_SUCCESS;
	int fd;
	int err;
	struct utimbuf times = {mtime, mtime};

	uk_pr_info("Extracting %s (%zu bytes)\n", path, len);

	fd = uk_syscall_do_open(path, O_CREAT | O_WRONLY | O_EXCL, 0);
	if (unlikely(fd == -EEXIST)) {
		uk_pr_info("Path exists, trying removal\n");
		err = try_remove(path);
		if (unlikely(err))
			/* Not fatal, following open may still succeed */
			uk_pr_warn("%s: Path cleanup failed: %s (%d)\n",
				   path, strerror(-err), -err);
		fd = uk_syscall_do_open(path, O_CREAT | O_WRONLY | O_TRUNC, 0);
	}
	if (unlikely(fd < 0)) {
		uk_pr_err("%s: Failed to create file: %s (%d)\n",
			  path, strerror(-fd), -fd);
		ret = -UKCPIO_FILE_CREATE_FAILED;
		goto out;
	}

	err = write_file(fd, contents, len);
	if (unlikely(err)) {
		uk_pr_err("%s: Failed to load content: %s (%d)\n",
			  path, strerror(-err), -err);
		ret = -UKCPIO_FILE_WRITE_FAILED;
		goto close_out;
	}

	err = uk_syscall_do_chmod(path, mode);
	if (unlikely(err))
		uk_pr_warn("%s: Failed to chmod: %s (%d)\n",
			   path, strerror(-err), -err);

	err = uk_syscall_do_utime(path, &times);
	if (unlikely(err))
		uk_pr_warn("%s: Failed to set modification time: %s (%d)",
			   path, strerror(-err), -err);

close_out:
	err = uk_syscall_do_close(fd);
	if (unlikely(err)) {
		uk_pr_err("%s: Failed to close file: %s (%d)\n",
			  path, strerror(-err), -err);
		if (!ret)
			ret = -UKCPIO_FILE_CLOSE_FAILED;
	}
out:
	return ret;
}

static enum ukcpio_error
extract_dir(const char *path, mode_t mode)
{
	int r;

	uk_pr_info("Creating directory %s\n", path);
	r = uk_syscall_do_mkdir(path, mode);
	if (unlikely(r == -EEXIST)) {
		struct stat pstat;

		uk_pr_info("Path exists, checking type\n");
		r = uk_syscall_do_stat(path, &pstat);
		if (unlikely(r < 0)) {
			uk_pr_warn("%s: Cannot stat path: %s (%d)\n",
				   path, strerror(-r), -r);
			r = -EEXIST;
			goto err_out;
		}
		if (!S_ISDIR(pstat.st_mode)) {
			/* Path exists but is not dir; remove & try again */
			uk_pr_info("Path exists and not dir, trying removal\n");
			r = try_remove(path);
			if (unlikely(r)) {
				uk_pr_warn("%s: Path cleanup failed: %s (%d)\n",
					   path, strerror(-r), -r);
				goto err_out;
			}
			r = uk_syscall_do_mkdir(path, mode);
			if (unlikely(r))
				goto err_out;
			else
				return UKCPIO_SUCCESS;
		}

		/* Directory already exists, set mode only */
		uk_pr_info("Path exists and is dir, doing chmod\n");
		r = uk_syscall_do_chmod(path, mode);
		if (unlikely(r < 0))
			uk_pr_warn("%s: Failed to chmod: %s (%d)\n",
				   path, strerror(-r), -r);
	} else if (unlikely(r < 0)) {
		goto err_out;
	}
	return UKCPIO_SUCCESS;

err_out:
	uk_pr_err("%s: Failed to create directory: %s (%d)\n",
		  path, strerror(-r), -r);
	return -UKCPIO_MKDIR_FAILED;
}

static enum ukcpio_error
extract_symlink(const char *path, const char *contents, size_t len)
{
	int r;
	char target[len + 1];

	uk_pr_info("Creating symlink %s\n", path);
	/* NUL not guaranteed at the end of contents; need to copy */
	memcpy(target, contents, len);
	target[len] = 0;

	uk_pr_info("%s: Target is %s\n", path, target);
	r = uk_syscall_do_symlink(target, path);
	if (r == -EEXIST) {
		uk_pr_info("Path exists, trying removal\n");
		r = try_remove(path);
		if (!r)
			r = uk_syscall_do_symlink(target, path);
	}
	if (likely(!r))
		return UKCPIO_SUCCESS;

	uk_pr_err("%s: Failed to create symlink: %s (%d)\n",
		  path, strerror(-r), -r);
	return -UKCPIO_SYMLINK_FAILED;
}

static enum ukcpio_error
extract_section(struct cpio_ilist *ilist,
		const struct uk_cpio_header **headerp, char *fullpath,
		const char *eof, size_t prefixlen)
{
	const struct uk_cpio_header *header = *headerp;
	uint32_t mode = UKCPIO_U32FIELD(header->mode);
	uint32_t filesize = UKCPIO_U32FIELD(header->filesize);
	uint32_t namesize = UKCPIO_U32FIELD(header->namesize);
	uint32_t mtime = UKCPIO_U32FIELD(header->mtime);
	uint32_t nlink = UKCPIO_U32FIELD(header->nlink);
	const char *fname = UKCPIO_FILENAME(header);
	const char *data = UKCPIO_DATA(header, namesize);
	struct cpio_ilist_elm *elm, *new_elm;
	enum ukcpio_error err;
	uint32_t ino;
	int rc;

	if (unlikely(fname + namesize > eof)) {
		uk_pr_err("File name exceeds archive bounds at %p\n", header);
		return -UKCPIO_MALFORMED_INPUT;
	}
	if (unlikely(data + filesize > eof)) {
		uk_pr_err("File exceeds archive bounds: %s\n", fname);
		return -UKCPIO_MALFORMED_INPUT;
	}

	/* namesize includes trailing NUL */
	if (unlikely(prefixlen + namesize > PATH_MAX)) {
		uk_pr_err("Resulting path too long: %s\n", fname);
		return -UKCPIO_MALFORMED_INPUT;
	}
	memcpy(fullpath + prefixlen, fname, namesize);

	if (nlink > 1 && strcmp(".", fname)) {
		ino = UKCPIO_U32FIELD(header->inode_num);
		uk_pr_info("%s inode %u has more than 1 link (%u)\n",
			   fname, ino, nlink);
		elm = cpio_ilist_elm_upto(ilist, ino);
		/*
		 * If the list already contains an element with this inode
		 * just link this current entry to it.
		 */
		if (elm && elm->ino == ino) {
			uk_pr_info("Inode %u has already been linked by %s, creating new hardlink at %s\n",
				   ino, elm->path, fullpath);
			rc = uk_syscall_do_link(elm->path, fullpath);
			if (unlikely(rc)) {
				uk_pr_err("Failed to create new hard link %s (from %s).\n",
					  fullpath, elm->path);
				return -UKCPIO_LINK_FAILED;
			}

			*headerp = UKCPIO_NEXT(header, namesize, filesize);
			return UKCPIO_SUCCESS;
		}

		/*
		 * There is no element with current entry's inode and found
		 * element's inode is smaller so create the new inode list
		 * element to insert.
		 */
		new_elm = cpio_ilist_elm_create(ilist, ino, fullpath,
						prefixlen + namesize);
		if (unlikely(!new_elm)) {
			uk_pr_err("Failed to create new ilist elm.\n");
			return -UKCPIO_NOMEM;
		}

		/*
		 * If the list is empty or the new inode is the new smallest
		 * inode, insert right after the head.
		 * Otherwise, insert right after the element already in the
		 * list whose inode is smaller to keep the list sorted
		 * ascendingly.
		 */
		if (!elm)
			UK_SLIST_INSERT_HEAD(&ilist->elms, new_elm, next);
		else
			UK_SLIST_INSERT_AFTER(elm, new_elm, next);
	}

	err = UKCPIO_SUCCESS;

	if (UKCPIO_IS_DIR(mode))
		err = extract_dir(fullpath, mode & 0777);
	else if (UKCPIO_IS_FILE(mode))
		err = extract_file(fullpath, data, filesize,
				   mode & 0777, mtime);
	else if (UKCPIO_IS_SYMLINK(mode))
		err = extract_symlink(fullpath, data, filesize);
	else
		uk_pr_warn("File %s unknown mode %o\n", fullpath, mode);

	*headerp = UKCPIO_NEXT(header, namesize, filesize);
	return err;
}

/**
 * Extracts a CPIO section to the dest directory.
 *
 * @param headerp
 *  Pointer to the CPIO header, on success gets advanced to next section.
 * @param fullpath
 *  Buffer containing the destination path to extract the current header to.
 * @param eof
 *  Pointer to the first byte after end of file.
 * @param prefixlen
 *  Length of the destination path already present in fullpath.
 * @return
 *  Returns 0 on success or one of ukcpio_error enum.
 */
static enum ukcpio_error
process_section(struct cpio_ilist *ilist,
		const struct uk_cpio_header **headerp, char *fullpath,
		const char *eof, size_t prefixlen)
{
	const struct uk_cpio_header *header = *headerp;

	if (unlikely((char *)header >= eof ||
		     UKCPIO_FILENAME(header) > eof)) {
		uk_pr_err("Truncated CPIO header at %p", header);
		return -UKCPIO_INVALID_HEADER;
	}
	if (unlikely(!ukcpio_valid_magic(header))) {
		uk_pr_err("Bad magic number in CPIO header at %p\n", header);
		return -UKCPIO_INVALID_HEADER;
	}
	if (unlikely(UKCPIO_ISLAST(header))) {
		*headerp = NULL;
		return UKCPIO_SUCCESS;
	}
	return extract_section(ilist, headerp, fullpath, eof, prefixlen);
}

enum ukcpio_error
ukcpio_extract(const char *dest, const void *buf, size_t buflen)
{
	struct cpio_ilist ilist = {
		.elms = UK_SLIST_HEAD_INITIALIZER(&ilist.elms),
	};
	enum ukcpio_error error = UKCPIO_SUCCESS;
	const struct uk_cpio_header *header = buf;
	size_t max_alloc, destlen;
	char pathbuf[PATH_MAX];
	struct uk_alloc *a;
	void *region_base;

	if (dest == NULL)
		return -UKCPIO_NODEST;

	destlen = strlcpy(pathbuf, dest, PATH_MAX);
	if (unlikely(destlen > PATH_MAX - 1))
		return -UKCPIO_NODEST;
	if (pathbuf[destlen - 1] != '/') {
		pathbuf[destlen++] = '/';
		pathbuf[destlen] = 0;
	}

	a = uk_alloc_get_default();

	/*
	 * Since we cannot know how many CPIO entries with nlink > 1 are in
	 * a CPIO archive, just initialize the region allocator based on the
	 * minimum between the biggest possible contiguous allocation of the
	 * current default allocator and the maximum possible count of CPIO
	 * entries that could fit in the given buffer. In the case of the
	 * latter, if the CPIO archive is very small, the division might
	 * actually result in a number that is not even enough for the
	 * allocator's metadata, so add PAGE_SIZE as a safety measure,
	 * just in case.
	 *
	 * TODO: Find a better estimate than PAGE_SIZE, possibly a way for
	 * allocators to give information about minimum required usable
	 * memory for initialization.
	 */
	max_alloc = MIN((size_t)uk_alloc_maxalloc(a),
			PAGE_SIZE +
			buflen / sizeof(struct uk_cpio_header) *
			sizeof(struct cpio_ilist_elm));
	region_base = uk_malloc(a, max_alloc);
	if (unlikely(!region_base)) {
		uk_pr_err("Failed to allocate CPIO inode list.\n");
		return -UKCPIO_NOMEM;
	}

	ilist.ar = uk_allocregion_init(region_base, max_alloc);
	if (unlikely(!ilist.ar)) {
		uk_pr_err("Failed to initialize region allocator.\n");
		uk_free(a, region_base);
		return -UKCPIO_NOMEM;
	}

	while (header && error == UKCPIO_SUCCESS) {
		error = process_section(&ilist,
					&header, pathbuf,
					(char *)header + buflen, destlen);
	}

	uk_free(a, region_base);

	return error;
}
