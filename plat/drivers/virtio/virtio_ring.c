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
/**
 * Inspired from the FreeBSD.
 * Commit-id: a89e7a10d501
 */
#include <uk/config.h>
#include <string.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/plat/common/cpu.h>
#include <uk/sglist.h>
#include <uk/arch/atomic.h>
#include <uk/plat/io.h>
#include <virtio/virtio_ring.h>
#include <virtio/virtqueue.h>

#define VIRTQUEUE_MAX_SIZE  32768
#define to_virtqueue_vring(vq)			\
	__containerof(vq, struct virtqueue_vring, vq)

struct virtqueue_desc_info {
	void *cookie;
	__u16 desc_count;
};

struct virtqueue_vring {
	struct virtqueue vq;
	/* Descriptor Ring */
	struct vring vring;
	/* Reference to the vring */
	void   *vring_mem;
	/* Keep track of available descriptors */
	__u16 desc_avail;
	/* Index of the next available slot */
	__u16 head_free_desc;
	/* Index of the last used descriptor by the host */
	__u16 last_used_desc_idx;
	/* Cookie to identify driver buffer */
	struct virtqueue_desc_info vq_info[];
};

/**
 * Static function Declaration(s).
 */
static inline void virtqueue_ring_update_avail(struct virtqueue_vring *vrq,
					       __u16 idx);
static inline void virtqueue_detach_desc(struct virtqueue_vring *vrq,
					 __u16 head_idx);
static inline int virtqueue_buffer_enqueue_segments(
						    struct virtqueue_vring *vrq,
						    __u16 head,
						    struct uk_sglist *sg,
						    __u16 read_bufs,
						    __u16 write_bufs);
static void virtqueue_vring_init(struct virtqueue_vring *vrq, __u16 nr_desc,
				 __u16 align);

/**
 * Driver implementation
 */
void virtqueue_intr_disable(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	vrq->vring.avail->flags |= (VRING_AVAIL_F_NO_INTERRUPT);
}

int virtqueue_intr_enable(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;
	int rc = 0;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	/* Check if there are no more packets enabled */
	if (!virtqueue_hasdata(vq)) {
		if (vrq->vring.avail->flags | VRING_AVAIL_F_NO_INTERRUPT) {
			vrq->vring.avail->flags &=
				(~VRING_AVAIL_F_NO_INTERRUPT);
			/**
			 * We enabled the interrupts. We ensure it using the
			 * memory barrier and check if there are any further
			 * data available in the queue. The check for data
			 * after enabling the interrupt is to make sure we do
			 * not miss any interrupt while transitioning to enable
			 * interrupt. This is inline with the requirement from
			 * virtio specification section 3.2.2
			 */
			mb();
			/* Check if there are further descriptors */
			if (virtqueue_hasdata(vq)) {
				virtqueue_intr_disable(vq);
				rc = 1;
			}
		}
	} else {
		/**
		 * There are more packet in the virtqueue to be processed while
		 * the interrupt was disabled.
		 */
		rc = 1;
	}
	return rc;
}

static inline void virtqueue_ring_update_avail(struct virtqueue_vring *vrq,
					__u16 idx)
{
	__u16 avail_idx;

	avail_idx = vrq->vring.avail->idx & (vrq->vring.num - 1);
	/* Adding the idx to available ring */
	vrq->vring.avail->ring[avail_idx] = idx;
	/**
	 * Write barrier to make sure we push the descriptor on the available
	 * descriptor and then increment available index.
	 */
	wmb();
	vrq->vring.avail->idx++;
}

static inline void virtqueue_detach_desc(struct virtqueue_vring *vrq,
					__u16 head_idx)
{
	struct vring_desc *desc;
	struct virtqueue_desc_info *vq_info;

	desc = &vrq->vring.desc[head_idx];
	vq_info = &vrq->vq_info[head_idx];
	vrq->desc_avail += vq_info->desc_count;
	vq_info->desc_count--;

	while (desc->flags & VRING_DESC_F_NEXT) {
		desc = &vrq->vring.desc[desc->next];
		vq_info->desc_count--;
	}

	/* The value should be empty */
	UK_ASSERT(vq_info->desc_count == 0);

	/* Appending the descriptor to the head of list */
	desc->next = vrq->head_free_desc;
	vrq->head_free_desc = head_idx;
}

int virtqueue_notify_enabled(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;

	UK_ASSERT(vq);
	vrq = to_virtqueue_vring(vq);

	return ((vrq->vring.used->flags & VRING_USED_F_NO_NOTIFY) == 0);
}

static inline int virtqueue_buffer_enqueue_segments(
		struct virtqueue_vring *vrq,
		__u16 head, struct uk_sglist *sg, __u16 read_bufs,
		__u16 write_bufs)
{
	int i = 0, total_desc = 0;
	struct uk_sglist_seg *segs;
	__u16 idx = 0;

	total_desc = read_bufs + write_bufs;

	for (i = 0, idx = head; i < total_desc; i++) {
		segs = &sg->sg_segs[i];
		vrq->vring.desc[idx].addr = segs->ss_paddr;
		vrq->vring.desc[idx].len = segs->ss_len;
		vrq->vring.desc[idx].flags = 0;
		if (i >= read_bufs)
			vrq->vring.desc[idx].flags |= VRING_DESC_F_WRITE;

		if (i < total_desc - 1)
			vrq->vring.desc[idx].flags |= VRING_DESC_F_NEXT;
		idx = vrq->vring.desc[idx].next;
	}
	return idx;
}

int virtqueue_hasdata(struct virtqueue *vq)
{
	struct virtqueue_vring *vring;

	UK_ASSERT(vq);

	vring = to_virtqueue_vring(vq);
	return (vring->last_used_desc_idx != vring->vring.used->idx);
}

__u64 virtqueue_feature_negotiate(__u64 feature_set)
{
	__u64 feature = (1ULL << VIRTIO_TRANSPORT_F_START) - 1;

	/**
	 * Currently out vring driver does not support any ring feature. We will
	 * add support to transport feature in the future.
	 */
	feature &= feature_set;
	return feature;
}

int virtqueue_ring_interrupt(void *obj)
{
	struct virtqueue *vq = (struct virtqueue *)obj;
	int rc = 0;

	UK_ASSERT(vq);

	if (!virtqueue_hasdata(vq))
		return rc;

	if (likely(vq->vq_callback))
		rc = vq->vq_callback(vq, vq->priv);
	return rc;
}

__phys_addr virtqueue_physaddr(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq = NULL;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	return ukplat_virt_to_phys(vrq->vring_mem);
}

int virtqueue_buffer_dequeue(struct virtqueue *vq, void **cookie, __u32 *len)
{
	struct virtqueue_vring *vrq = NULL;
	__u16 used_idx, head_idx;
	struct vring_used_elem *elem;

	UK_ASSERT(vq);
	UK_ASSERT(cookie);
	vrq = to_virtqueue_vring(vq);

	/* No new descriptor since last dequeue operation */
	if (!virtqueue_hasdata(vq))
		return -ENOMSG;
	used_idx = vrq->last_used_desc_idx++ & (vrq->vring.num - 1);
	elem = &vrq->vring.used->ring[used_idx];
	/**
	 * We are reading from the used descriptor information updated by the
	 * host.
	 */
	rmb();
	head_idx = elem->id;
	if (len)
		*len = elem->len;
	*cookie = vrq->vq_info[head_idx].cookie;
	virtqueue_detach_desc(vrq, head_idx);
	vrq->vq_info[head_idx].cookie = NULL;
	return (vrq->vring.num - vrq->desc_avail);
}

int virtqueue_buffer_enqueue(struct virtqueue *vq, void *cookie,
			     struct uk_sglist *sg, __u16 read_bufs,
			     __u16 write_bufs)
{
	__u32 total_desc = 0;
	__u16 head_idx = 0, idx = 0;
	struct virtqueue_vring *vrq = NULL;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	total_desc = read_bufs + write_bufs;
	if (unlikely(total_desc < 1 || total_desc > vrq->vring.num)) {
		uk_pr_err("%"__PRIu32" invalid number of descriptor\n",
			  total_desc);
		return -EINVAL;
	} else if (vrq->desc_avail < total_desc) {
		uk_pr_err("Available descriptor:%"__PRIu16", Requested descriptor:%"__PRIu32"\n",
			  vrq->desc_avail, total_desc);
		return -ENOSPC;
	}
	/* Get the head of free descriptor */
	head_idx = vrq->head_free_desc;
	UK_ASSERT(cookie);
	/* Additional information to reconstruct the data buffer */
	vrq->vq_info[head_idx].cookie = cookie;
	vrq->vq_info[head_idx].desc_count = total_desc;

	/**
	 * We separate the descriptor management to enqueue segment(s).
	 */
	idx = virtqueue_buffer_enqueue_segments(vrq, head_idx, sg,
			read_bufs, write_bufs);
	/* Metadata maintenance for the virtqueue */
	vrq->head_free_desc = idx;
	vrq->desc_avail -= total_desc;

	uk_pr_debug("Old head:%d, new head:%d, total_desc:%d\n",
		    head_idx, idx, total_desc);

	virtqueue_ring_update_avail(vrq, head_idx);
	return vrq->desc_avail;
}

static void virtqueue_vring_init(struct virtqueue_vring *vrq, __u16 nr_desc,
				 __u16 align)
{
	int i = 0;

	vring_init(&vrq->vring, nr_desc, vrq->vring_mem, align);

	vrq->desc_avail = vrq->vring.num;
	vrq->head_free_desc = 0;
	vrq->last_used_desc_idx = 0;
	for (i = 0; i < nr_desc - 1; i++)
		vrq->vring.desc[i].next = i + 1;
	/**
	 * When we reach this descriptor we have completely used all the
	 * descriptor in the vring.
	 */
	vrq->vring.desc[nr_desc - 1].next = VIRTQUEUE_MAX_SIZE;
}

struct virtqueue *virtqueue_create(__u16 queue_id, __u16 nr_descs, __u16 align,
				   virtqueue_callback_t callback,
				   virtqueue_notify_host_t notify,
				   struct virtio_dev *vdev, struct uk_alloc *a)
{
	struct virtqueue_vring *vrq;
	struct virtqueue *vq;
	int rc;
	size_t ring_size = 0;

	UK_ASSERT(a);

	vrq = uk_malloc(a, sizeof(struct virtqueue) +
			nr_descs * sizeof(struct virtqueue_desc_info));
	if (!vrq) {
		uk_pr_err("Allocation of virtqueue failed\n");
		rc = -ENOMEM;
		goto err_exit;
	}
	/**
	 * Initialize the value before referencing it in
	 * uk_posix_memalign as we don't set NULL on all failures in the
	 * allocation.
	 */
	vrq->vring_mem = NULL;

	ring_size = vring_size(nr_descs, align);
	if (uk_posix_memalign(a, &vrq->vring_mem,
			      __PAGE_SIZE, ring_size) != 0) {
		uk_pr_err("Allocation of vring failed\n");
		rc = -ENOMEM;
		goto err_freevq;
	}
	memset(vrq->vring_mem, 0, ring_size);
	virtqueue_vring_init(vrq, nr_descs, align);

	vq = &vrq->vq;
	vq->queue_id = queue_id;
	vq->vdev = vdev;
	vq->vq_callback = callback;
	vq->vq_notify_host = notify;
	return vq;

err_freevq:
	uk_free(a, vrq);
err_exit:
	return ERR2PTR(rc);
}

void virtqueue_destroy(struct virtqueue *vq, struct uk_alloc *a)
{
	struct virtqueue_vring *vrq;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);

	/* Free the ring */
	uk_free(a, vrq->vring_mem);

	/* Free the virtqueue metadata */
	uk_free(a, vrq);
}

int virtqueue_is_full(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	return (vrq->desc_avail == 0);
}
