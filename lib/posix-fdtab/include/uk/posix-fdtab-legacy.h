/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Legacy file interface into posix-fdtab for use by vfscore */

#ifndef __UK_POSIX_FDTAB_LEGACY_H__
#define __UK_POSIX_FDTAB_LEGACY_H__

#include <vfscore/file.h>

int uk_fdtab_legacy_open(struct vfscore_file *vf);

struct vfscore_file *uk_fdtab_legacy_get(int fd);

#endif /* __UK_POSIX_FDTAB_LEGACY_H__ */
