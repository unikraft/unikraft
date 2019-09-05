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

#ifndef __UK_9PDEV_CORE__
#define __UK_9PDEV_CORE__

#include <string.h>
#include <inttypes.h>
#include <uk/config.h>
#include <uk/arch/spinlock.h>
#include <uk/bitmap.h>
#include <uk/list.h>
#include <uk/9p_core.h>
#if CONFIG_LIBUKSCHED
#include <uk/wait_types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct uk_9pdev;
struct uk_9preq;

/**
 * Function type used for connecting to a device on a certain transport.
 * The implementation should also set the msize field in the 9P device
 * struct to the maximum allowed message size.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param device_identifier
 *   The identifier of the underlying device (mount_tag for virtio, etc.)
 * @param mount_args
 *   Arguments received by the mount() call, for transport-specific options.
 * @return
 *   - (-EBUSY): Device is already in-use.
 *   - (-ENOENT): Device does not exist.
 *   - (0): Successful.
 *   - (< 0): Failed with a transport layer dependent error.
 */
typedef int (*uk_9pdev_connect_t)(struct uk_9pdev *dev,
				const char *device_identifier,
				const char *mount_args);

/**
 * Function type used for disconnecting from the device.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @return
 *   - (0): Successful.
 *   - (< 0): Failed with a transport layer dependent error.
 */
typedef int (*uk_9pdev_disconnect_t)(struct uk_9pdev *dev);

/**
 * Function type used for sending a request to the 9P device.
 *
 * @param dev
 *   The Unikraft 9P device.
 * @param req
 *   Reference to the request to be sent.
 * @return
 *   - (0): Successful.
 *   - (< 0): Failed. If -ENOSPC, then the transport layer does not have enough
 *   space to send this request and retries are required.
 */
typedef int (*uk_9pdev_request_t)(struct uk_9pdev *dev,
				struct uk_9preq *req);

/**
 * A structure used to store the operations supported by a certain transport.
 */
struct uk_9pdev_trans_ops {
	uk_9pdev_connect_t                      connect;
	uk_9pdev_disconnect_t                   disconnect;
	uk_9pdev_request_t                      request;
};

/**
 * @internal
 * A structure used for 9p requests' management.
 */
struct uk_9pdev_req_mgmt {
	/* Spinlock protecting this data. */
	spinlock_t                      spinlock;
	/* Bitmap of available tags. */
	unsigned long                   tag_bm[UK_BITS_TO_LONGS(UK_9P_NUMTAGS)];
	/* List of requests allocated and not yet removed. */
	struct uk_list_head             req_list;
};

/**
 * @internal
 * A structure used to describe the availability of 9P fids.
 */
struct uk_9pdev_fid_mgmt {
	/* Spinlock protecting fids. */
	spinlock_t			spinlock;
	/* Next available fid. */
	uint32_t			next_fid;
	/* Free-list of fids that can be reused. */
	struct uk_list_head		fid_free_list;
	/*
	 * List of fids that are currently active, to be clunked at the end of
	 * a 9pfs session.
	 */
	struct uk_list_head		fid_active_list;
};

/**
 * @internal
 * 9PDEV transport state
 *
 * - CONNECTED: Default state after initialization and during normal operation.
 * - DISCONNECTING: After a uk_9pdev_disconnect() call.
 *   No requests are allowed anymore. When all live resources have been
 *   destroyed, the 9pdev will free itself.
 */
enum uk_9pdev_trans_state {
	UK_9PDEV_CONNECTED,
	UK_9PDEV_DISCONNECTING
};

/**
 * 9PDEV
 * A structure used to interact with a 9P device.
 */
struct uk_9pdev {
	/* Underlying transport operations. */
	const struct uk_9pdev_trans_ops *ops;
	/* Allocator used by this device. */
	struct uk_alloc                 *a; /* See uk_9pdev_connect(). */
	/* Transport state. */
	enum uk_9pdev_trans_state       state;
	/* Maximum size of a message. */
	uint32_t                        msize;
	/* Maximum size of a message for the transport. */
	uint32_t                        max_msize;
	/* Transport-allocated data. */
	void                            *priv;
	/* @internal Fid management. */
	struct uk_9pdev_fid_mgmt	_fid_mgmt;
	/* @internal Request management. */
	struct uk_9pdev_req_mgmt        _req_mgmt;
#if CONFIG_LIBUKSCHED
	/*
	 * Slept on by threads waiting for their turn for enough space to send
	 * the request.
	 */
	struct uk_waitq                 xmit_wq;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* __UK_9PDEV_CORE__ */
