/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_POSIX_FDIO_IMPL_H__
#define __UK_POSIX_FDIO_IMPL_H__

#include <fcntl.h>

#include <uk/posix-fd.h>
#include <uk/mutex.h>

/* Open file helpers */

/* Stable, will not ever change on an open file */
#define _CAN_READ(m)    (!((m) & O_WRONLY))
#define _CAN_WRITE(m)  (!!((m) & (O_WRONLY|O_RDWR)))
#define _IS_SEEKABLE(m) (!((m) & UKFD_O_NOSEEK))
#define _IS_SYNC(m)    (!!((m) & O_SYNC))
#define _IS_DSYNC(m)   (!!((m) & O_DSYNC))
#define _SHOULD_LOCK(m) (!((m) & UKFD_O_NOIOLOCK))
/* Volatile, may change at any time */
#define _IS_BLOCKING(m) (!((m) & O_NONBLOCK))
#define _IS_APPEND(m)  (!!((m) & O_APPEND))

#define _of_lock(of) uk_mutex_lock(&(of)->lock)
#define _of_unlock(of) uk_mutex_unlock(&(of)->lock)

#endif /* __UK_POSIX_FDIO_IMPL_H__ */
