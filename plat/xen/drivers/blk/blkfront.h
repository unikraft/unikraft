/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest.
 * All rights reserved.
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
#ifndef __BLKFRONT_H__
#define __BLKFRONT_H__

/**
 * Unikraft Blockfront interface.
 *
 * This header contains all the information needed by the block device driver
 * implementation.
 */
#include <uk/blkdev.h>
#if CONFIG_XEN_BLKFRONT_GREFPOOL
#include <uk/list.h>
#include <uk/semaphore.h>
#include <stdbool.h>
#endif

#include <xen/io/blkif.h>
#include <common/gnttab.h>
#include <common/events.h>

#define BLK_RING_PAGES_NUM 1

#if CONFIG_XEN_BLKFRONT_GREFPOOL
/**
 * Structure used to describe a list of blkfront_gref elements.
 */
UK_STAILQ_HEAD(blkfront_gref_list, struct blkfront_gref);

/*
 * Structure used to describe a pool of grant refs for each queue.
 * It contains max BLKIF_MAX_SEGMENTS_PER_REQUEST elems.
 **/
struct blkfront_grefs_pool {
	/* List of grefs. */
	struct blkfront_gref_list grefs_list;
	/* Semaphore for synchronization. */
	struct uk_semaphore sem;
};
#endif

/**
 * Structure used to describe a grant ref element.
 */
struct blkfront_gref {
	/* Grant ref number. */
	grant_ref_t ref;
#if CONFIG_XEN_BLKFRONT_GREFPOOL
	/* Entry for pool. */
	UK_STAILQ_ENTRY(struct blkfront_gref) _list;
	/* It is True if it was pulled from the pool.
	 * Otherwise this structure was allocated during the request.
	 **/
	bool reusable_gref;
#endif
};

/**
 * Structure used to describe a front device request.
 */
struct blkfront_request {
	/* Request from the API. */
	struct uk_blkreq *req;
	/* List with maximum number of blkfront_grefs for a request. */
	struct blkfront_gref *gref[BLKIF_MAX_SEGMENTS_PER_REQUEST];
	/* Number of segments. */
	uint16_t nb_segments;
	/* Queue in which the request will be stored */
	struct uk_blkdev_queue *queue;
};

/*
 * Structure used to describe a queue used for both requests and responses
 */
struct uk_blkdev_queue {
	/* Front_ring structure */
	struct blkif_front_ring ring;
	/* Grant ref pointing at the front ring. */
	grant_ref_t ring_ref;
	/* Event channel for the front ring. */
	evtchn_port_t evtchn;
	/* Allocator for this queue. */
	struct uk_alloc *a;
	/* The libukblkdev queue identifier */
	uint16_t queue_id;
	/* The flag to interrupt on the queue */
	int intr_enabled;
	/* Reference to the Blkfront Device */
	struct blkfront_dev *dev;
#if CONFIG_XEN_BLKFRONT_GREFPOOL
	/* Grant refs pool. */
	struct blkfront_grefs_pool ref_pool;
#endif
};

/**
 * Structure used to describe the Blkfront device.
 */
struct blkfront_dev {
	/* Xenbus Device. */
	struct xenbus_device *xendev;
	/* Blkdev Device. */
	struct uk_blkdev blkdev;
	/* Blkfront device number from Xenstore path. */
	blkif_vdev_t	handle;
	/* Value which indicates that the backend can process requests with the
	 * BLKIF_OP_WRITE_BARRIER request opcode.
	 */
	int barrier;
	/* Value which indicates that the backend can process requests with the
	 * BLKIF_OP_WRITE_FLUSH_DISKCACHE request opcode.
	 */
	int flush;
	/* Number of configured queues used for requests */
	uint16_t nb_queues;
	/* Vector of queues used for communication with backend */
	struct uk_blkdev_queue *queues;
	/* The blkdev identifier */
	__u16 uid;
};

#endif /* __BLKFRONT_H__ */
