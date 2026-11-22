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
 */
/**
 * Inspired from the FreeBSD.
 * Commit-id: a89e7a10d501
 */

#include <string.h>

#include <uk/arch/util.h>
#include <uk/atomic.h>
#include <uk/config.h>
#include <uk/errptr.h>
#include <uk/paging.h>
#include <uk/plat/spinlock.h>
#include <uk/print.h>
#include <uk/sglist.h>

#include <virtio/virtio_ring.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_bus.h>

#ifdef CONFIG_LIBUKVMEM
#include <uk/falloc.h>
#include <uk/vmem.h>
#endif /* CONFIG_LIBUKVMEM */

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
	__spinlock lock;
	/* Keep track of available descriptors */
	__u16 desc_avail;
	/* Index of the next available slot */
	__u16 head_free_desc;
	/* Index of the last used descriptor by the host */
	__u16 last_used_desc_idx;
	/* EVENT_IDX notification suppression is used */
	__u8 uses_event_idx;
	/* EVENT_IDX last notified idx */
	__u16 last_notified_idx;
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

static inline int virtqueue_hasdata_locked(struct virtqueue_vring *vrq)
{
	return (vrq->last_used_desc_idx != UK_READ_ONCE(vrq->vring.used->idx));
}

static inline void virtqueue_intr_disable_locked(struct virtqueue_vring *vrq)
{
	if (vrq->uses_event_idx) {
		vring_used_event(&vrq->vring) =
			vrq->last_used_desc_idx - vrq->vring.num - 1;
		return;
	}
	vrq->vring.avail->flags |= (VRING_AVAIL_F_NO_INTERRUPT);
}

/**
 * Driver implementation
 */
void virtqueue_intr_disable(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;
	unsigned long irqf;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	ukplat_spin_lock_irqsave(&vrq->lock, irqf);
	virtqueue_intr_disable_locked(vrq);
	ukplat_spin_unlock_irqrestore(&vrq->lock, irqf);
}

static inline int virtqueue_intr_enable_locked(struct virtqueue_vring *vrq)
{
	int rc = 0;

	/* Check if there are no more packets enabled */
	if (!virtqueue_hasdata_locked(vrq)) {
		if (vrq->uses_event_idx) {
			/* TODO: This allows delaying the interrupts by a
			 *       adjustable count of descriptors. This is could
			 *       be useful for TXQ interrupts, which can be
			 *       handled in a batched fashion.
			 */
			vring_used_event(&vrq->vring) =
			    vrq->last_used_desc_idx + 0;
		} else {
			vrq->vring.avail->flags &=
				(~VRING_AVAIL_F_NO_INTERRUPT);
		}

		/**
		 * We enabled the interrupts. We ensure it using the
		 * memory barrier and check if there are any further
		 * data available in the queue. The check for data
		 * after enabling the interrupt is to make sure we do
		 * not miss any interrupt while transitioning to enable
		 * interrupt. This is inline with the requirement from
		 * virtio specification section 3.2.2
		 */
		uk_arch_mb();
		/* Check if there are further descriptors */
		if (virtqueue_hasdata_locked(vrq)) {
			virtqueue_intr_disable_locked(vrq);
			rc = 1;
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

int virtqueue_intr_enable(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;
	unsigned long irqf;
	int rc;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	ukplat_spin_lock_irqsave(&vrq->lock, irqf);
	rc = virtqueue_intr_enable_locked(vrq);
	ukplat_spin_unlock_irqrestore(&vrq->lock, irqf);
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
	uk_arch_wmb();
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

static inline int virtqueue_notify_enabled_locked(struct virtqueue_vring *vrq)
{
	__u16 old, new;

	if (vrq->uses_event_idx) {
		new = vrq->vring.avail->idx;
		old = vrq->last_notified_idx;

		return vring_need_event(vring_avail_event(&vrq->vring),
					new, old);
	}

	return ((vrq->vring.used->flags & VRING_USED_F_NO_NOTIFY) == 0);
}

void virtqueue_host_notify(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;
	unsigned long irqf;
	int do_notify = 0;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);

	/*
	 * Before notifying the virtio backend on the host we should make sure
	 * that the virtqueue index update operation happened. Note that this
	 * function is declared as inline.
	 */
	uk_arch_mb();

	ukplat_spin_lock_irqsave(&vrq->lock, irqf);
	if (vq->vq_notify_host && virtqueue_notify_enabled_locked(vrq)) {
		vrq->last_notified_idx = vrq->vring.avail->idx;
		do_notify = 1;
	}
	ukplat_spin_unlock_irqrestore(&vrq->lock, irqf);

	if (do_notify) {
		uk_pr_debug_isr("notify queue %d\n", vq->queue_id);
		vq->vq_notify_host(vq->vdev, vq->queue_id);
	}
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
	unsigned long irqf;
	int rc;

	UK_ASSERT(vq);

	vring = to_virtqueue_vring(vq);
	ukplat_spin_lock_irqsave(&vring->lock, irqf);
	rc = virtqueue_hasdata_locked(vring);
	ukplat_spin_unlock_irqrestore(&vring->lock, irqf);
	return rc;
}

__u64 virtqueue_feature_negotiate(__u64 feature_set)
{
	__u64 feature = (1ULL << VIRTIO_TRANSPORT_F_START) - 1;

	/* Allow version 1 flag */
	feature |= 1ULL << VIRTIO_F_VERSION_1;
	/* Allow event index feature */
	feature |= 1ULL << VIRTIO_F_EVENT_IDX;

	feature &= feature_set;
	return feature;
}

int virtqueue_ring_interrupt(void *obj)
{
	struct virtqueue *vq = (struct virtqueue *)obj;

	UK_ASSERT(vq);

	/* It is possible that the vqueue is empty if the
	 * interrupt arrives before the data is observable
	 * by the device. In that case there's not much we
	 * can do now but defer handling to the next irq.
	 */
	if (!virtqueue_hasdata(vq))
		return 1;

	return (likely(vq->vq_callback)) ? vq->vq_callback(vq, vq->priv) : 1;
}

__paddr_t virtqueue_physaddr(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq = NULL;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	return uk_paging_virt_to_phys((__vaddr_t)vrq->vring_mem);
}

__paddr_t virtqueue_get_avail_addr(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq = NULL;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	return virtqueue_physaddr(vq) +
		((char *)vrq->vring.avail - (char *)vrq->vring.desc);
}

__paddr_t virtqueue_get_used_addr(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq = NULL;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	return virtqueue_physaddr(vq) +
		((char *)vrq->vring.used - (char *)vrq->vring.desc);
}

unsigned int virtqueue_vring_get_num(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq = NULL;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	return vrq->vring.num;
}

static int virtqueue_buffer_dequeue_locked(struct virtqueue_vring *vrq,
					   void **cookie, __u32 *len)
{
	__u16 used_idx, head_idx;
	struct vring_used_elem *elem;

	/* No new descriptor since last dequeue operation */
	if (!virtqueue_hasdata_locked(vrq))
		return -ENOMSG;
	used_idx = vrq->last_used_desc_idx++ & (vrq->vring.num - 1);
	elem = &vrq->vring.used->ring[used_idx];
	/**
	 * We are reading from the used descriptor information updated by the
	 * host.
	 */
	uk_arch_rmb();
	head_idx = elem->id;
	if (len)
		*len = elem->len;
	*cookie = vrq->vq_info[head_idx].cookie;
	virtqueue_detach_desc(vrq, head_idx);
	vrq->vq_info[head_idx].cookie = NULL;
	return (vrq->vring.num - vrq->desc_avail);
}

int virtqueue_buffer_dequeue(struct virtqueue *vq, void **cookie, __u32 *len)
{
	struct virtqueue_vring *vrq = NULL;
	unsigned long irqf;
	int rc;

	UK_ASSERT(vq);
	UK_ASSERT(cookie);
	vrq = to_virtqueue_vring(vq);
	ukplat_spin_lock_irqsave(&vrq->lock, irqf);
	rc = virtqueue_buffer_dequeue_locked(vrq, cookie, len);
	ukplat_spin_unlock_irqrestore(&vrq->lock, irqf);
	return rc;
}

static int virtqueue_buffer_enqueue_locked(struct virtqueue_vring *vrq,
					   void *cookie, struct uk_sglist *sg,
					   __u16 read_bufs, __u16 write_bufs)
{
	__u32 total_desc = 0;
	__u16 head_idx = 0, idx = 0;

	total_desc = read_bufs + write_bufs;
	if (unlikely(total_desc < 1 || total_desc > vrq->vring.num)) {
		uk_pr_err_isr("%"__PRIu32" invalid number of descriptor\n",
			      total_desc);
		return -EINVAL;
	} else if (vrq->desc_avail < total_desc) {
		uk_pr_debug_isr("Available descriptor:%"__PRIu16", Requested descriptor:%"__PRIu32"\n",
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

	uk_pr_debug_isr("Old head:%d, new head:%d, total_desc:%d\n",
			head_idx, idx, total_desc);

	virtqueue_ring_update_avail(vrq, head_idx);
	return vrq->desc_avail;
}

int virtqueue_buffer_enqueue(struct virtqueue *vq, void *cookie,
			     struct uk_sglist *sg, __u16 read_bufs,
			     __u16 write_bufs)
{
	struct virtqueue_vring *vrq = NULL;
	unsigned long irqf;
	int rc;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	ukplat_spin_lock_irqsave(&vrq->lock, irqf);
	rc = virtqueue_buffer_enqueue_locked(vrq, cookie, sg,
					     read_bufs, write_bufs);
	ukplat_spin_unlock_irqrestore(&vrq->lock, irqf);
	return rc;
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

#if CONFIG_LIBUKVMEM
/* The heap is paged in on demand, so take contiguous frames and map them */
static void *virtqueue_vring_alloc(struct uk_alloc *a __unused, __sz size)
{
	struct uk_pagetable *pt = uk_paging_pt_get_active();
	__paddr_t paddr = UK_PAGING_PADDR_ANY;
	__vaddr_t vaddr = UK_PAGING_VADDR_ANY;
	int rc;

	size = UK_PAGING_PAGE_ALIGN_UP(size);

	rc = pt->fa->falloc(pt->fa, &paddr, size >> UK_PAGING_PAGE_SHIFT, 0);
	if (unlikely(rc))
		return NULL;

	rc = uk_vma_map_dma(uk_vas_get_active(), &vaddr, size,
			    UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			    "virtqueue", paddr);
	if (unlikely(rc)) {
		pt->fa->ffree(pt->fa, paddr, size >> UK_PAGING_PAGE_SHIFT);
		return NULL;
	}

	return (void *)vaddr;
}

static void virtqueue_vring_free(struct uk_alloc *a __unused, void *mem,
				 __sz size)
{
	struct uk_pagetable *pt = uk_paging_pt_get_active();
	__paddr_t paddr = uk_paging_virt_to_phys((__vaddr_t)mem);
	int rc;

	size = UK_PAGING_PAGE_ALIGN_UP(size);

	rc = uk_vma_unmap(uk_vas_get_active(), (__vaddr_t)mem, size, 0);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to unmap virtio ring %p: %d\n", mem, rc);
		return;
	}

	/* DMA mappings do not own their frames */
	pt->fa->ffree(pt->fa, paddr, size >> UK_PAGING_PAGE_SHIFT);
}
#else /* !CONFIG_LIBUKVMEM */
/* The device uses only the physical base, so find a contiguous ring.
 * Keep the blocks that fail until the end, or the allocator gives them again.
 */
static void *virtqueue_vring_alloc(struct uk_alloc *a, __sz size)
{
	void *mem = NULL, *next, *rejected = NULL;

	while (!uk_posix_memalign(a, &mem, __PAGE_SIZE, size)) {
		if (uk_sglist_count(mem, size) == 1)
			break;

		uk_pr_debug_isr("vring at %p is not physically contiguous\n",
				mem);
		*(void **)mem = rejected;
		rejected = mem;

		/* uk_posix_memalign() can keep the old mem when it fails */
		mem = NULL;
	}

	while (rejected) {
		next = *(void **)rejected;
		uk_free(a, rejected);
		rejected = next;
	}

	return mem;
}

static void virtqueue_vring_free(struct uk_alloc *a, void *mem,
				 __sz size __unused)
{
	uk_free(a, mem);
}
#endif /* !CONFIG_LIBUKVMEM */

struct virtqueue *virtqueue_create(__u16 queue_id, __u16 nr_descs, __u16 align,
				   virtqueue_callback_t callback,
				   virtqueue_notify_host_t notify,
				   struct virtio_dev *vdev, struct uk_alloc *a)
{
	struct virtqueue_vring *vrq;
	struct virtqueue *vq;
	int rc;
	__sz ring_size = 0;

	UK_ASSERT(a);

	vrq = uk_malloc(a, sizeof(*vrq) +
			nr_descs * sizeof(struct virtqueue_desc_info));
	if (!vrq) {
		uk_pr_err_isr("Allocation of virtqueue failed\n");
		rc = -ENOMEM;
		goto err_exit;
	}
	ukarch_spin_init(&vrq->lock);

	ring_size = vring_size(nr_descs, align);
	vrq->vring_mem = virtqueue_vring_alloc(a, ring_size);
	if (unlikely(!vrq->vring_mem)) {
		rc = -ENOMEM;
		goto err_freevq;
	}
	memset(vrq->vring_mem, 0, ring_size);
	virtqueue_vring_init(vrq, nr_descs, align);
	vrq->uses_event_idx =
		VIRTIO_FEATURE_HAS(vdev->features, VIRTIO_F_EVENT_IDX);
	vrq->last_notified_idx = 0;


	vq = &vrq->vq;
	vq->queue_id = queue_id;
	vq->vdev = vdev;
	vq->vq_callback = callback;
	vq->vq_notify_host = notify;
	return vq;

err_freevq:
	uk_pr_err_isr("Allocation of vring failed\n");
	uk_free(a, vrq);
err_exit:
	return ERR2PTR(rc);
}

void virtqueue_destroy(__u16 align, struct virtqueue *vq, struct uk_alloc *a)
{
	struct virtqueue_vring *vrq;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);

	virtqueue_vring_free(a, vrq->vring_mem,
			     vring_size(vrq->vring.num, align));

	/* Free the virtqueue metadata */
	uk_free(a, vrq);
}

int virtqueue_is_full(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;
	unsigned long irqf;
	int full;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	ukplat_spin_lock_irqsave(&vrq->lock, irqf);
	full = (vrq->desc_avail == 0);
	ukplat_spin_unlock_irqrestore(&vrq->lock, irqf);
	return full;
}

unsigned int virtqueue_desc_avail(struct virtqueue *vq)
{
	struct virtqueue_vring *vrq;
	unsigned long irqf;
	unsigned int avail;

	UK_ASSERT(vq);

	vrq = to_virtqueue_vring(vq);
	ukplat_spin_lock_irqsave(&vrq->lock, irqf);
	avail = vrq->desc_avail;
	ukplat_spin_unlock_irqrestore(&vrq->lock, irqf);
	return avail;
}
