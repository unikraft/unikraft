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

/* Priority of fdtab in the inittab system. */
#define POSIX_FDTAB_REGISTER_PRIO 0

/* Also used from posix-sysinfo to determine sysconf(_SC_OPEN_MAX). */
#define FDTABLE_MAX_FILES CONFIG_LIBPOSIX_FDTAB_MAX_FILES

/*
 * Kernel encoding of open mode; separate read and write bits that are
 * independently testable: 1 greater than the above.
 */
#define UK_FREAD           0x00000001
#define UK_FWRITE          0x00000002

#define FOF_OFFSET  0x0800    /* Use the offset in uio argument */

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
	/*
	 * Mandatory operations
	 */
	fdop_free_t		fdop_free;
	fdop_read_t		fdop_read; /* required if f_flags & UK_FREAD  */
	fdop_write_t		fdop_write;/* required if f_flags & UK_FWRITE */

	/*
	 * Optional operations
	 */
	fdop_poll_t		fdop_poll;
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

/**
 * @brief A description of a file pointed to by file descriptors numbers.
 *
 * This structure is intended to be embedded into a structure by a client of the
 * posix-fdtab library. The client can then use this encapsulation structure to
 * store client-specific data. Note that the pointer to a fdtab_file has to be
 * stable and must not change
 */
struct fdtab_file {
	/* Pointer to operations table. Set by the client of posix-fdtab */
	struct fdops   *f_op;

	/* The file descriptor number that this was last registered with. Note,
	 * that a file can be associated with multiple file descriptor numbers.
	 * Therefore, this field is mostly useful for debugging.
	 */
	int fd;
	int		f_count;	/* reference count */
	int		f_flags;	/* open flags */
	struct uk_mutex f_lock;

	struct uk_list_head f_ep;	/* List of eventpoll_fd's */
};

#define FD_LOCK(fp)       uk_mutex_lock(&((fp)->f_lock))
#define FD_UNLOCK(fp)     uk_mutex_unlock(&((fp)->f_lock))

/**
 * @brief Get the active file descriptor table for the current thread.
 * @returns the active file descriptor table.
 */
struct fdtab_table *fdtab_get_active(void);

/**
 * @brief Switch to a different file descriptor table.
 * @param tab the fdtab_table to switch to.
 */
void fdtab_set_active(struct fdtab_table *tab);

/**
 * @brief Allocate a file descriptor number.
 * @param tab the fdtab_table to act on.
 *
 * @returns the allocated file descriptor number, or a negative value in case of
 *          an error.
 */
int fdtab_alloc_fd(struct fdtab_table *tab);

/**
 * @brief Try to reserve a specific file descriptor number.
 * @param tab the fdtab_table to act on.
 * @param fd the file descriptor number to reserve.
 *
 * @returns zero if the file descriptor number was successfully reserved, or a
 *          negative number if the number was already in use.
 */
int fdtab_reserve_fd(struct fdtab_table *tab, int fd);

/**
 * @brief Remove a file descriptor from the file descriptor table.
 *
 * This removes the file descriptor and decreases the reference count of the
 * referenced file structure.
 *
 * @param tab the fdtab_table to act on.
 * @param fd the file descriptor to remove.
 *
 * @returns zero if the file descriptor was successfully removed, or a negative
 *          value if an error occurred.
 */
int fdtab_put_fd(struct fdtab_table *tab, int fd);

/**
 * @brief Install a fdtab_file at a specific file descriptor number.
 *
 * If there is an file at the specified file descriptor number, then this file
 * will be removed first. The function will take the ownership of the passed
 * fdtab_file.
 *
 * @param tab the fdtab_table to act on.
 * @param fd the file descriptor to install the fdtab_file to.
 * @param file the fdtab_file to install.
 *
 * @returns zero if the file was successfully installed, or a negative value if
 *          an error occurred while doing so.
 */
int fdtab_install_fd(struct fdtab_table *tab, int fd, struct fdtab_file *file);

/**
 * @brief Get the file associated with the specfied file descriptor number.
 * @param tab the fdtab_table to act on.
 * @param fd the file descriptor number.
 *
 * @returns a pointer to the fdtab_file, a NULL pointer if there was no file
 *          associated. The reference count will have been incremented.
 */
struct fdtab_file *fdtab_get_file(struct fdtab_table *tab, int fd);

/**
 * @brief Decrement the reference count of the file.
 * @param file the file of which the reference count should be decremented.
 */
void fdtab_put_file(struct fdtab_file *file);

/*
 * File descriptors reference count
 */
/**
 * @brief Increment the reference count of a file by one.
 * @param fp the file to act on.
 */
void fdtab_fhold(struct fdtab_file *fp);

/**
 * @brief Decrement the reference count of a file.
 *
 * This function will free the file if the reference count reaches zero.
 *
 * @param fp the file to act on.
 *
 * @returns one if the reference count reached zero and the file was freed, and
 *          zero otherwise
 */
int fdtab_fdrop(struct fdtab_file *fp);

/**
 * @brief Initialize the specified file.
 * @param fp the file to initialize.
 */
void fdtab_file_init(struct fdtab_file *fp);

/**
 * @brief Wrapper around fdtab_get_file which returns a errno code.
 *
 * Compared to fdtab_get_file this function acts by default on the current file
 * descriptor table.
 *
 * @param fd the file descriptor number of the file to get.
 * @param[out] out_fp the file with file descriptor number fd, not touched in
 *                    case an error occurs.
 *
 * @returns zero if the operation was successful, a positive errno code
 *          otherwise.
 */
int fdtab_fget(int fd, struct fdtab_file **out_fp);

/**
 * @brief Register a file at a unused file descriptor number.
 *
 * The caller keeps a (shared) ownership of the referenced file description.
 *
 * @param fp the file to register.
 * @param newfd the file descriptor number at which the file was registered at.
 *
 * @return zero if the operation was successful, a positive errno code
 *         otherwise.
 */
int fdtab_fdalloc(struct fdtab_file *fp, int *newfd);

#ifdef __cplusplus
}
#endif

#endif /* __FDTAB_FD_H__ */
