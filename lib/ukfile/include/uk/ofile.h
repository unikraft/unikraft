/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Open file description */

#ifndef __UKFILE_OFILE_H__
#define __UKFILE_OFILE_H__

#include <uk/essentials.h>
#include <uk/file.h>
#include <uk/mutex.h>

struct uk_ofile {
	const struct uk_file *file;
	unsigned int mode;
	__atomic refcnt;
	off_t pos;
	struct uk_mutex lock; /* Lock for modifying open file state */
};

static inline
void uk_ofile_init(struct uk_ofile *of)
{
	uk_refcount_init(&of->refcnt, 0);
	uk_mutex_init(&of->lock);
}

#endif /* __UKFILE_OFILE_H__ */
