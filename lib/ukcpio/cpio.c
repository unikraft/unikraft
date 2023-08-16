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

#include <uk/assert.h>
#include <uk/print.h>
#include <uk/cpio.h>
#include <uk/essentials.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>

/*
 * Currently only supports BSD new-style cpio archive format.
 */
#define UKCPIO_MAGIC_NEWC "070701"
#define UKCPIO_MAGIC_CRC  "070702"
#define FILE_TYPE_MASK    0170000
#define DIRECTORY_BITS    040000
#define FILE_BITS         0100000
#define SYMLINK_BITS      0120000

#define IS_FILE_OF_TYPE(mode, bits) (((mode) & (FILE_TYPE_MASK)) == (bits))
#define IS_FILE(mode) IS_FILE_OF_TYPE((mode), (FILE_BITS))
#define IS_DIR(mode) IS_FILE_OF_TYPE((mode), (DIRECTORY_BITS))
#define IS_SYMLINK(mode) IS_FILE_OF_TYPE((mode), (SYMLINK_BITS))

struct cpio_header {
	char magic[6];
	char inode_num[8];
	char mode[8];
	char uid[8];
	char gid[8];
	char nlink[8];
	char mtime[8];
	char filesize[8];
	char major[8];
	char minor[8];
	char ref_major[8];
	char ref_minor[8];
	char namesize[8];
	char chksum[8];
};

static int
valid_magic(const struct cpio_header *header)
{
	return memcmp(header->magic, UKCPIO_MAGIC_NEWC, 6) == 0
		|| memcmp(header->magic, UKCPIO_MAGIC_CRC, 6) == 0;
}

/**
 * Function to convert len digits of hexadecimal string loc
 * to an integer.
 *
 * @param buf
 *  The string character buffer.
 * @param count
 *  The size of the buffer.
 * @return
 *   The converted unsigned integer value on success.  Returns 0 on error.
 */
static unsigned int
snhex_to_int(const char *buf, size_t count)
{
	unsigned int val = 0;
	size_t i;

	UK_ASSERT(buf);

	for (i = 0; i < count; i++) {
		val *= 16;
		if (buf[i] >= '0' && buf[i] <= '9')
			val += (buf[i] - '0');
		else if (buf[i] >= 'A' && buf[i] <= 'F')
			val += (buf[i] - 'A') + 10;
		else if (buf[i] >= 'a' && buf[i] <= 'f')
			val += (buf[i] - 'a') + 10;
		else
			return 0;
	}
	return val;
}

#define ALIGN_4(ptr)      ((void *)ALIGN_UP((uintptr_t)(ptr), 4))

#define CPIO_U32FIELD(buf) \
	((uint32_t) snhex_to_int((buf), 8))
#define CPIO_FILENAME(header) \
	((char *)header + sizeof(struct cpio_header))
#define CPIO_DATA(header, fnlen) \
	(char *)ALIGN_4((char *)(header) + sizeof(struct cpio_header) + fnlen)
#define CPIO_NEXT(header, fnlen, fsize) \
	(struct cpio_header *)ALIGN_4(CPIO_DATA(header, fnlen) + fsize)
#define CPIO_ISLAST(header) \
	(strcmp(CPIO_FILENAME(header), "TRAILER!!!") == 0)

/**
 * Create absolute path with prefix.
 *
 * @param prefix
 *  Prefix to the path.
 * @param path
 *  Path to file or directory.
 * @return
 *  Returns the absolute path with prefix.
 */
static int
absolute_path(char *abs_path, const char *prefix, const char *path)
{
	int add_slash;
	size_t prefix_len;
	size_t path_len;
	size_t abs_path_len;

	UK_ASSERT(prefix);
	UK_ASSERT(path);

	prefix_len = strlen(prefix);
	path_len = strlen(path);

	add_slash = prefix[prefix_len - 1] == '/' ? 0 : 1;
	abs_path_len = prefix_len + add_slash + path_len + 1;

	if (abs_path_len > PATH_MAX)
		return -1;

	memcpy(abs_path, prefix, prefix_len);
	if (add_slash)
		abs_path[prefix_len] = '/';
	memcpy(&abs_path[prefix_len + add_slash], path, path_len);

	abs_path[abs_path_len - 1] = '\0';
	return 0;
}

/* Raw filesystem syscalls; not provided by headers */
int uk_syscall_r_open(const char *, int, mode_t);
int uk_syscall_r_close(int);
ssize_t uk_syscall_r_write(int, const void *, size_t);
int uk_syscall_r_chmod(const char *, mode_t);
int uk_syscall_r_utime(const char *, const struct utimbuf *);
int uk_syscall_r_mkdir(const char *, mode_t);
int uk_syscall_r_symlink(const char *, const char *);

static enum ukcpio_error
extract_file(const char *path, const char *contents, size_t len,
             mode_t mode, uint32_t mtime)
{
	int ret = UKCPIO_SUCCESS;
	int fd;
	int err;
	struct utimbuf times = {mtime, mtime};

	uk_pr_info("Extracting %s (%zu bytes)\n", path, len);

	fd = uk_syscall_r_open(path, O_CREAT|O_RDWR, 0);
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
	if ((r = uk_syscall_r_mkdir(path, mode))) {
		uk_pr_err("%s: Failed to create directory: %s (%d)\n",
		          path, strerror(-r), -r);
		return -UKCPIO_MKDIR_FAILED;
	}
	return UKCPIO_SUCCESS;
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
 * @param dest
 *  Destination path to extract the current header to.
 * @param eof
 *  Pointer to the first byte after end of file.
 * @return
 *  Returns 0 on success or one of ukcpio_error enum.
 */
static enum ukcpio_error
process_section(const struct cpio_header **headerp,
                const char *dest, const char *eof)
{
	const struct cpio_header *header = *headerp;

	if ((char *)header >= eof || CPIO_FILENAME(header) > eof) {
		uk_pr_err("Truncated CPIO header at %p", header);
		return -UKCPIO_INVALID_HEADER;
	}
	if (!valid_magic(header)) {
		uk_pr_err("Bad magic number in CPIO header at %p\n", header);
		return -UKCPIO_INVALID_HEADER;
	}

	if (CPIO_ISLAST(header)) {
		*headerp = NULL;
		return UKCPIO_SUCCESS;
	} else {
		uint32_t mode = CPIO_U32FIELD(header->mode);
		uint32_t filesize = CPIO_U32FIELD(header->filesize);
		uint32_t namesize = CPIO_U32FIELD(header->namesize);
		uint32_t mtime = CPIO_U32FIELD(header->mtime);
		const char *fname = CPIO_FILENAME(header);
		const char *data = CPIO_DATA(header, namesize);

		if (fname + namesize > eof) {
			uk_pr_err("File name exceeds archive bounds at %p\n",
			          header);
			return -UKCPIO_MALFORMED_INPUT;
		}
		if (data + filesize > eof) {
			uk_pr_err("File exceeds archive bounds: %s\n", fname);
			return -UKCPIO_MALFORMED_INPUT;
		}

		UK_ASSERT(dest);
		char fullpath[PATH_MAX];

		if (absolute_path(fullpath, dest, fname)) {
			uk_pr_err("Resulting path too long: %s\n", fname);
			return -UKCPIO_MALFORMED_INPUT;
		}

		enum ukcpio_error err = UKCPIO_SUCCESS;

		/* Skip "." as dest is already there */
		if (IS_DIR(mode) && strcmp(".", fname)) {
			err = extract_dir(fullpath, mode & 0777);
		} else if (IS_FILE(mode)) {
			err = extract_file(fullpath, data, filesize,
			                   mode & 0777, mtime);
		} else if (IS_SYMLINK(mode)) {
			err = extract_symlink(fullpath, data, filesize);
		} else {
			uk_pr_warn("File %s unknown mode %o\n", fullpath, mode);
		}

		*headerp = CPIO_NEXT(header, namesize, filesize);
		return err;
	}
}

enum ukcpio_error
ukcpio_extract(const char *dest, void *buf, size_t buflen)
{
	enum ukcpio_error error = UKCPIO_SUCCESS;
	const struct cpio_header *header = (struct cpio_header *)(buf);

	if (dest == NULL)
		return -UKCPIO_NODEST;

	while (header && error == UKCPIO_SUCCESS)
		error = process_section(&header, dest, (char *)header + buflen);

	return error;
}
