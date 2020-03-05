/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __PLAT_DRV_VIRTQUEUE_H__
#define __PLAT_DRV_VIRTQUEUE_H__

#include <uk/config.h>
#include <uk/list.h>
#include <uk/sglist.h>
#include <uk/arch/types.h>
#include <virtio/virtio_ring.h>
#include <virtio/virtio_config.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Type declarations
 */
struct virtqueue;
struct virtio_dev;
typedef int (*virtqueue_callback_t)(struct virtqueue *, void *priv);
typedef int (*virtqueue_notify_host_t)(struct virtio_dev *, __u16 queue_nr);

/**
 * Structure to describe the virtqueue.
 */
struct virtqueue {
	/* Reference the virtio_dev it belong to */
	struct virtio_dev *vdev;
	/* Virtqueue identifier */
	__u16 queue_id;
	/* Notify to the host */
	virtqueue_notify_host_t vq_notify_host;
	/* Callback from the virtqueue */
	virtqueue_callback_t vq_callback;
	/* Next entry of the queue */
	UK_TAILQ_ENTRY(struct virtqueue) next;
	/* Private data structure used by the driver of the queue */
	void *priv;
};

/**
 * Fetch the physical address of the descriptor ring.
 * @param vq
 *	Reference to the virtqueue.
 *
 * @return
 *	Return the guest physical address of the vring.
 */
__phys_addr virtqueue_physaddr(struct virtqueue *vq);

/**
 * Ring interrupt handler. This function is invoked from the interrupt handler
 * in the virtio device for interrupt specific to the ring.
 *
 * @param obj
 *	Reference to the virtqueue.
 *
 * @return int
 *	0, Interrupt was not for this virtqueue.
 *	1, Virtqueue has handled the interrupt.
 */
int virtqueue_ring_interrupt(void *obj);

/**
 * Negotiate with the virtqueue features.
 * @param feature_set
 *	The feature set the device request.
 *
 * @return __u64
 *	The negotiated feature set.
 */
__u64 virtqueue_feature_negotiate(__u64 feature_set);

/**
 * Check if host notification is enabled.
 *
 * @param vq
 *	Reference to the virtqueue.
 * @return
 *	Returns 1, host needs notification on new descriptors.
 *		0, otherwise.
 */
int virtqueue_notify_enabled(struct virtqueue *vq);

/**
 * Remove the user buffer from the virtqueue.
 *
 * @param vq
 *	Reference to the virtqueue.
 * @param
 *      Reference to a reference that will point to the cookie that was
 *      submitted with the dequeued descriptor after successful exit of this
 *      function.
 * @param len
 *	Reference to the length of the data packet.
 * @return
 *	>= 0 A buffer was dequeued from the ring and the count indicates
 *	the number of used slots in the ring after dequeueing.
 *	< 0 Failed to dequeue a buffer, the output parameters cookie and len
 *      are unmodified.
 */
int virtqueue_buffer_dequeue(struct virtqueue *vq, void **cookie, __u32 *len);

/**
 * Create a descriptor chain starting at index head,
 * using vq->bufs also starting at index head.
 * @param vq
 *	Reference to the virtual queue
 * @param cookie
 *	Reference to the cookie to reconstruct the buffer.
 * @param sg
 *	Reference to the scatter gather list
 * @param read_bufs
 *	The number of read buffer.
 * @param write_bufs
 *	The number of write buffer.
 *
 * @return
 *	> 0 The buffers were enqueued into the ring and the count indicates
 *	the number of available slots in the ring.
 *	0  The buffers were enqueued and there are no further descriptors
 *	available.
 *	< 0 Failed to enqueue the buffers.
 *
 */
int virtqueue_buffer_enqueue(struct virtqueue *vq, void *cookie,
			     struct uk_sglist *sg, __u16 read_bufs,
			     __u16 write_bufs);

/**
 * Allocate a virtqueue.
 * @param queue_id
 *	The virtqueue hw id.
 * @param nr_descs
 *	The number of descriptor for the queue.
 * @param align
 *	The memory alignment for the ring memory.
 * @param callback
 *	A reference to callback to the virtio-dev.
 * @param notify
 *	A reference to notification function to the host.
 * @param vdev:
 *	A reference to the virtio device.
 * @param  a:
 *	A reference to the allocator.
 *
 * @return struct virtqueue *
 *	On success, return a reference to the virtqueue.
 *	On failure,
 *		   -ENOMEM: Failed to allocate the queue.
 */
struct virtqueue *virtqueue_create(__u16 queue_id, __u16 nr_descs, __u16 align,
				   virtqueue_callback_t callback,
				   virtqueue_notify_host_t notify,
				   struct virtio_dev *vdev, struct uk_alloc *a);

/**
 * Check the virtqueue if full.
 * @param vq
 *	A reference to the virtqueue.
 * @return int
 *	1 on full,
 *	0 otherwise
 */
int virtqueue_is_full(struct virtqueue *vq);

/**
 * Check the virtqueue if has any pending responses.
 * @param vq
 *	A reference to the virtqueue.
 * @return int
 *	1 on true,
 *	0 otherwise
 */
int virtqueue_hasdata(struct virtqueue *vq);

/*
 * Destroy a virtual queue
 * @param vq
 *	A reference to the virtual queue
 * @param a
 *	Reference to the memory allocator
 */
void virtqueue_destroy(struct virtqueue *vq, struct uk_alloc *a);

/**
 * Disable interrupts on the virtqueue.
 * @param vq
 *      Reference to the virtqueue.
 */
void virtqueue_intr_disable(struct virtqueue *vq);

/**
 * Enable interrupts on the virtqueue.
 * @param vq
 *      Reference to the virtqueue
 * @return
 *	0, On successful enabling of interrupt.
 *	1, More packet in the ring to be processed.
 */
int virtqueue_intr_enable(struct virtqueue *vq);

/**
 * Notify the host of an event.
 * @param vq
 *      Reference to the virtual queue.
 */
static inline void virtqueue_host_notify(struct virtqueue *vq)
{
	UK_ASSERT(vq);

	/*
	 * Before notifying the virtio backend on the host we should make sure
	 * that the virtqueue index update operation happened. Note that this
	 * function is declared as inline.
	 */
	mb();

	if (vq->vq_notify_host && virtqueue_notify_enabled(vq)) {
		uk_pr_debug("notify queue %d\n", vq->queue_id);
		vq->vq_notify_host(vq->vdev, vq->queue_id);
	}
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAT_DRV_VIRTQUEUE_H__ */
