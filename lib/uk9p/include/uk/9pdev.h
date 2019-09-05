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

#ifndef __UK_9PDEV__
#define __UK_9PDEV__

#include <stdbool.h>
#include <inttypes.h>
#include <uk/config.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/9pdev_core.h>
#include <uk/plat/irq.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_9pdev_trans;

/**
 * Connect to an underlying device, obtaining a 9pdev handle to the connection.
 *
 * @param trans
 *   The underlying transport.
 * @param device_identifier
 *   The identifier of the device (for virtio and xen, the mount tag).
 * @param mount_args
 *   Arguments passed down from the mount() call.
 * @param a
 *   (Optional) Allocator used for any allocations done by this 9pdev.
 *   If NULL, the transport-specific allocator will be used.
 * @return
 *   - Connection handle, if successful.
 *   - Error pointer, otherwise.
 */
struct uk_9pdev *uk_9pdev_connect(const struct uk_9pdev_trans *trans,
				const char *device_identifier,
				const char *mount_args,
				struct uk_alloc *a);

/**
 * Disconnect from the underlying device.
 *
 * Important: Even in case of failure, the device is closed and should not be
 * used after calling uk_9pdev_disconnect().
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @return
 *   - (0): Successful.
 *   - (< 0): Failure to disconnect.
 */
int uk_9pdev_disconnect(struct uk_9pdev *dev);

/**
 * Send a 9P request to the given 9P device.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param req
 *   The 9P request.
 * @return
 *   - (0): Successful.
 *   - (< 0): Failed.
 */
int uk_9pdev_request(struct uk_9pdev *dev, struct uk_9preq *req);

/**
 * Notify the 9P device that the device's transmit queue is not full and
 * it may attempt to send requests again.
 *
 * @param dev
 *   The Unikraft 9P Device.
 */
void uk_9pdev_xmit_notify(struct uk_9pdev *dev);

/**
 * Creates and sends 9P request to the given 9P device, serializing it with
 * the given arguments. This function acts as a shorthand for the explicit
 * calls to req_create(), serialize(), ready(), request(), waitreply().
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param type
 *   Transmit type of the request, e.g. Tversion, Tread, and so on.
 * @param size
 *   The maximum size for the receive and send buffers.
 * @param fmt
 *   The format of the data to be serialized, in the way uk_9preq_serialize()
 *   expects it.
 * @param ...
 *   The arguments to be serialized.
 * @return
 *   - (!PTRISERR): The 9p request in the UK_9PREQ_RECEIVED state.
 *   - PTRISERR: The error code with which any of the steps failed.
 */
struct uk_9preq *uk_9pdev_call(struct uk_9pdev *dev, uint8_t type,
			uint32_t size, const char *fmt, ...);

/**
 * Create a new request, automatically allocating its tag, based on its type.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param type
 *   Transmit type of the request, e.g. Tversion, Tread, and so on.
 * @param size
 *   The maximum size for the receive and send buffers.
 * @return
 *   If not an error pointer, the created request.
 *   Otherwise, the error in creating the request:
 *   - ENOMEM: No memory for the request or no available tags.
 */
struct uk_9preq *uk_9pdev_req_create(struct uk_9pdev *dev, uint8_t type,
				uint32_t size);

/**
 * Looks up a request based on the given tag. This is generally used by
 * transport layers on receiving a 9P message.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param tag
 *   The tag to look up.
 * @return
 *   - NULL: No request with the given tag was found.
 *   - (!=NULL): The request.
 */
struct uk_9preq *uk_9pdev_req_lookup(struct uk_9pdev *dev, uint16_t tag);

/**
 * Remove a request from the given 9p device. If the request is in-flight,
 * it will be freed when all the references to it are gone.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param req
 *   The request to be removed.
 * @return
 *   - 0: There are more active references.
 *   - 1: This was the last reference to the request.
 */
int uk_9pdev_req_remove(struct uk_9pdev *dev, struct uk_9preq *req);

/**
 * Creates a FID associated with the given 9P device.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @return
 *   If not an error pointer, the created fid.
 *   Otherwise, the error in creating the fid:
 *   - ENOMEM: No memory for the request or no available tags.
 */
struct uk_9pfid *uk_9pdev_fid_create(struct uk_9pdev *dev);

/**
 * @internal
 * Releases a FID when its reference count goes to 0.
 *
 * Should not be called directly, but rather via uk_9pfid_put().
 *
 * @param fid
 *   The FID to be released.
 */
void uk_9pdev_fid_release(struct uk_9pfid *fid);

/**
 * Sets the maximum allowed message size.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param msize
 *   Allowed maximum message size.
 * @return
 *   - true: Setting the msize succeeded.
 *   - false: Setting the msize failed, as the given msize is greater than the
 *     maximum allowed message size.
 */
bool uk_9pdev_set_msize(struct uk_9pdev *dev, uint32_t msize);

/**
 * Gets the maximum message size.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @return
 *   Maximum message size.
 */
uint32_t uk_9pdev_get_msize(struct uk_9pdev *dev);

#ifdef __cplusplus
}
#endif

#endif /* __UK_9PDEV__ */
