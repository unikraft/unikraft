/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Internal virtio PCI transport definitions.
 */

#ifndef __VIRTIO_PCI_PRIVATE_H__
#define __VIRTIO_PCI_PRIVATE_H__

#include <stddef.h>
#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/bus/pci.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_pci.h>

#define VPCI_COMMON_CFG_MIN_LEN \
	(offsetof(struct virtio_pci_common_cfg, queue_device) + \
	 sizeof(__virtio_le64))
#define VPCI_CFG_OFF(member) \
	offsetof(struct virtio_pci_common_cfg, member)

#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
#define VPCI_TEST_VISIBLE
#define VPCI_TEST_EXTERN extern
#else /* !(CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL) */
#define VPCI_TEST_VISIBLE static
#define VPCI_TEST_EXTERN static
#endif /* !(CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL) */

enum virtio_pci_transport {
	VIRTIO_PCI_TRANSPORT_LEGACY = 0,
	VIRTIO_PCI_TRANSPORT_MODERN,
};

struct virtio_pci_region {
	enum pci_bar_type type;
	__u8 bar;
	__u64 base;
	__u32 length;
#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
	void *test_cookie;
	__u8 (*test_read8)(void *cookie, __u32 off);
	__u16 (*test_read16)(void *cookie, __u32 off);
	__u32 (*test_read32)(void *cookie, __u32 off);
	void (*test_write8)(void *cookie, __u32 off, __u8 val);
	void (*test_write16)(void *cookie, __u32 off, __u16 val);
	void (*test_write32)(void *cookie, __u32 off, __u32 val);
#endif /* CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL */
};

struct virtio_pci_modern_caps {
	struct virtio_pci_cap common;
	struct virtio_pci_notify_cap notify;
	struct virtio_pci_cap isr;
	struct virtio_pci_cap device;
	__u8 has_common;
	__u8 has_notify;
	__u8 has_isr;
	__u8 has_device;
};

struct virtio_pci_dev {
	struct virtio_dev vdev;
	enum virtio_pci_transport transport;
	__u64 pci_base_addr;
	__u64 pci_isr_addr;
	struct pci_device *pdev;
	struct virtio_pci_region common_cfg;
	struct virtio_pci_region notify_cfg;
	struct virtio_pci_region isr_cfg;
	struct virtio_pci_region device_cfg;
	__u32 notify_off_multiplier;
	__u64 *notify_addrs;
	__u16 notify_addrs_count;
	__u8 features_written;
	__u8 irq_registered;
};

static inline struct virtio_pci_dev *to_virtiopcidev(struct virtio_dev *vdev)
{
	return __containerof(vdev, struct virtio_pci_dev, vdev);
}

typedef int (*virtio_pci_modern_add_func_t)(struct pci_device *,
					    struct virtio_pci_dev *);

VPCI_TEST_EXTERN struct virtio_config_ops vpci_legacy_ops;
VPCI_TEST_EXTERN struct virtio_config_ops vpci_modern_ops;

VPCI_TEST_VISIBLE void
vpci_region_write64(const struct virtio_pci_region *r, __u32 off, __u64 val);
VPCI_TEST_VISIBLE int virtio_pci_device_id_supported(__u16 pci_device_id);
VPCI_TEST_VISIBLE int virtio_pci_modern_virtio_id(__u16 pci_device_id,
						  __u16 *virtio_device_id);
VPCI_TEST_VISIBLE int vpci_intx_routing_valid(__u8 pin, unsigned long irq);
VPCI_TEST_VISIBLE int
virtio_pci_modern_notify_addr(struct virtio_pci_dev *vpdev,
			      __u16 queue_notify_off, __u64 *addr);
VPCI_TEST_VISIBLE __u64 vpci_modern_features_get(struct virtio_dev *vdev);
VPCI_TEST_VISIBLE void vpci_modern_features_set(struct virtio_dev *vdev);
VPCI_TEST_VISIBLE int vpci_modern_vq_find(struct virtio_dev *vdev,
					  __u16 num_vqs,
					  __u16 *qdesc_size);
VPCI_TEST_VISIBLE struct virtqueue *
vpci_modern_vq_setup(struct virtio_dev *vdev, __u16 queue_id,
		     __u16 num_desc, virtqueue_callback_t callback,
		     struct uk_alloc *a);
VPCI_TEST_VISIBLE int vpci_modern_config_get(struct virtio_dev *vdev,
					     __u16 offset, void *buf,
					     __u32 len, __u8 type_len);
VPCI_TEST_VISIBLE int vpci_modern_config_set(struct virtio_dev *vdev,
					     __u16 offset, const void *buf,
					     __u32 len);
VPCI_TEST_VISIBLE int
virtio_pci_modern_parse_caps_raw(const __u8 cfg[256],
				 const struct pci_bar bars[PCI_BAR_COUNT],
				 struct virtio_pci_modern_caps *caps);
VPCI_TEST_VISIBLE int
virtio_pci_modern_init_dev(struct pci_device *pci_dev,
			   struct virtio_pci_dev *vpci_dev,
			   const struct virtio_pci_modern_caps *caps);
VPCI_TEST_VISIBLE int
virtio_pci_probe_transport(struct pci_device *pci_dev,
			   struct virtio_pci_dev *vpci_dev,
			   virtio_pci_modern_add_func_t modern_add);

#endif /* __VIRTIO_PCI_PRIVATE_H__ */
