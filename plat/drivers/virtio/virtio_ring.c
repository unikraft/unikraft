/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Ricardo Koller
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Taken and adapted from solo5 virtio_ring.c */

#include <string.h>
#include <uk/print.h>
#include <cpu.h>
#include <io.h>
#include <pci/virtio/virtio_pci.h>
#include <pci/virtio/virtio_ring.h>
#include <uk/arch/atomic.h>

#define VIRTQ_MAX_QUEUE_SIZE  32768


/*
 * Create a descriptor chain starting at index head, using vq->bufs
 * also starting at index head.
 * Make sure the vq-bufs are cleaned before using them again.
 */
int virtq_add_descriptor_chain(struct virtq *vq, __u16 head, __u16 num)
{
	struct virtq_desc *desc;
	__u16 used_descs, mask, i;

	UK_ASSERT(vq != NULL);

	if (head >= vq->num)
		return -EINVAL;

	if (num == 0 || num > vq->num)
		return -EINVAL;

	if (vq->num_avail < num) {
		uk_pr_warn("virtq full! next_avail:%"__PRIu16" last_used:%"__PRIu16"\n",
			   vq->next_avail, vq->last_used);
		return -ENOMEM;
	}

	used_descs = num;
	mask = vq->num - 1;

	for (i = head; used_descs > 0; used_descs--) {
		struct io_buffer *buf = &vq->bufs[i];
		__u32 len = 0;

		/*
		 * The first field of a "struct io_buffer" is the "data" field,
		 * so in the interrupt handler we can just cast this pointer
		 * back into a 'struct io_buffer'.
		 */
		UK_ASSERT(buf->data == (__u8 *) buf);

		len = ukarch_load_n(&buf->len);
		UK_ASSERT(len <= MAX_BUFFER_LEN);

		desc = &vq->desc[i];
		desc->addr = (__u64) buf->data;
		desc->len = len;
		desc->flags = VIRTQ_DESC_F_NEXT | buf->extra_flags;

		i = (i + 1) & mask;
		desc->next = i;
	}

	/* The last descriptor in the chain does not have a next */
	desc->next = 0;
	desc->flags &= ~VIRTQ_DESC_F_NEXT;

	vq->num_avail -= num;
	vq->avail->ring[vq->avail->idx & mask] = head;
	/* The new entry must be set before announcing it. */
	wmb();
	/* avail->idx always increments and wraps naturally at 65536 */
	vq->avail->idx++;
	vq->next_avail += num;

	return 0;
}

int virtq_rings_init(struct virtq *vq, __u16 pci_base,
			__virtio_le16 queue_select, struct uk_alloc *a)
{
	__u8 *data = NULL;
	__u16 vq_num;
	__sz vq_size;
	__phys_addr pa;

	UK_ASSERT(vq != NULL);
	UK_ASSERT(a != NULL);

	/* read queue size */
	outw(pci_base + VIRTIO_PCI_QUEUE_SEL, queue_select);
	vq_num = inw(pci_base + VIRTIO_PCI_QUEUE_SIZE);

	if (vq_num == 0) {
		uk_pr_err("No such queue: pci_base=%"__PRIx16" selector=%"__PRIx16"\n",
			  pci_base, queue_select);
		return -EINVAL;
	}

	UK_ASSERT(vq_num <= VIRTQ_MAX_QUEUE_SIZE);

	vq->last_used = vq->next_avail = 0;
	vq->num = vq->num_avail = vq_num;

	vq_size = VIRTQ_SIZE(vq);

	/* allocate queue memory */
	uk_posix_memalign_ifpages(a, (void **) &data, __PAGE_SIZE, vq_size);
	if (!data)
		return -ENOMEM;

	memset(data, 0, vq_size);

	vq->desc = (struct virtq_desc *) (data + VIRTQ_OFF_DESC(vq));
	vq->avail = (struct virtq_avail *) (data + VIRTQ_OFF_AVAIL(vq));
	vq->used = (struct virtq_used *) (data + VIRTQ_OFF_USED(vq));

	/* set queue memory */
	outw(pci_base + VIRTIO_PCI_QUEUE_SEL, queue_select);

	/* use physical address */
	pa = ukplat_virt_to_phys(data);
	outl(pci_base + VIRTIO_PCI_QUEUE_PFN,
		(pa >> VIRTIO_PCI_QUEUE_ADDR_SHIFT));

	return 0;
}

void virtq_rings_fini(struct virtq *vq, __u16 pci_base,
		__virtio_le16 queue_select, struct uk_alloc *a)
{
	__u8 *data;

	UK_ASSERT(vq != NULL);
	UK_ASSERT(a != NULL);

	/* reset queue memory */
	outw(pci_base + VIRTIO_PCI_QUEUE_SEL, queue_select);
	outl(pci_base + VIRTIO_PCI_QUEUE_PFN, 0);

	/* free queue memory */
	data = (__u8 *) vq->desc - VIRTQ_OFF_DESC(vq->num);
	uk_free(a, data);

	/* cleanup the queue */
	memset(vq, 0, sizeof(*vq));
}
