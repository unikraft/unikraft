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
 * Opens a legacy vfscore_file structure and assigns it a new file descriptor.
 *
 * @param vf
 *   Pointer to the vfscore_file to be opened; its fd field will be filled in
 * @return
 *   = 0, success
 *   < 0, failure
 */
int uk_fdtab_legacy_open(struct vfscore_file *vf);

/**
 * Looks up a file descriptor and return the associated legacy vfscore_file.
 *
 * If the file descriptor maps to a different file type it is reported as free.
 *
 * @param fd
 *   File descriptor to look up
 * @return
 *   Pointer to the vfscore_file associated with fd
 */
struct vfscore_file *uk_fdtab_legacy_get(int fd);

#endif /* CONFIG_LIBVFSCORE */
#endif /* __UK_POSIX_FDTAB_LEGACY_H__ */
