/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Common definitions pertaining to POSIX open file descriptions */

#ifndef __UK_POSIX_FD_H__
#define __UK_POSIX_FD_H__

#include <fcntl.h>
#include <sys/epoll.h>

#include <uk/ofile.h>

/* Mode bits from fcntl.h that open files are interested in */
#define UKFD_MODE_MASK \
	(O_WRONLY|O_RDWR|O_NONBLOCK|O_APPEND|O_DIRECT|O_SYNC|O_DSYNC)

/* Unikraft-specific mode bits, chosen to not overlap with any O_* flags */
/* Open file is not seekable (e.g. for pipes, sockets & FIFOs) */
#define UKFD_O_NOSEEK   010
/* File I/O should not use the file locks (e.g. if driver handles them) */
#define UKFD_O_NOIOLOCK 020

/* Event sets */
#define UKFD_POLL_ALWAYS (EPOLLERR|EPOLLHUP)
#define UKFD_POLLIN (EPOLLIN|EPOLLRDNORM|EPOLLRDBAND)
#define UKFD_POLLOUT (EPOLLOUT|EPOLLWRNORM|EPOLLWRBAND)

#endif /* __UK_POSIX_FD_H__ */
