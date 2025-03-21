/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* VFS root file.
 *
 * This is a static file that is meant to be mounted at the canonical vfs root.
 * It serves exactly 3 purposes:
 * - a read-only empty dir when nothing is mounted
 * - a mount point for any real root
 * - a lookup artifice ensuring '/' is always its own parent
 */

#ifndef __UK_POSIX_VFSROOT_H__
#define __UK_POSIX_VFSROOT_H__

#include <uk/file.h>
#include <uk/posix-fd.h>

/* Raw ukfile instance */
extern const struct uk_file uk_posix_vfsroot_file;

/* Open file description of the above, named "/" */
extern struct uk_ofile uk_posix_vfsroot_ofile;

#endif /* __UK_POSIX_VFSROOT_H__ */
