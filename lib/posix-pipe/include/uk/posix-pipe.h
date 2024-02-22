/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Support for POSIX pipes */

#ifndef __UKPOSIX_PIPE_H__
#define __UKPOSIX_PIPE_H__

#include <uk/file.h>
#include <uk/posix-fdtab.h>

/* File creation */

int uk_pipefile_create(struct uk_file *pipes[2]);

/* Internal syscalls */

int uk_sys_pipe(int pipefd[2], int flags);

#endif /* __UKPOSIX_PIPE_H__ */
