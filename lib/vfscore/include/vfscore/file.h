/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __VFSCORE_FILE_H__
#define __VFSCORE_FILE_H__

#include <stdint.h>
#include <sys/types.h>
#include <vfscore/dentry.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vfscore_file;

/* Set this flag if vfs should NOt handle POSition for this file. The
 * file is not seek-able, updating f_offset does not make sense for
 * it */
#define UK_VFSCORE_NOPOS ((int) (1 << 0))

struct vfscore_file {
	int fd;
	int		f_flags;	/* open flags */
	int		f_count;	/* reference count */
	off_t		f_offset;	/* current position in file */
	void		*f_data;        /* file descriptor specific data */
	int		f_vfs_flags;    /* internal implementation flags */
	struct dentry   *f_dentry;
	struct uk_mutex f_lock;
};

#define FD_LOCK(fp)       uk_mutex_lock(&(fp->f_lock))
#define FD_UNLOCK(fp)     uk_mutex_unlock(&(fp->f_lock))

int vfscore_alloc_fd(void);
int vfscore_reserve_fd(int fd);
int vfscore_put_fd(int fd);
int vfscore_install_fd(int fd, struct vfscore_file *file);
struct vfscore_file *vfscore_get_file(int fd);
void vfscore_put_file(struct vfscore_file *file);

/*
 * File descriptors reference count
 */
void fhold(struct vfscore_file* fp);
int fdrop(struct vfscore_file* fp);

#define FOF_OFFSET  0x0800    /* Use the offset in uio argument */

/* Also used from posix-sysinfo to determine sysconf(_SC_OPEN_MAX). */
#define FDTABLE_MAX_FILES 1024

#ifdef __cplusplus
}
#endif

#endif /* __VFSCORE_FILE_H__ */
