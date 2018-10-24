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
#include <cpu.h>
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
static void virtqueue_vring_init(struct virtqueue_vring *vrq, __u16 nr_desc,
				 __u16 align);

/**
 * Driver implementation
 */
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

__phys_addr virtqueue_physaddr(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq = NULL;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	return ukplat_virt_to_phys(vrq->vring_mem);
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
	uk_posix_memalign(a, (void **)&vrq->vring_mem, __PAGE_SIZE, ring_size);
	if (!vrq->vring_mem) {
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
