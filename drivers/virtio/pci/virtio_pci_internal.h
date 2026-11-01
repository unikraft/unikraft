/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __VIRTIO_PCI_INTERNAL_H__
#define __VIRTIO_PCI_INTERNAL_H__

#include <uk/arch/types.h>

struct virtio_dev;
struct pci_device;
struct uk_alloc;

int virtio_pci_irq_dispatch(struct virtio_dev *vdev, __u8 status);
#if CONFIG_LIBVIRTIO_PCI_MODERN
int virtio_pci_modern_add_dev(struct pci_device *pdev, struct uk_alloc *a);
#endif

#endif
