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
 */

#ifndef __FDTAB_FD_H__
#define __FDTAB_FD_H__

#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <uk/list.h>
#include <uk/mutex.h>
#include <uk/fdtab/uio.h>
#include <uk/fdtab/eventpoll.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fdtab_file;
struct fdtab_table;

#define POSIX_FDTAB_REGISTER_PRIO 0

/*
 * Kernel encoding of open mode; separate read and write bits that are
 * independently testable: 1 greater than the above.
 */
#define UK_FREAD           0x00000001
#define UK_FWRITE          0x00000002

static inline int fdtab_fflags(int oflags)
{
	int rw = oflags & O_ACCMODE;

	oflags &= ~O_ACCMODE;
	return (rw + 1) | oflags;
}

static inline int fdtab_oflags(int fflags)
{
	int rw = fflags & (UK_FREAD|UK_FWRITE);

	fflags &= ~(UK_FREAD|UK_FWRITE);
	return (rw - 1) | fflags;
}

typedef	int (*fdop_free_t)	(struct fdtab_file *);
typedef	int (*fdop_read_t)	(struct fdtab_file *, struct uio *, int);
typedef	int (*fdop_write_t)	(struct fdtab_file *, struct uio *, int);
typedef	int (*fdop_seek_t)	(struct fdtab_file *, off_t, int, off_t*);
typedef	int (*fdop_ioctl_t)	(struct fdtab_file *, unsigned long, void *);
typedef	int (*fdop_fsync_t)	(struct fdtab_file *);
typedef	int (*fdop_fstat_t)	(struct fdtab_file *, struct stat *);
typedef	int (*fdop_truncate_t)	(struct fdtab_file *, off_t);
typedef int (*fdop_fallocate_t) (struct fdtab_file *, int, off_t, off_t);
typedef int (*fdop_poll_t)	(struct fdtab_file *, unsigned int *,
				 struct eventpoll_cb *);

/*
 * fd operations
 */
struct fdops {
	fdop_free_t		fdop_free;
	fdop_read_t		fdop_read;
	fdop_write_t		fdop_write;
	fdop_poll_t		fdop_poll;

	/* Optional operations */
	fdop_ioctl_t		fdop_ioctl;
	fdop_seek_t		fdop_seek;
	fdop_fsync_t		fdop_fsync;
	fdop_fstat_t		fdop_fstat;
	fdop_truncate_t		fdop_truncate;
	fdop_fallocate_t	fdop_fallocate;
};

/*
 * fd interface
 */
#define FDOP_FREE(FP)			((FP)->f_op->fdop_free)(FP)
#define FDOP_READ(FP, U, F)		((FP)->f_op->fdop_read)(FP, U, F)
#define FDOP_WRITE(FP, U, F)		((FP)->f_op->fdop_write)(FP, U, F)
#define FDOP_SEEK(FP, OFF, TY, O)	((FP)->f_op->fdop_seek)(FP, OFF, TY, O)
#define FDOP_IOCTL(FP, C, A)		((FP)->f_op->fdop_ioctl)(FP, C, A)
#define FDOP_FSYNC(FP)			((FP)->f_op->fdop_fsync)(FP)
#define FDOP_FSTAT(FP, S)		((FP)->f_op->fdop_fstat(FP, S))
#define FDOP_TRUNCATE(FP, N)		((FP)->f_op->fdop_truncate)(FP, N)
#define FDOP_FALLOCATE(FP, M, OFF, LEN) ((FP)->f_op->fdop_fallocate)(FP, M, \
								     OFF, LEN)
#define FDOP_POLL(FP, EP, ECP)		((FP)->f_op->fdop_poll)(FP, EP, ECP)

struct fdtab_file {
	int fd;
	int		f_count;	/* reference count */
	int		f_flags;	/* open flags */
	struct uk_mutex f_lock;
	struct fdops   *f_op;

	struct uk_list_head f_ep;	/* List of eventpoll_fd's */
};

#define FD_LOCK(fp)       uk_mutex_lock(&((fp)->f_lock))
#define FD_UNLOCK(fp)     uk_mutex_unlock(&((fp)->f_lock))

struct fdtab_table *fdtab_get_active(void);
void fdtab_set_active(struct fdtab_table *tab);

int fdtab_alloc_fd(struct fdtab_table *tab);

int fdtab_reserve_fd(struct fdtab_table *tab, int fd);
int fdtab_put_fd(struct fdtab_table *tab, int fd);
int fdtab_install_fd(struct fdtab_table *tab, int fd, struct fdtab_file *file);
struct fdtab_file *fdtab_get_file(struct fdtab_table *tab, int fd);
void fdtab_put_file(struct fdtab_file *file);

/*
 * File descriptors reference count
 */
void fdtab_fhold(struct fdtab_file *fp);
int fdtab_fdrop(struct fdtab_file *fp);

void fdtab_file_init(struct fdtab_file *fp);

int fdtab_fget(int fd, struct fdtab_file **out_fp);
int fdtab_fdalloc(struct fdtab_file *fp, int *newfd);

#define FOF_OFFSET  0x0800    /* Use the offset in uio argument */

/* Also used from posix-sysinfo to determine sysconf(_SC_OPEN_MAX). */
#define FDTABLE_MAX_FILES CONFIG_LIBPOSIX_FDTAB_MAX_FILES

#ifdef __cplusplus
}
#endif

#endif /* __FDTAB_FD_H__ */
