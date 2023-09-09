/* SPDX-License-Identifier: BSD-3-Clause */
/* An interface for efficient virtio implementation, currently for use by KVM,
 * but hopefully others soon.  Do NOT change this since it will
 * break existing servers and clients.
 *
 * This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright Rusty Russell IBM Corporation 2007. */
/**
 * Taken and modified from Linux.
 * include/uapi/linux/virtio_ring.h
 *
 * Commit id: ecda85e70277
 */

#ifndef __PLAT_DRV_VIRTIO_RING_H__
#define __PLAT_DRV_VIRTIO_RING_H__

#include <uk/arch/limits.h>
#include <virtio/virtio_types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus __ */

#if __BYTE_ORDER__ !=  __ORDER_LITTLE_ENDIAN__
#error "Please define conversion functions from host to little endian!"
#endif

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT       1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE      2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT   4

/*
 * The device uses this in used->flags to advise the driver:
 * don't kick me when you add a buffer. It's unreliable, so
 * it's simply an optimization.
 */
#define VRING_USED_F_NO_NOTIFY  1

/*
 * The driver uses this in avail->flags to advise the device:
 * don't interrupt me when you consume a buffer. It's unreliable, so
 * it's simply an optimization.
 */
#define VRING_AVAIL_F_NO_INTERRUPT      1

/* Support for indirect descriptors */
#define VIRTIO_F_INDIRECT_DESC    28

/* Support for avail_event and used_event fields */
#define VIRTIO_F_EVENT_IDX        29

/* Arbitrary descriptor layouts. */
#define VIRTIO_F_ANY_LAYOUT       27

/**
 * Virtqueue descriptors: 16 bytes.
 * These can chain together via "next".
 */
struct vring_desc {
	/* Address (guest-physical). */
	__virtio_le64 addr;
	/* Length. */
	__virtio_le32 len;
	/* The flags as indicated above. */
	__virtio_le16 flags;
	/* We chain unused descriptors via this, too */
	__virtio_le16 next;
};

struct vring_avail {
	__virtio_le16 flags;
	__virtio_le16 idx;
	__virtio_le16 ring[];
	/* Only if VIRTIO_F_EVENT_IDX: __virtio_le16 used_event; */
};

/* __virtio_le32 is used here for ids for padding reasons. */
struct vring_used_elem {
	/* Index of start of used descriptor chain. */
	__virtio_le32 id;
	/* Total length of the descriptor chain which was written to. */
	__virtio_le32 len;
};

struct vring_used {
	__virtio_le16 flags;
	__virtio_le16 idx;
	struct vring_used_elem ring[];
	/* Only if VIRTIO_F_EVENT_IDX: __virtio_le16 avail_event; */
};

struct vring {
	unsigned int num;

	struct vring_desc *desc;
	struct vring_avail *avail;
	struct vring_used *used;
};

/* The standard layout for the ring is a continuous chunk of memory which
 * looks like this.  We assume num is a power of 2.
 *
 * struct vring {
 *      // The actual descriptors (16 bytes each)
 *      struct vring_desc desc[num];
 *
 *      // A ring of available descriptor heads with free-running index.
 *      __virtio_le16 avail_flags;
 *      __virtio_le16 avail_idx;
 *      __virtio_le16 available[num];
 *      __virtio_le16 used_event_idx;
 *
 *      // Padding to the next align boundary.
 *      char pad[];
 *
 *      // A ring of used descriptor heads with free-running index.
 *      __virtio_le16 used_flags;
 *      __virtio_le16 used_idx;
 *      struct vring_used_elem used[num];
 *      __virtio_le16 avail_event_idx;
 * };
 *
 * NOTE: for VirtIO PCI, align is 4096.
 */

/**
 * We publish the used event index at the end of the available ring, and vice
 * versa. They are at the end for backwards compatibility.
 */
#define vring_used_event(vr) ((vr)->avail->ring[(vr)->num])
#define vring_avail_event(vr) (*(__virtio16 *)&(vr)->used->ring[(vr)->num])

static inline void vring_init(struct vring *vr, unsigned int num, uint8_t *p,
			      unsigned long align)
{
	vr->num = num;
	vr->desc = (struct vring_desc *) p;
	vr->avail = (struct vring_avail *) (p +
			num * sizeof(struct vring_desc));
	vr->used = (void *)
	(((unsigned long) &vr->avail->ring[num] + align - 1) & ~(align - 1));
}

static inline unsigned int vring_size(unsigned int num, unsigned long align)
{
	int size;

	size = num * sizeof(struct vring_desc);
	size += sizeof(struct vring_avail) + (num * sizeof(uint16_t)) +
		sizeof(uint16_t);
	size = (size + align - 1) & ~(align - 1);
	size += sizeof(struct vring_used) +
	    (num * sizeof(struct vring_used_elem)) + sizeof(uint16_t);
	return size;
}

static inline int vring_need_event(__u16 event_idx, __u16 new_idx,
				   __u16 old_idx)
{
	return (new_idx - event_idx - 1) < (new_idx - old_idx);
}

#ifdef __cplusplus
}
#endif /* __cplusplus __ */

#endif /* __PLAT_DRV_VIRTIO_RING_H__ */
