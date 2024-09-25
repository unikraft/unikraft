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
	size_t pos; /* Current file read/write offset position */
	struct uk_mutex lock; /* Lock for modifying open file state */
	char name[]; /* Name of open file description; driver dependent */
	/* Filesystem-backed files should be named after the path they were
	 * opened with. Pseudo-files should be named something descriptive.
	 */
};

/* Allocation size of a named uk_ofile with a name of length `namelen` */
#define UKFD_OFILE_SIZE(namelen) (sizeof(struct uk_ofile) + (namelen) + 1)

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

/**
 * Acquire a reference on open file description `of`.
 *
 * @param of Open file description
 */
static inline
void uk_ofile_acquire(struct uk_ofile *of)
{
	uk_refcount_acquire(&of->refcnt);
}

/**
 * Release a reference held on open file description `of`.
 *
 * Do not call directly unless you are prepared to handle cleanup after the last
 * reference is dropped. Instead use the release function provided by the lib
 * where you got the open file reference from.
 *
 * @param of Open file description
 *
 * @return
 *  == 0: There are remaining references held
 *  != 0: The last reference has just been released
 */
static inline
int uk_ofile_release(struct uk_ofile *of)
{
	return uk_refcount_release(&of->refcnt);
}


/* Mode bits from fcntl.h that open files are interested in */
#define UKFD_MODE_MASK \
	(O_WRONLY|O_RDWR|O_NONBLOCK|O_APPEND|O_DIRECT|O_SYNC|O_DSYNC)

/* Unikraft-specific mode bits, chosen to not overlap with any O_* flags */
/* Open file is not seekable (e.g. for pipes, sockets & FIFOs) */
#define UKFD_O_NOSEEK   010
/* File I/O should not use the file locks (e.g. if driver handles them) */
#define UKFD_O_NOIOLOCK 020

/* INTERNAL. Open file description has .name field allocated */
#define UKFD_O_NAMED 040

/**
 * Retrieve the name of open file description `of`, or `fallback` if anonymous.
 *
 * @param of Open file description
 * @param fallback Name to return if `of` is anonymous
 *
 * @return
 *  Name of `of` (NUL-terminated string) or `fallback` if `of` is anonymous
 */
static inline
const char *uk_ofile_name(const struct uk_ofile *of, const char *fallback)
{
	return (of->mode & UKFD_O_NAMED) ? of->name : fallback;
}

/* Event sets */
#define UKFD_POLL_ALWAYS (EPOLLERR|EPOLLHUP)
#define UKFD_POLLIN (EPOLLIN|EPOLLRDNORM|EPOLLRDBAND)
#define UKFD_POLLOUT (EPOLLOUT|EPOLLWRNORM|EPOLLWRBAND)

#endif /* __UK_POSIX_FD_H__ */
