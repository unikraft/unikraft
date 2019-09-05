/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UK_9PFID__
#define __UK_9PFID__

#include <stdbool.h>
#include <inttypes.h>
#include <uk/config.h>
#include <uk/9p_core.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/refcount.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure describing a managed fid via reference counting.
 */
struct uk_9pfid {
	/* Fid number. */
	uint32_t                fid;
	/* Associated server qid. */
	struct uk_9p_qid        qid;
	/* I/O unit. */
	uint32_t                iounit;
	/*
	 * If removed, no clunk is necessary, as the remove operation
	 * implicitly clunks the fid.
	 */
	bool was_removed;
	/* Tracks the number of live references. */
	__atomic                refcount;
	/* @internal Associated 9P device. */
	struct uk_9pdev         *_dev;
	/*
	 * @internal
	 * List on which this fid currently is. See uk_9pdev_fid_mgmt for
	 * details.
	 */
	struct uk_list_head     _list;
};

/**
 * @internal
 * Allocates a 9p fid.
 * Should not be used directly, use uk_9pdev_fid_create() instead.
 *
 * @param a
 *   Allocator to use.
 * @return
 *   - NULL: Out of memory.
 *   - (!=NULL): Successful.
 */
struct uk_9pfid *uk_9pfid_alloc(struct uk_9pdev *dev);

/**
 * Gets the 9p fid, incrementing the reference count.
 *
 * @param fid
 *   Reference to the 9p fid.
 */
void uk_9pfid_get(struct uk_9pfid *fid);

/**
 * Puts the 9p fid, decrementing the reference count.
 * If this was the last live reference, the memory will be freed.
 *
 * @param fid
 *   Reference to the 9p fid.
 * @return
 *   - 0: This was not the last live reference.
 *   - 1: This was the last live reference.
 */
int uk_9pfid_put(struct uk_9pfid *fid);

#ifdef __cplusplus
}
#endif

#endif /* __UK_9PFID__ */
