/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Support for POSIX pipes */

#ifndef __UKPOSIX_PIPE_H__
#define __UKPOSIX_PIPE_H__

#include <stdbool.h>
#include <uk/file.h>

/* File creation */
int uk_pipefile_create(struct uk_file *pipes[2]);

/* Optional ownership carried on the same queue record as socket data. */
struct uk_pipe_control {
	void (*release)(struct uk_pipe_control *control);
	struct uk_pipe_control *next;
};

/* Message pipes are used exclusively with the following operations.
 * A send consumes control when it queues a packet or any stream bytes.
 * A receive transfers control and returns the original length of a packet.
 */
int uk_pipefile_create_msg(struct uk_file *pipes[2]);
ssize_t uk_pipefile_send_msg(const struct uk_file *f,
			     const struct iovec *iov, size_t iovcnt,
			     bool packet, struct uk_pipe_control *control);
ssize_t uk_pipefile_recv_msg(const struct uk_file *f,
			     const struct iovec *iov, size_t iovcnt,
			     bool packet, struct uk_pipe_control **control);

/* Internal syscalls */
#if CONFIG_LIBPOSIX_FDTAB
int uk_sys_pipe(int pipefd[2], int flags);
#endif /* CONFIG_LIBPOSIX_FDTAB */

#endif /* __UKPOSIX_PIPE_H__ */
