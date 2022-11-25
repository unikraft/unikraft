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

#ifndef FDTAB_SYSCALLS_H
#define FDTAB_SYSCALLS_H

#include <uk/fdtab/fd.h>
#include <sys/statfs.h>

/**
 * Reads data from the descriptor provided by the fp structure into
 * niov buffers of type iovec, starting at offset. The number of read bytes is
 * returned in count.
 *
 * @param fp
 *	Pointer to the fdtab_file structure
 * @param iov
 *	Array of buffers to store the data into
 * @param niov
 *	Number of elements in the iov array
 * @param offset
 *	Offset to start reading at
 * @param[out] count
 *	Number of read bytes
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_read(struct fdtab_file *fp, const struct iovec *iov, size_t niov,
	     off_t offset, size_t *count);

/**
 * Writes data from the array of buffers iov of size niov at the
 * file descriptor provided by the fp structure, starting at offset.
 * The number of written bytes is returned in count.
 *
 * @param fp
 *	Pointer to the fdtab_file structure
 * @param iov
 *	Array of buffers to write the data into
 * @param niov
 *	Number of elements in the iov array
 * @param offset
 *	Offset to start writing at
 * @param[out] count
 *	Number of bytes written
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_write(struct fdtab_file *fp, const struct iovec *iov, size_t niov,
	      off_t offset, size_t *count);

/**
 * Repositions read/write file cursor.
 *
 * @param fp
 *	Pointer to the fdtab_file structure
 * @param off
 *	Offset bytes to reposition the cursor, according to the type
 * @param type
 *	It can be one of the following types:
 *		* SEEK_SET: The file offset is set to offset bytes.
 *		* SEEK_CUR: The file offset is set to its current
 *			location plus offset bytes.
 *		* SEEK_END: The file offset is set to the size of
 *			the file plus offset bytes.
 * @param[out] cur_off
 *	The resulting offset measured from the start of the file
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_lseek(struct fdtab_file *fp, off_t off, int type,
	      off_t *cur_off);

/**
 * Modifies device parameters of special files.
 *
 * @param fp
 *	Pointer to the fdtab_file structure
 * @param request
 *	Device-dependent request code
 * @param buf
 *	Pointer to the data that should be used by call
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_ioctl(struct fdtab_file *fp, unsigned long request, void *buf);

/**
 * The sys_fstat() function is equivalent to sys_stat(),
 * except that it takes a fdtab_file structure as parameter.
 *
 * @param fp
 *	Pointer to the fdtab_file structure
 * @param[out] st
 *	Buffer to store information about the file
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fstat(struct fdtab_file *fp, struct stat *st);

/**
 * Synchronizes the in-core data referred by fdtab_file to the backing
 * store device.
 *
 * @param fp
 *	Pointer to the fdtab_file structure
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fsync(struct fdtab_file *fp);

/**
 * Similar to the sys_truncate() function, but it takes a fdtab_file
 * instead of path.
 *
 * @param fp
 *	Pointer to the fdtab_file
 * @param length
 *	The new size (in bytes) of the file to which fp is referring to
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_ftruncate(struct fdtab_file *fp, off_t length);

/**
 * Manipulates the file space for the file referred by fd, starting
 * at offset and continuing for len bytes.
 *
 * @param fp
 *	Pointer to a fdtab_file structure
 * @param mode
 *	Specifies how to manipulates the space. The mode can have the
 *	the following values:
 *		* 0 - allocates len bytes starting from offset.
 *		* FALLOC_FL_KEEP_SIZE - the file size will not change even
 *			if offset + len is greater than the file size.
 *		* FALLOC_FL_PUNCH_HOLE - deallocates len bytes starting at
 *			offset and creates a hole in the file. This flag should
 *			always come together with FALLOC_FL_KEEP_SIZE, otherwise
 *			the sys_futimens call will fail.
 * @param offset
 *	Starting byte from which this operation will apply
 * @param len
 *	Number of bytes for which this operation will apply
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fallocate(struct fdtab_file *fp, int mode, loff_t offset, loff_t len);

#endif /* FDTAB_SYSCALLS_H */
