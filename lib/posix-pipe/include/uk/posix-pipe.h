/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Support for POSIX pipes */

#ifndef __UKPOSIX_PIPE_H__
#define __UKPOSIX_PIPE_H__

#include <uk/file.h>

/* File creation */
int uk_pipefile_create(struct uk_file *pipes[2]);

/* Packet operations use a pipe exclusively in packet mode. A receive returns
 * the packet length, even when the supplied buffers hold fewer bytes.
 */
ssize_t uk_pipefile_send_packet(const struct uk_file *f,
				const struct iovec *iov, size_t iovcnt);
ssize_t uk_pipefile_recv_packet(const struct uk_file *f,
				const struct iovec *iov, size_t iovcnt);

/* Internal syscalls */
#if CONFIG_LIBPOSIX_FDTAB
int uk_sys_pipe(int pipefd[2], int flags);
#endif /* CONFIG_LIBPOSIX_FDTAB */

#endif /* __UKPOSIX_PIPE_H__ */
