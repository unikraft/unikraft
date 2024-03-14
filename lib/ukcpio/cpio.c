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

#include <uk/assert.h>
#include <uk/print.h>
#include <uk/cpio.h>
#include <uk/essentials.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>


/* Raw filesystem syscalls; not provided by headers */
int uk_syscall_r_open(const char *, int, mode_t);
int uk_syscall_r_close(int);
ssize_t uk_syscall_r_write(int, const void *, size_t);
int uk_syscall_r_chmod(const char *, mode_t);
int uk_syscall_r_utime(const char *, const struct utimbuf *);
int uk_syscall_r_mkdir(const char *, mode_t);
int uk_syscall_r_symlink(const char *, const char *);
int uk_syscall_r_stat(const char *, struct stat *);

static enum ukcpio_error
extract_file(const char *path, const char *contents, size_t len,
	     mode_t mode, uint32_t mtime)
{
	int ret = UKCPIO_SUCCESS;
	int fd;
	int err;
	struct utimbuf times = {mtime, mtime};

	uk_pr_info("Extracting %s (%zu bytes)\n", path, len);

	fd = uk_syscall_r_open(path, O_CREAT | O_WRONLY | O_TRUNC, 0);
	if (fd < 0) {
		uk_pr_err("%s: Failed to create file: %s (%d)\n",
			  path, strerror(-fd), -fd);
		ret = -UKCPIO_FILE_CREATE_FAILED;
		goto out;
	}

	while (len) {
		ssize_t written = uk_syscall_r_write(fd, contents, len);

		if (written < 0) {
			err = written;
			uk_pr_err("%s: Failed to load content: %s (%d)\n",
				  path, strerror(-err), -err);
			ret = -UKCPIO_FILE_WRITE_FAILED;
			goto close_out;
		}
		UK_ASSERT(len >= (size_t)written);
		contents += written;
		len -= written;
	}

	if ((err = uk_syscall_r_chmod(path, mode)))
		uk_pr_warn("%s: Failed to chmod: %s (%d)\n",
			   path, strerror(-err), -err);

	if ((err = uk_syscall_r_utime(path, &times)))
		uk_pr_warn("%s: Failed to set modification time: %s (%d)",
			   path, strerror(-err), -err);

close_out:
	if ((err = uk_syscall_r_close(fd))) {
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
	r = uk_syscall_r_mkdir(path, mode);
	if (r == -EEXIST) {
		struct stat pstat;

		r = uk_syscall_r_stat(path, &pstat);
		if (r < 0) {
			r = -EEXIST;
			goto err_out;
		}
		if (!S_ISDIR(pstat.st_mode)) {
			r = -EEXIST;
			goto err_out;
		}

		/* Directory already exists, set mode only */
		r = uk_syscall_r_chmod(path, mode);
		if (r < 0) {
			uk_pr_warn("%s: Failed to chmod: %s (%d)\n",
				   path, strerror(-r), -r);
		}
	} else if (r < 0) {
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
	if ((r = uk_syscall_r_symlink(target, path))) {
		uk_pr_err("%s: Failed to create symlink: %s (%d)\n",
			  path, strerror(-r), -r);
		return -UKCPIO_SYMLINK_FAILED;
	}
	return UKCPIO_SUCCESS;
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
process_section(const struct uk_cpio_header **headerp, char *fullpath,
		const char *eof, size_t prefixlen)
{
	const struct uk_cpio_header *header = *headerp;

	if ((char *)header >= eof || UKCPIO_FILENAME(header) > eof) {
		uk_pr_err("Truncated CPIO header at %p", header);
		return -UKCPIO_INVALID_HEADER;
	}
	if (!ukcpio_valid_magic(header)) {
		uk_pr_err("Bad magic number in CPIO header at %p\n", header);
		return -UKCPIO_INVALID_HEADER;
	}

	if (UKCPIO_ISLAST(header)) {
		*headerp = NULL;
		return UKCPIO_SUCCESS;
	} else {
		uint32_t mode = UKCPIO_U32FIELD(header->mode);
		uint32_t filesize = UKCPIO_U32FIELD(header->filesize);
		uint32_t namesize = UKCPIO_U32FIELD(header->namesize);
		uint32_t mtime = UKCPIO_U32FIELD(header->mtime);
		const char *fname = UKCPIO_FILENAME(header);
		const char *data = UKCPIO_DATA(header, namesize);
		enum ukcpio_error err;

		if (fname + namesize > eof) {
			uk_pr_err("File name exceeds archive bounds at %p\n",
				  header);
			return -UKCPIO_MALFORMED_INPUT;
		}
		if (data + filesize > eof) {
			uk_pr_err("File exceeds archive bounds: %s\n", fname);
			return -UKCPIO_MALFORMED_INPUT;
		}

		/* namesize includes trailing NUL */
		if (prefixlen + namesize > PATH_MAX) {
			uk_pr_err("Resulting path too long: %s\n", fname);
			return -UKCPIO_MALFORMED_INPUT;
		}
		memcpy(fullpath + prefixlen, fname, namesize);

		err = UKCPIO_SUCCESS;

		/* Skip "." as dest is already there */
		if (UKCPIO_IS_DIR(mode) && strcmp(".", fname)) {
			err = extract_dir(fullpath, mode & 0777);
		} else if (UKCPIO_IS_FILE(mode)) {
			err = extract_file(fullpath, data, filesize,
					   mode & 0777, mtime);
		} else if (UKCPIO_IS_SYMLINK(mode)) {
			err = extract_symlink(fullpath, data, filesize);
		} else {
			uk_pr_warn("File %s unknown mode %o\n", fullpath, mode);
		}

		*headerp = UKCPIO_NEXT(header, namesize, filesize);
		return err;
	}
}

enum ukcpio_error
ukcpio_extract(const char *dest, const void *buf, size_t buflen)
{
	enum ukcpio_error error = UKCPIO_SUCCESS;
	const struct uk_cpio_header *header = buf;
	char pathbuf[PATH_MAX];
	size_t destlen;

	if (dest == NULL)
		return -UKCPIO_NODEST;

	if ((destlen = strlcpy(pathbuf, dest, PATH_MAX)) > PATH_MAX - 1)
		return -UKCPIO_NODEST;
	if (pathbuf[destlen-1] != '/') {
		pathbuf[destlen++] = '/';
		pathbuf[destlen] = 0;
	}

	while (header && error == UKCPIO_SUCCESS) {
		error = process_section(&header, pathbuf,
					(char *)header + buflen, destlen);
	}
	return error;
}
