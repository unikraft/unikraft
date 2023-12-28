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
 * Open the file `f` with `mode` and associate it with a file descriptor.
 *
 * The lifetime of `f` must cover the entirety of this function call.
 *
 * @return
 *   The newly allocated file descriptor.
 */
int uk_fdtab_open(const struct uk_file *f, unsigned int mode);

/**
 * Get the open file description associated with descriptor `fd`.
 *
 * Users should call uk_fdtab_ret when done with the open file reference.
 *
 * @return
 *   Open file reference or NULL if `fd` is not an open file descriptor.
 */
struct uk_ofile *uk_fdtab_get(int fd);

/**
 * Return a reference to an open file when done using it.
 */
void uk_fdtab_ret(struct uk_ofile *of);

/**
 * Set flags on file descriptor. Currently only supports O_CLOEXEC.
 *
 * @return
 *   0 if successful, < 0 if `fd` not mapped
 */
int uk_fdtab_setflags(int fd, int flags);

/**
 * Get the flags set on `fd`. Currently only supports O_CLOEXEC.
 *
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
 * Get the open file description associated with descriptor `fd`, shimming
 * between vfscore_file and uk_ofile, writing the value in `*out`.
 *
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
int uk_sys_close(int fd);
int uk_sys_dup(int oldfd);
int uk_sys_dup_min(int oldfd, int min, int flags);
int uk_sys_dup2(int oldfd, int newfd);
int uk_sys_dup3(int oldfd, int newfd, int flags);

#endif /* __UK_POSIX_FDTAB_H__ */
