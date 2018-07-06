/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
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
/* An interface for efficient virtio implementation.
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers.
 *
 * Copyright 2007, 2009, IBM Corporation
 * Copyright 2011, Red Hat, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL IBM OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __PLAT_CMN_PCI_VIRTIO_RING_H__
#define __PLAT_CMN_PCI_VIRTIO_RING_H__

#include <uk/arch/limits.h>
#include <uk/alloc.h>

/*
 * TODO In the future we may find that the Linux variant of virtio_ring.h
 * (in include/uapi/linux/virtio_ring.h) may be more suitable for our needs.
 */

typedef __u16 __virtio_le16;
typedef __u32 __virtio_le32;
typedef __u64 __virtio_le64;

#if __BYTE_ORDER__ !=  __ORDER_LITTLE_ENDIAN__
#error "Please define conversion functions from host to little endian!"
#endif

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT       1
/* This marks a buffer as write-only (otherwise read-only). */
#define VIRTQ_DESC_F_WRITE      2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT   4

/*
 * The device uses this in used->flags to advise the driver:
 * don't kick me when you add a buffer. It's unreliable, so
 * it's simply an optimization.
 */
#define VIRTQ_USED_F_NO_NOTIFY  1

/*
 * The driver uses this in avail->flags to advise the device:
 * don't interrupt me when you consume a buffer. It's unreliable, so
 * it's simply an optimization.
 */
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1

/* Support for indirect descriptors */
#define VIRTIO_F_INDIRECT_DESC    28

/* Support for avail_event and used_event fields */
#define VIRTIO_F_EVENT_IDX        29

/* Arbitrary descriptor layouts. */
#define VIRTIO_F_ANY_LAYOUT       27


/*
 * Virtqueue descriptors: 16 bytes.
 * These can chain together via "next".
 */
struct virtq_desc {
	/* Address (guest-physical). */
	__virtio_le64 addr;
	/* Length. */
	__virtio_le32 len;
	/* The flags as indicated above. */
	__virtio_le16 flags;
	/* We chain unused descriptors via this, too */
	__virtio_le16 next;
};

struct virtq_avail {
	__virtio_le16 flags;
	__virtio_le16 idx;
	__virtio_le16 ring[];
	/* Only if VIRTIO_F_EVENT_IDX: __virtio_le16 used_event; */
};

/* __virtio_le32 is used here for ids for padding reasons. */
struct virtq_used_elem {
	/* Index of start of used descriptor chain. */
	__virtio_le32 id;
	/* Total length of the descriptor chain which was written to. */
	__virtio_le32 len;
};

struct virtq_used {
	__virtio_le16 flags;
	__virtio_le16 idx;
	struct virtq_used_elem ring[];
	/* Only if VIRTIO_F_EVENT_IDX: __virtio_le16 avail_event; */
};

/*
 * TODO We set the maximum buffer length as it was in Solo5. The value
 * was chosen for keeping the 12 byte header and 1514 bytes for the
 * network packet. Support should be added in order to allow drivers
 * to configure their own buffer sizes (adopting the Linux variant of
 * virtio_ring.h would help on that matter).
 */
#define MAX_BUFFER_LEN 1526

/*
 * Each one of these io_buffer's map to a descriptor.
 * An array of io_buffer's of size virtq->num (same
 * as virtq->desc) is allocated during init.
 */
struct io_buffer {
	__u8 data[MAX_BUFFER_LEN];

	/*
	 * Data length in bytes. It is written by the driver on
	 * a tx/write, or by the device on a rx/read on interrupt
	 * handling.
	 */
	__u32 len;

	/* Extra flags to be added to the corresponding descriptor. */
	__u16 extra_flags;
};

struct virtq {
	unsigned int num;

	struct virtq_desc *desc;
	struct virtq_avail *avail;
	struct virtq_used *used;
	struct io_buffer *bufs;

	/* Keep track of available (free) descriptors */
	__u16 num_avail;

	/* Indexes in the descriptors array */
	__u16 last_used;
	__u16 next_avail;
};

/*
 * Helper macros for accessing virtqueue fields
 */

#define VIRTQ_OFF_DESC(q)         0

#define VIRTQ_OFF_AVAIL(q)        ((q)->num * sizeof(struct virtq_desc))
#define VIRTQ_OFF_AVAIL_RING(q) \
	(VIRTQ_OFF_AVAIL(q) + sizeof(struct virtq_avail))

#define VIRTQ_OFF_PADDING(q) \
	(VIRTQ_OFF_AVAIL_RING(q) + (sizeof(__virtio_le16) * (q)->num))

#define VIRTQ_OFF_USED(q) \
	((VIRTQ_OFF_PADDING(q) + __PAGE_SIZE - 1) & __PAGE_MASK)
#define VIRTQ_OFF_USED_RING(q) \
	(VIRTQ_OFF_USED(q) + sizeof(struct virtq_used))

#define VIRTQ_SIZE(q) \
	(VIRTQ_OFF_USED_RING(q) + (sizeof(struct virtq_used_elem) * (q)->num))

static inline
int virtq_need_event(__u16 event_idx, __u16 new_idx, __u16 old_idx)
{
	return (new_idx - event_idx - 1) < (new_idx - old_idx);
}

/* Get location of event indices (only with VIRTIO_F_EVENT_IDX) */
static inline __virtio_le16 *virtq_used_event(struct virtq *vq)
{
	/*
	 * For backwards compatibility, used event index
	 * is at *end* of avail ring.
	 */
	return &vq->avail->ring[vq->num];
}

static inline __virtio_le16 *virtq_avail_event(struct virtq *vq)
{
	/*
	 * For backwards compatibility, avail event index
	 * is at *end* of used ring.
	 */
	return (__virtio_le16 *) &vq->used->ring[vq->num];
}

/*
 * Create a descriptor chain starting at index head,
 * using vq->bufs also starting at index head.
 * @param vq Virtual queue
 * @param head Starting descriptor index
 * @param num Number of descriptors (and number of bufs).
 * @return 0 on success, < 0 otherwise
 */
int virtq_add_descriptor_chain(struct virtq *vq,
		__u16 head, __u16 num);

/*
 * Initializes a virtual queue
 * @param vq Virtual queue
 * @param pci_base Base in PCI configuration space
 * @param queue_select Virtual queue selector
 * @param a Memory allocator
 * @return 0 on success, < 0 otherwise
 */
int virtq_rings_init(struct virtq *vq, __u16 pci_base,
			__virtio_le16 queue_select, struct uk_alloc *a);

/*
 * Deinitializes a virtual queue
 * @param vq Virtual queue
 * @param pci_base Base in PCI configuration space
 * @param queue_select Virtual queue selector
 * @param a Memory allocator
 */
void virtq_rings_fini(struct virtq *vq, __u16 pci_base,
			__virtio_le16 queue_select, struct uk_alloc *a);

#endif /* __PLAT_CMN_PCI_VIRTIO_RING_H__ */
