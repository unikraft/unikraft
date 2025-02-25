/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* System init/term priority conventions for filesystems */

#ifndef __UK_FS_PRIO_H__
#define __UK_FS_PRIO_H__

#include <uk/prio.h>

/* Primary filesystems have no fs dependencies and can initialize first */
#define UK_FS_PRIO_PRIMARY UK_PRIO_EARLIEST

/* Secondary fs that depend on primary during init run next */
#define UK_FS_PRIO_SECONDARY UK_PRIO_AFTER(UK_FS_PRIO_PRIMARY)

/* Priority where all filesystems are initialized and available */
#define UK_FS_PRIO_FSAVAIL UK_PRIO_AFTER(UK_FS_PRIO_SECONDARY)

#endif /* __UK_FS_PRIO_H__ */
