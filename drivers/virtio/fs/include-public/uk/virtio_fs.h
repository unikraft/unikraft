/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_VIRTIO_FS_H__
#define __UK_VIRTIO_FS_H__

#include <uk/arch/types.h>

struct uk_virtiofs_iovec {
	void *iov_base;
	__sz iov_len;
};

struct uk_virtiofs_iovecs {
	const struct uk_virtiofs_iovec *iov;
	__sz iovcnt;
};

struct uk_virtiofs_dev;

/* Find device by tag */

struct uk_virtiofs_dev *uk_virtiofs_dev_lookup(const char *tag);

/* Config (unconditionally resets device; caller responsible for lifetime) */

int uk_virtiofs_dev_configure(struct uk_virtiofs_dev *dev);

void uk_virtiofs_dev_shutdown(struct uk_virtiofs_dev *dev);

/* Perform synchronous requests; block until reply is received */

__ssz uk_virtiofs_request(struct uk_virtiofs_dev *dev,
			  const struct uk_virtiofs_iovec *iov,
			  __sz in_iovcnt, __sz out_iovcnt);

__ssz uk_virtiofs_request_vec(struct uk_virtiofs_dev *dev,
			      const struct uk_virtiofs_iovecs *iovecs,
			      __sz num_iovec, __sz in_iovcnt, __sz out_iovcnt);

/* TODO: export request queueing to enable async I/O */

#endif /* __UK_VIRTIO_FS_H__ */
