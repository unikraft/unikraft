/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *          Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2015, IBM
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
/**
 * Taken and adapted from solo5 virtio_pci.h
 * kernel/virtio/virtio_pci.h
 * Commit-id: 6e0e12133aa7
 */

#ifndef __VIRTIO_PCI_H__
#define __VIRTIO_PCI_H__

#include <uk/arch/types.h>
#include <uk/compiler.h>
#include <virtio/virtio_types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus __ */

/* virtio config space layout */
#define VIRTIO_PCI_HOST_FEATURES        0    /* 32-bit r/o */
#define VIRTIO_PCI_GUEST_FEATURES       4    /* 32-bit r/w */
#define VIRTIO_PCI_QUEUE_PFN            8    /* 32-bit r/w */
#define VIRTIO_PCI_QUEUE_SIZE           12   /* 16-bit r/o */
#define VIRTIO_PCI_QUEUE_SEL            14   /* 16-bit r/w */
#define VIRTIO_PCI_QUEUE_NOTIFY         16   /* 16-bit r/w */

/*
 * Shift size used for writing physical queue address to QUEUE_PFN
 */
#define VIRTIO_PCI_QUEUE_ADDR_SHIFT     12

/*
 * The status register lets us tell the device where we are in
 * initialization
 */
#define VIRTIO_PCI_STATUS               18   /* 8-bit r/w */

/*
 * Reading the value will return the current contents of the interrupt
 * status register and will also clear it.  This is effectively a
 * read-and-acknowledge.
 */
#define VIRTIO_PCI_ISR                  19   /* 8-bit r/o */
#define VIRTIO_PCI_ISR_HAS_INTR         0x1  /* interrupt is for this device */
#define VIRTIO_PCI_ISR_CONFIG           0x2  /* config change bit */

/* TODO Revisit when adding MSI support. */
#define VIRTIO_PCI_CONFIG_OFF           20
#define VIRTIO_PCI_VRING_ALIGN          4096

#define VIRTIO_PCI_VENDOR_ID		 0x1af4
#define VIRTIO_PCI_LEGACY_DEVICEID_START 0x1000
#define VIRTIO_PCI_LEGACY_DEVICEID_END	 0x103f
#define VIRTIO_PCI_MODERN_DEVICEID_START 0x1040
#define VIRTIO_PCI_MODERN_DEVICEID_END	 0x107f

#define VIRTIO_PCI_CAP_VENDOR		 0x09

#define VIRTIO_PCI_CAP_COMMON_CFG	 1
#define VIRTIO_PCI_CAP_NOTIFY_CFG	 2
#define VIRTIO_PCI_CAP_ISR_CFG		 3
#define VIRTIO_PCI_CAP_DEVICE_CFG	 4
#define VIRTIO_PCI_CAP_PCI_CFG		 5
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8
#define VIRTIO_PCI_CAP_VENDOR_CFG	 9

#define VIRTIO_MSI_NO_VECTOR		 0xffff

struct virtio_pci_cap {
	__u8 cap_vndr;
	__u8 cap_next;
	__u8 cap_len;
	__u8 cfg_type;
	__u8 bar;
	__u8 id;
	__u8 padding[2];
	__virtio_le32 offset;
	__virtio_le32 length;
} __packed;

struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	__virtio_le32 notify_off_multiplier;
} __packed;

struct virtio_pci_common_cfg {
	__virtio_le32 device_feature_select;
	__virtio_le32 device_feature;
	__virtio_le32 driver_feature_select;
	__virtio_le32 driver_feature;
	__virtio_le16 config_msix_vector;
	__virtio_le16 num_queues;
	__u8 device_status;
	__u8 config_generation;
	__virtio_le16 queue_select;
	__virtio_le16 queue_size;
	__virtio_le16 queue_msix_vector;
	__virtio_le16 queue_enable;
	__virtio_le16 queue_notify_off;
	__virtio_le64 queue_desc;
	__virtio_le64 queue_driver;
	__virtio_le64 queue_device;
	__virtio_le16 queue_notify_data;
	__virtio_le16 queue_reset;
} __packed;

#ifdef __cplusplus
}
#endif /* __cplusplus __ */

#endif /* __VIRTIO_PCI_H__ */
