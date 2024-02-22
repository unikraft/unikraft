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

#include <uk/essentials.h>
#include <uk/file.h>
#include <uk/mutex.h>
#include <uk/refcount.h>

/* Open file description */
struct uk_ofile {
	const struct uk_file *file;
	unsigned int mode;
	__atomic refcnt;
	off_t pos; /* Current file read/write offset position */
	struct uk_mutex lock; /* Lock for modifying open file state */
};

/**
 * Initialize an open file description with a refcount of 1.
 *
 * @param of Open file description to be initialized
 * @param f File to reference
 * @param mode Mode bits of open file description
 */
static inline
void uk_ofile_init(struct uk_ofile *of,
		   const struct uk_file *f, unsigned int mode)
{
	uk_refcount_init(&of->refcnt, 1);
	uk_mutex_init(&of->lock);
	of->file = f;
	of->mode = mode;
	of->pos = 0;
}


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
