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

#include <virtio/virtio_types.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus __ */

/* virtio config space layout */
/* TODO we currently support only the legacy interface */
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


/* Types of capabilities for virtio configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG	1
#define VIRTIO_PCI_CAP_NOTIFY_CFG	2
#define VIRTIO_PCI_CAP_ISR		3
#define VIRTIO_PCI_CAP_DEVICE_CFG	4
#define VIRTIO_PCI_CAP_PCI_CFG		5
#define VIRTIO_PCI_CAP_SHARED_MEM_CFG	8


struct virtio_pci_cap {
	__u8 cap_vndr;
	__u8 cap_next;
	__u8 cap_len;
	__u8 cfg_type;
	__u8 bar;
	__u8 padding[3];
	__virtio_le32 offset;
	__virtio_le32 length;
};

struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	__virtio_le32 notify_off_multiplier;
};

struct virtio_pci_common_cfg {
	/* About the whole device. */
	__virtio_le32 host_feature_select;
	__virtio_le32 host_feature;
	__virtio_le32 guest_feature_select;
	__virtio_le32 guest_feature;
	__virtio_le16 msix_config;
	__virtio_le16 num_queues;
	__u8 device_status;
	__u8 config_generation;

	/* About a specific virtqueue. */
	__virtio_le16 queue_select;
	__virtio_le16 queue_size;
	__virtio_le16 queue_msix_vector;
	__virtio_le16 queue_enable;
	__virtio_le16 queue_notify_off;
	__virtio_le32 queue_desc_lo;
	__virtio_le32 queue_desc_hi;
	__virtio_le32 queue_driver_lo;
	__virtio_le32 queue_driver_hi;
	__virtio_le32 queue_device_lo;
	__virtio_le32 queue_device_hi;
};

/* Offsets calculation for consistency with virtio_mmio.h */
/* Device (host) features set selector - Write Only */
#define VIRTIO_PCI_CFG_DEVICE_FEATURES                                         \
	__offsetof(struct virtio_pci_common_cfg, host_feature)

/*
 * Bitmask of the features supported by the device (host)
 * (32 bits per set) - Read Only
 */
#define VIRTIO_PCI_CFG_DEVICE_FEATURES_SEL                                     \
	__offsetof(struct virtio_pci_common_cfg, host_feature_select)

/*
 * Bitmask of features activated by the driver (guest)
 * (32 bits per set) - Write Only
 */
#define VIRTIO_PCI_CFG_DRIVER_FEATURES                                         \
	__offsetof(struct virtio_pci_common_cfg, guest_feature)

/* Activated features set selector - Write Only */
#define VIRTIO_PCI_CFG_DRIVER_FEATURES_SEL                                     \
	__offsetof(struct virtio_pci_common_cfg, guest_feature_select)

/* Maximum number of queues - Read Only */
#define VIRTIO_PCI_CFG_NUM_QUEUES                                              \
	__offsetof(struct virtio_pci_common_cfg, num_queues)

/* Device status - Read Write */
#define VIRTIO_PCI_CFG_DEVICE_STATUS                                           \
	__offsetof(struct virtio_pci_common_cfg, device_status)

/* Configuration atomicity value */
#define VIRTIO_PCI_CFG_CONFIG_GENERATION                                       \
	__offsetof(struct virtio_pci_common_cfg, config_generation)

/* Queue selector - Write Only */
#define VIRTIO_PCI_CFG_QUEUE_SEL                                               \
	__offsetof(struct virtio_pci_common_cfg, queue_select)

/* Maximum size of the currently selected queue - Read Only */
#define VIRTIO_PCI_CFG_QUEUE_SIZE                                              \
	__offsetof(struct virtio_pci_common_cfg, queue_size)

/* Ready bit for the currently selected queue - Read Write */
#define VIRTIO_PCI_CFG_QUEUE_READY                                             \
	__offsetof(struct virtio_pci_common_cfg, queue_enable)

/* Offset from the notification structure where this virtqueue is located - Read
 * Only
 */
#define VIRTIO_PCI_CFG_QUEUE_NOTIFY_OFF                                        \
	__offsetof(struct virtio_pci_common_cfg, queue_notify_off)

/* Selected queue's Descriptor Table address, 64 bits in two halves */
#define VIRTIO_PCI_CFG_QUEUE_DESC_LOW                                          \
	__offsetof(struct virtio_pci_common_cfg, queue_desc_lo)
#define VIRTIO_PCI_CFG_QUEUE_DESC_HIGH                                         \
	__offsetof(struct virtio_pci_common_cfg, queue_desc_hi)

/* Selected queue's Available Ring address, 64 bits in two halves */
#define VIRTIO_PCI_CFG_QUEUE_AVAIL_LOW                                         \
	__offsetof(struct virtio_pci_common_cfg, queue_driver_lo)
#define VIRTIO_PCI_CFG_QUEUE_AVAIL_HIGH                                        \
	__offsetof(struct virtio_pci_common_cfg, queue_driver_hi)

/* Selected queue's Used Ring address, 64 bits in two halves */
#define VIRTIO_PCI_CFG_QUEUE_USED_LOW                                          \
	__offsetof(struct virtio_pci_common_cfg, queue_device_lo)
#define VIRTIO_PCI_CFG_QUEUE_USED_HIGH                                         \
	__offsetof(struct virtio_pci_common_cfg, queue_device_hi)

#ifdef __cplusplus
}
#endif /* __cplusplus __ */

#endif /* __VIRTIO_PCI_H__ */
