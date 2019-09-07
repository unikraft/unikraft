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

#ifndef __9PFRONT_H__
#define __9PFRONT_H__

#include <string.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/plat/spinlock.h>
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#endif
#include <xen/io/9pfs.h>
#include <common/events.h>
#include <common/gnttab.h>

struct p9front_dev_ring {
	/* Backpointer to the p9front device. */
	struct p9front_dev *dev;
	/* The 9pfs data interface, as dedfined by the xen headers. */
	struct xen_9pfs_data_intf *intf;
	/* The 9pfs data, as defined by the xen headers. */
	struct xen_9pfs_data data;
	/* The event channel for this ring. */
	evtchn_port_t evtchn;
	/* Grant reference for the interface. */
	grant_ref_t ref;
	/* Per-ring spinlock. */
	spinlock_t spinlock;
	/* Tracks if this ring was initialized. */
	bool initialized;
#if CONFIG_LIBUKSCHED
	/* Tracks if there is any data available on this ring. */
	bool data_avail;
	/* Bottom-half thread. */
	struct uk_thread *bh_thread;
	/* Bottom-half thread name. */
	char *bh_thread_name;
	/* Wait-queue on which the thread waits for available data. */
	struct uk_waitq bh_wq;
#endif
};

struct p9front_dev {
	/* Xenbus device. */
	struct xenbus_device *xendev;
	/* 9P API device. */
	struct uk_9pdev *p9dev;
	/* Entry within the 9pfront device list. */
	struct uk_list_head _list;
	/* Number of maximum rings, read from xenstore. */
	int nb_max_rings;
	/* Maximum ring page order, read from xenstore. */
	int max_ring_page_order;
	/* Mount tag for this device, read from xenstore. */
	char *tag;
	/* Number of rings to use. */
	int nb_rings;
	/* Ring page order. */
	int ring_order;
	/* Device data rings. */
	struct p9front_dev_ring *rings;
};

#endif /* __9PFRONT_H__ */
