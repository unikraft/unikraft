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
#include <inttypes.h>
#include <limits.h>

#include <uk/assert.h>
#include <uk/print.h>
#include <uk/cpio.h>
#include <uk/essentials.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Currently only supports BSD new-style cpio archive format.
 */
#define UKCPIO_MAGIC_NEWC "070701"
#define UKCPIO_MAGIC_CRC  "070702"
#define FILE_TYPE_MASK    0170000
#define DIRECTORY_BITS    040000
#define FILE_BITS         0100000
#define SYMLINK_BITS      0120000

#define ALIGN_4(ptr)      ((void *)ALIGN_UP((uintptr_t)(ptr), 4))

#define IS_FILE_OF_TYPE(mode, bits) (((mode) & (FILE_TYPE_MASK)) == (bits))
#define IS_FILE(mode) IS_FILE_OF_TYPE((mode), (FILE_BITS))
#define IS_DIR(mode) IS_FILE_OF_TYPE((mode), (DIRECTORY_BITS))
#define IS_SYMLINK(mode) IS_FILE_OF_TYPE((mode), (SYMLINK_BITS))

#define S8HEX_TO_U32(buf) ((uint32_t) snhex_to_int((buf), 8))
#define GET_MODE(hdr)     ((mode_t) S8HEX_TO_U32((hdr)->mode))

#define filename(header) ((const char *)header + sizeof(struct cpio_header))

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
valid_magic(struct cpio_header *header)
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
int uk_syscall_r_mkdir(const char *, mode_t);
int uk_syscall_r_symlink(const char *, const char *);

/**
 * Reads the section to the dest from a given a CPIO header.
 *
 * @param header_ptr
 *  Pointer to the CPIO header.
 * @param dest
 *  Destination path to extract the current header.
 * @param last
 *  The size of the CPIO section.
 * @return
 *  Returns 0 on success or one of ukcpio_error enum.
 */
static enum ukcpio_error
read_section(struct cpio_header **header_ptr,
				const char *dest, uintptr_t last)
{
	enum ukcpio_error error = UKCPIO_SUCCESS;
	int fd;
	int err;
	struct cpio_header *header;
	char path_from_root[PATH_MAX];
	mode_t header_mode;
	uint32_t header_filesize;
	uint32_t header_namesize;
	char *data_location;
	uint32_t bytes_to_write;
	int bytes_written;
	struct cpio_header *next_header;

	if (strcmp(filename(*header_ptr), "TRAILER!!!") == 0) {
		*header_ptr = NULL;
		return UKCPIO_SUCCESS;
	}

	if (!valid_magic(*header_ptr)) {
		uk_pr_err("Unsupported or invalid magic number in CPIO header at %p\n",
			  *header_ptr);
		*header_ptr = NULL;
		return -UKCPIO_INVALID_HEADER;
	}

	UK_ASSERT(dest);

	header = *header_ptr;

	if (absolute_path(path_from_root, dest, filename(header))) {
		uk_pr_err("Resulting path too long: %s\n", filename(header));
		*header_ptr = NULL;
		error = -UKCPIO_MALFORMED_INPUT;
		goto out;
	}

	header_mode = GET_MODE(header);
	header_filesize = S8HEX_TO_U32(header->filesize);
	header_namesize = S8HEX_TO_U32(header->namesize);

	if ((uintptr_t)header + sizeof(struct cpio_header) > last) {
		*header_ptr = NULL;
		error = -UKCPIO_MALFORMED_INPUT;
		goto out;
	}

	if (IS_FILE(header_mode)) {
		uk_pr_info("Extracting %s (%"PRIu32" bytes)\n",
			   path_from_root, header_filesize);
		fd = uk_syscall_r_open(path_from_root, O_CREAT|O_RDWR, 0);

		if (fd < 0) {
			uk_pr_err("%s: Failed to create file\n",
				  path_from_root);
			*header_ptr = NULL;
			error = -UKCPIO_FILE_CREATE_FAILED;
			goto out;
		}

		data_location = (char *)ALIGN_4(
				(char *)(header) + sizeof(struct cpio_header)
				+ header_namesize);

		if ((uintptr_t)data_location + header_filesize > last) {
			uk_pr_err("%s: File exceeds archive bounds\n",
				  path_from_root);
			*header_ptr = NULL;
			error = -UKCPIO_MALFORMED_INPUT;
			goto out;
		}

		bytes_to_write = header_filesize;
		bytes_written = 0;

		while (bytes_to_write > 0) {
			bytes_written = uk_syscall_r_write(fd,
				data_location + bytes_written,
				bytes_to_write);
			if (bytes_written < 0) {
				uk_pr_err("%s: Failed to load content: %s (%d)\n",
					  path_from_root,
					  strerror(-bytes_written),
					  -bytes_written);
				*header_ptr = NULL;
				error = -UKCPIO_FILE_WRITE_FAILED;
				goto out;
			}
			bytes_to_write -= bytes_written;
		}

		if ((err = uk_syscall_r_chmod(path_from_root,
					      header_mode & 0777)))
			uk_pr_warn("%s: Failed to chmod: %s (%d)\n",
				   path_from_root, strerror(-err), -err);

		if ((err = uk_syscall_r_close(fd))) {
			uk_pr_err("%s: Failed to close file: %s (%d)\n",
				  path_from_root, strerror(-err), -err);
			*header_ptr = NULL;
			error = -UKCPIO_FILE_CLOSE_FAILED;
			goto out;
		}
	} else if (IS_DIR(header_mode)) {
		uk_pr_info("Creating directory %s\n", path_from_root);
		if (strcmp(".", filename(header)) != 0 &&
		    (err = uk_syscall_r_mkdir(path_from_root,
		                              header_mode & 0777)))
		{
			uk_pr_err("%s: Failed to create directory: %s (%d)\n",
				  path_from_root, strerror(-err), -err);
			*header_ptr = NULL;
			error = -UKCPIO_MKDIR_FAILED;
			goto out;
		}
	} else if (IS_SYMLINK(header_mode)) {
		uk_pr_info("Creating symlink %s\n", path_from_root);

		data_location = (char *)ALIGN_4(
				(char *)(header) + sizeof(struct cpio_header)
				+ header_namesize);

		if ((uintptr_t)data_location + header_filesize > last) {
			uk_pr_err("%s: File exceeds archive bounds\n",
				  path_from_root);
			*header_ptr = NULL;
			error = -UKCPIO_MALFORMED_INPUT;
			goto out;
		}

		char target[header_filesize + 1];
		memcpy(target, data_location, header_filesize);
		target[header_filesize] = 0;

		uk_pr_info("%s: Target is %s\n", path_from_root, target);
		if ((err = uk_syscall_r_symlink(target, path_from_root))) {
			uk_pr_err("%s: Failed to create symlink: %s (%d)\n",
				  path_from_root, strerror(-err), -err);
			*header_ptr = NULL;
			error = -UKCPIO_SYMLINK_FAILED;
			goto out;
		}
	} else {
		uk_pr_warn("File %s unknown mode %o\n",
			   path_from_root, header_mode);
	}

	next_header = (struct cpio_header *)ALIGN_4(
		(char *)header + sizeof(struct cpio_header) + header_namesize
	);

	next_header = (struct cpio_header *)ALIGN_4(
		(char *)next_header + header_filesize
	);

	*header_ptr = next_header;

out:
	return error;
}

enum ukcpio_error
ukcpio_extract(const char *dest, void *buf, size_t buflen)
{
	enum ukcpio_error error = UKCPIO_SUCCESS;
	struct cpio_header *header = (struct cpio_header *)(buf);
	struct cpio_header **header_ptr = &header;
	uintptr_t end = (uintptr_t)header;

	if (dest == NULL)
		return -UKCPIO_NODEST;

	while ((error == UKCPIO_SUCCESS) && header) {
		error = read_section(header_ptr, dest, end + buflen);
		header = *header_ptr;
	}

	return error;
}
