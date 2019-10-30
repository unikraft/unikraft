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
#include <xen/io/blkif.h>
#include <common/gnttab.h>
#include <common/events.h>

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
	/* Reference to the Blkfront Device */
	struct blkfront_dev *dev;
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
	/* Number of configured queues used for requests */
	uint16_t nb_queues;
	/* Vector of queues used for communication with backend */
	struct uk_blkdev_queue *queues;
	/* The blkdev identifier */
	__u16 uid;
};

#endif /* __BLKFRONT_H__ */
