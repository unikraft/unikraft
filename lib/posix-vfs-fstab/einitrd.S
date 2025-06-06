/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>

.globl vfs_einitrd_start
.globl vfs_einitrd_end

.section .rodata

.balign 4096
vfs_einitrd_start:
.incbin CONFIG_LIBPOSIX_VFS_FSTAB_EINITRD_PATH
.balign 1
vfs_einitrd_end:
.byte 0
