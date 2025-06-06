/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Runtime user-supplied fstab; internal use only in fstab.h */

#ifndef __UK_POSIX_VFS_FSTAB_USER_H__
#define __UK_POSIX_VFS_FSTAB_USER_H__

#ifndef __UK_POSIX_VFS_FSTAB_H__
#error "Do not include this header directly"
#endif /* __UK_POSIX_VFS_FSTAB_H__ */

#if CONFIG_LIBPOSIX_VFS_FSTAB_USER

#include <uk/libparam.h>

static char *fstab_user[CONFIG_LIBPOSIX_VFS_FSTAB_USER_SIZE] = { NULL };

UK_LIBPARAM_PARAM_ARR_ALIAS(fstab, &fstab_user, charp,
			    CONFIG_LIBPOSIX_VFS_FSTAB_USER_SIZE,
			    "Filesystem table: dev:path:fs[:flags[:opts[:ukopts]]]");

/* Built-in fall-back file system table: `fstab_fallback[]` */
#include "fstab-fallback.h"

#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_USER */

#endif /* __UK_POSIX_VFS_FSTAB_USER_H__ */
