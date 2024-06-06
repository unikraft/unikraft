/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Robert Hrusecky <roberth@cs.utexas.edu>
 *          Omar Jamil <omarj2898@gmail.com>
 *          Sachin Beldona <sachinbeldona@utexas.edu>
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
#ifndef __UK_CPIO_H__
#define __UK_CPIO_H__

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern C {
#endif /* __cplusplus */

/*
 * Currently only supports BSD new-style cpio archive format.
 */
#define UKCPIO_MAGIC_NEWC "070701"
#define UKCPIO_MAGIC_CRC  "070702"
#define UKCPIO_FILE_TYPE_MASK    0170000
#define UKCPIO_DIRECTORY_BITS    040000
#define UKCPIO_FILE_BITS         0100000
#define UKCPIO_SYMLINK_BITS      0120000

#define UKCPIO_IS_FILE_OF_TYPE(mode, bits) \
	(((mode) & (UKCPIO_FILE_TYPE_MASK)) == (bits))
#define UKCPIO_IS_FILE(mode) UKCPIO_IS_FILE_OF_TYPE((mode), (UKCPIO_FILE_BITS))
#define UKCPIO_IS_DIR(mode) \
	UKCPIO_IS_FILE_OF_TYPE((mode), (UKCPIO_DIRECTORY_BITS))
#define UKCPIO_IS_SYMLINK(mode) \
	UKCPIO_IS_FILE_OF_TYPE((mode), (UKCPIO_SYMLINK_BITS))

struct uk_cpio_header {
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

static inline
int ukcpio_valid_magic(const struct uk_cpio_header *header)
{
	return memcmp(header->magic, UKCPIO_MAGIC_NEWC, 6) == 0 ||
	       memcmp(header->magic, UKCPIO_MAGIC_CRC, 6) == 0;
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
static inline
unsigned int snhex_to_int(const char *buf, size_t count)
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

#define UKCPIO_ALIGN 4
#define UKCPIO_ALIGN_UP(ptr) ((void *)ALIGN_UP((uintptr_t)(ptr), UKCPIO_ALIGN))

#define UKCPIO_U32FIELD(buf) \
	((uint32_t)snhex_to_int((buf), 8))
#define UKCPIO_FILENAME(header) \
	((char *)(header) + sizeof(struct uk_cpio_header))
#define UKCPIO_DATA(header, fnlen) \
	((char *)UKCPIO_ALIGN_UP((char *)(header) + \
				 sizeof(struct uk_cpio_header) + (fnlen)))
#define UKCPIO_NEXT(header, fnlen, fsize) \
	((struct uk_cpio_header *)UKCPIO_ALIGN_UP(UKCPIO_DATA(header, fnlen) + \
						  (fsize)))
#define UKCPIO_ISLAST(header) \
	(strcmp(UKCPIO_FILENAME(header), "TRAILER!!!") == 0)

/**
 * Include also the case of unsupported headers
 */
enum ukcpio_error {
	UKCPIO_SUCCESS = 0,
	UKCPIO_INVALID_HEADER,
	UKCPIO_FILE_CREATE_FAILED,
	UKCPIO_FILE_WRITE_FAILED,
	UKCPIO_FILE_CHMOD_FAILED,
	UKCPIO_FILE_CLOSE_FAILED,
	UKCPIO_MKDIR_FAILED,
	UKCPIO_SYMLINK_FAILED,
	UKCPIO_MALFORMED_INPUT,
	UKCPIO_NODEST
};

/**
 * Extracts the given CPIO buffer to the path destination.
 *
 * @param dest
 *  The path location where the buffer will be extracted to.
 * @param buf
 *  A pointer to the first header of the CPIO buffer.
 * @param buflen
 *  The size of the CPIO buffer.
 * @return
 *  Returns 0 on success or one of ukcpio_error enums.
 */
enum ukcpio_error
ukcpio_extract(const char *dest, const void *buf, size_t buflen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CPIO_H__ */
