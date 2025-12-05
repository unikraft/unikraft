/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Virtiofs base data structures */

#ifndef __UK_VIRTIOFS_BASE_H__
#define __UK_VIRTIOFS_BASE_H__

#include <uk/atomic.h>
#include <uk/file.h>
#include <uk/mutex.h>
#include <uk/refcount.h>
#include <uk/tree.h>

#include "virtiofs-fuse.h"

/* TODO: make configurable / compile-time constant */
#define virtiofs_alloc uk_alloc_get_default()

#define VIRTIOFS_NODE_OPEN	1

struct viofs_dev;

struct viofs_node {
	uint64_t inode;
	uint64_t fh;
	UK_RB_ENTRY(viofs_node) rb_entry; /* Link in inode mapping */
	struct viofs_dev *dev;
	struct viofs_node *parent;
	/* Internal state */
	__atomic refcnt;
	unsigned int flags;
	struct uk_file_state fstate;
	struct uk_mutex openlk;
	/* attr cache */
	struct fuse_attr attr;
};

static inline
void virtiofs_init_node(struct viofs_node *n, struct viofs_dev *dev,
			uint64_t inode, struct viofs_node *parent)
{
	n->inode = inode;
	n->dev = dev;
	n->parent = parent;
	n->flags = 0;
	n->fstate = UK_FILE_STATE_INIT_VALUE(n->fstate);
	uk_mutex_init(&n->openlk);
	uk_refcount_init(&n->refcnt, 1);
}

static
uint64_t virtiofs_rb_node_key(struct viofs_node *n)
{
	return n->inode;
}

static
int virtiofs_rb_node_cmp(uint64_t a, uint64_t b)
{
	return a < b ? -1 : a == b ? 0 : 1;
}

UK_RB_HEAD(virtiofs_nodemap, viofs_node);

struct viofs_dev {
	struct virtiofs_fuse_dev fdev;
	/* FS-driver private bookkeeping */
	UK_RB_ENTRY(viofs_dev) rb_entry; /* Link in opendevs */
	/* inode mapping */
	struct virtiofs_nodemap nodemap;
	struct uk_mutex maplk;
	/* Root node */
	struct viofs_node rootnode;
};

static inline
void virtiofs_init_dev(struct viofs_dev *d, struct uk_virtiofs_dev *vd)
{
	d->fdev.dev = vd;
	d->fdev.unique = 0;
	UK_RB_INIT(&d->nodemap);
	uk_mutex_init(&d->maplk);
}

static
struct uk_virtiofs_dev *virtiofs_rb_dev_key(struct viofs_dev *d)
{
	return d->fdev.dev;
}

static
int virtiofs_rb_dev_cmp(struct uk_virtiofs_dev *a, struct uk_virtiofs_dev *b)
{
	return a < b ? -1 : a == b ? 0 : 1;
}

static inline
ssize_t virtiofs_file_read(struct viofs_node *n, size_t off, size_t len,
			   const struct iovec *iov, size_t iovcnt, long flags)
{
	const struct fuse_read_in in = {
		.fh = n->fh,
		.offset = off,
		.size = len,
		.flags = flags
	};

	UK_ASSERT(n->flags & VIRTIOFS_NODE_OPEN);
	return virtiofs_fuse_read(&n->dev->fdev, n->inode, &in, iov, iovcnt);
}

static inline
ssize_t virtiofs_file_write(struct viofs_node *n, size_t off, size_t len,
			    const struct iovec *iov, size_t iovcnt, long flags)
{
	const struct fuse_write_in in = {
		.fh = n->fh,
		.offset = off,
		.size = len,
		.flags = flags
	};

	UK_ASSERT(n->flags & VIRTIOFS_NODE_OPEN);
	return virtiofs_fuse_write(&n->dev->fdev, n->inode, &in, iov, iovcnt);
}

#endif /* __UK_VIRTIOFS_BASE_H__ */
