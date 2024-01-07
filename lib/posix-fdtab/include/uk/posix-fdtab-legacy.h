/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Legacy file interface into posix-fdtab for use by vfscore */

#ifndef __UK_POSIX_FDTAB_LEGACY_H__
#define __UK_POSIX_FDTAB_LEGACY_H__

#include <uk/config.h>

#if CONFIG_LIBVFSCORE

#include <vfscore/file.h>

/**
 * Opens a vfscore_file structure and assigns it a new file descriptor number.
 *
 * @param vf
 *   Pointer to vfscore_file structure that contains information about the
 *   file, such as its file descriptor
 * @return
 *   = 0, success
 *   < 0, failure
 */
int uk_fdtab_legacy_open(struct vfscore_file *vf);

/**
 * Returns the vfscore_file structure with the given file descriptor number.
 *
 * @param fd
 *   File descriptor number for which to search
 * @return
 *   Pointer to vfscore_file structure that has that file descriptor number
 */
struct vfscore_file *uk_fdtab_legacy_get(int fd);

#endif /* CONFIG_LIBVFSCORE */
#endif /* __UK_POSIX_FDTAB_LEGACY_H__ */
