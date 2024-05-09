/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* POSIX compatible file descriptor table */

#ifndef __UK_POSIX_FDTAB_H__
#define __UK_POSIX_FDTAB_H__

#include <uk/config.h>
#include <uk/ofile.h>

#define UK_LIBPOSIX_FDTAB_INIT_PRIO	UK_PRIO_EARLIEST
#define UK_FD_MAX INT_MAX

/**
 * Opens the file `f` with `mode` and associates it with a file descriptor.
 *
 * The lifetime of `f` must cover the entirety of this function call.
 *
 * @param f
 *   File to open
 * @param mode
 *   Mode flags to apply on the new open file description
 * @return
 *   The newly allocated file descriptor.
 */
int uk_fdtab_open(const struct uk_file *f, unsigned int mode);

/**
 * Gets the open file description associated with descriptor `fd`.
 *
 * Users should call uk_fdtab_ret when done with the open file reference.
 *
 * @param fd
 *   File descriptor to look up
 * @return
 *   Open file reference or NULL if `fd` is not an open file descriptor.
 */
struct uk_ofile *uk_fdtab_get(int fd);

/**
 * Returns a reference to an open file when done using it.
 *
 * @param of
 *   Reference to the open file description to be returned
 */
void uk_fdtab_ret(struct uk_ofile *of);

/**
 * Sets flags on file descriptor. Currently only supports O_CLOEXEC.
 *
 * @param fd
 *   File descriptor on which to set the flags
 * @param flags
 *   Flags to set on the file descriptor
 * @return
 *   0 if successful, < 0 if `fd` not mapped
 */
int uk_fdtab_setflags(int fd, int flags);

/**
 * Get the flags set on `fd`. Currently only supports O_CLOEXEC.
 *
 * @param fd
 *   File descriptor for which to get the flags
 * @return
 *   >= 0, flags set on `fd`
 *   < 0, if `fd` not mapped
 */
int uk_fdtab_getflags(int fd);

/**
 * Close all files that have the O_CLOEXEC flag set.
 *
 * Assumes no other threads are touching the active fdtab during the call.
 */
void uk_fdtab_cloexec(void);

#if CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM
/*
 * TODO: This shim interface exists to support cohabitation with vfscore until
 * its eventual removal. Unless you are implementing a vfscore shim interface,
 * and know what you are doing, please do not use this API.
 */
union uk_shim_file {
	struct uk_ofile *ofile;
#if CONFIG_LIBVFSCORE
	struct vfscore_file *vfile;
#endif /* CONFIG_LIBVFSCORE */
};

/**
 * Gets the open file description associated with descriptor `fd`, shimming
 * between vfscore_file and uk_ofile, writing the value in `*out`.
 *
 * @param fd
 *   File descriptor to look up
 * @param out
 *   Open file description, of type either uk_ofile or vfscore_file
 * @return
 *   UK_SHIM_OFILE, if successful and `*out` is a uk_ofile
 *   UK_SHIM_LEGACY, if successful and `*out` is a vfscore_file
 *   < 0, if `fd` is not mapped
 */
int uk_fdtab_shim_get(int fd, union uk_shim_file *out);

#define UK_SHIM_OFILE  0
#define UK_SHIM_LEGACY 1

#endif /* CONFIG_LIBPOSIX_FDTAB_LEGACY_SHIM */

/* Internal syscalls */

/**
 * Closes a file descriptor.
 *
 * @param fd
 *   File descriptor to be closed
 * @return
 *   = 0, success
 *   < 0, failure
 */
int uk_sys_close(int fd);

/**
 * Allocates a new file descriptor that refers to the same
 * open file description as the descriptor oldfd.
 *
 * @param oldfd
 *   File descriptor associated with the open file description to be duplicated
 * @return
 *   = 0, success
 *   < 0, failure
 */
int uk_sys_dup(int oldfd);

/**
 * Same as dup, but the caller can specify a minimum value for
 * the file descriptor to be assigned and the flags to be set on the new
 * file descriptor.
 *
 * @param oldfd
 *   File descriptor associated with the open file description to be duplicated
 * @param min
 *   Minimum new file descriptor number that can be assigned
 * @param flags
 *   File status flags to be added to the new file descriptor
 * @return
 *   = 0, success
 *   < 0, failure
 */
int uk_sys_dup_min(int oldfd, int min, int flags);

/**
 * Same as dup, but instead of using the lowest-numbered unused file descriptor,
 * it uses the file descriptor specified in newfd.
 *
 * If newfd is already open it is silently closed.
 * If newfd equals oldfd, this function does nothing.
 *
 * @param oldfd
 *   File descriptor associated with the open file description to be duplicated
 * @param newfd
 *   New file descriptor for the duplicated open file description
 * @return
 *   = 0, success
 *   < 0, failure
 */
int uk_sys_dup2(int oldfd, int newfd);

/**
 * Same as dup2, except that:
 * - The caller can force the close-on-exec flag to be set for the new file
 *   descriptor by specifying O_CLOEXEC in flags.
 * - If newfd equals oldfd, this function returns -EINVAL.
 *
 * @param oldfd
 *   File descriptor associated with the open file description to be duplicated
 * @param newfd
 *   New file descriptor for the duplicated open file description
 * @param flags
 *   Flags to set on newfd, currently only supports O_CLOEXEC
 * @return
 *   = 0, success
 *   < 0, failure
 */
int uk_sys_dup3(int oldfd, int newfd, int flags);

#endif /* __UK_POSIX_FDTAB_H__ */
