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

#include <uk/config.h>
#include <uk/arch/types.h>
#include <errno.h>
#include <uk/alloc.h>
#include <uk/print.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/irq.h>
#include <pci/pci_bus.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_pci.h>

#define VENDOR_QUMRANET_VIRTIO           (0x1AF4)
#define VIRTIO_PCI_MODERN_DEVICEID_START (0x1040)

static struct uk_alloc *a;

/**
 * The structure declares a pci device.
 */
struct virtio_pci_dev {
	/* Virtio Device */
	struct virtio_dev vdev;
	/* Pci base address */
	__u16 pci_base_addr;
	/* ISR Address Range */
	__u16 pci_isr_addr;
	/* Pci device information */
	struct pci_device *pdev;
};

/**
 * Fetch the virtio pci information from the virtio device.
 * @param vdev
 *	Reference to the virtio device.
 */
#define to_virtiopcidev(vdev) \
		__containerof(vdev, struct virtio_pci_dev, vdev)

/**
 * Static function declaration.
 */
static void vpci_legacy_pci_dev_reset(struct virtio_dev *vdev);
static int vpci_legacy_pci_config_set(struct virtio_dev *vdev, __u16 offset,
				      const void *buf, __u32 len);
static int vpci_legacy_pci_config_get(struct virtio_dev *vdev, __u16 offset,
				      void *buf, __u32 len, __u8 type_len);
static __u64 vpci_legacy_pci_features_get(struct virtio_dev *vdev);
static void vpci_legacy_pci_features_set(struct virtio_dev *vdev,
					 __u64 features);
static int vpci_legacy_pci_vq_find(struct virtio_dev *vdev, __u16 num_vq,
				   __u16 *qdesc_size);
static void vpci_legacy_pci_status_set(struct virtio_dev *vdev, __u8 status);
static __u8 vpci_legacy_pci_status_get(struct virtio_dev *vdev);
static struct virtqueue *vpci_legacy_vq_setup(struct virtio_dev *vdev,
					      __u16 queue_id,
					      __u16 num_desc,
					      virtqueue_callback_t callback,
					      struct uk_alloc *a);
static int virtio_pci_handle(void *arg);
static int vpci_legacy_notify(struct virtio_dev *vdev, __u16 queue_id);
static int virtio_pci_legacy_add_dev(struct pci_device *pci_dev,
				     struct virtio_pci_dev *vpci_dev);

/**
 * Configuration operations legacy PCI device.
 */
static struct virtio_config_ops vpci_legacy_ops = {
	.device_reset = vpci_legacy_pci_dev_reset,
	.config_get   = vpci_legacy_pci_config_get,
	.config_set   = vpci_legacy_pci_config_set,
	.features_get = vpci_legacy_pci_features_get,
	.features_set = vpci_legacy_pci_features_set,
	.status_get   = vpci_legacy_pci_status_get,
	.status_set   = vpci_legacy_pci_status_set,
	.vqs_find     = vpci_legacy_pci_vq_find,
	.vq_setup     = vpci_legacy_vq_setup,
};

static int vpci_legacy_notify(struct virtio_dev *vdev, __u16 queue_id)
{
	struct virtio_pci_dev *vpdev;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);
	virtio_cwrite16((void *)(unsigned long) vpdev->pci_base_addr,
			VIRTIO_PCI_QUEUE_NOTIFY, queue_id);

	return 0;
}

static int virtio_pci_handle(void *arg)
{
	struct virtio_pci_dev *d = (struct virtio_pci_dev *) arg;
	uint8_t isr_status;
	struct virtqueue *vq;
	int rc = 0;

	UK_ASSERT(arg);

	/* Reading the isr status is used to acknowledge the interrupt */
	isr_status = virtio_cread8((void *)(unsigned long)d->pci_isr_addr, 0);
	/* We don't support configuration interrupt on the device */
	if (isr_status & VIRTIO_PCI_ISR_CONFIG) {
		uk_pr_warn("Unsupported config change interrupt received on virtio-pci device %p\n",
			   d);
	}

	if (isr_status & VIRTIO_PCI_ISR_HAS_INTR) {
		UK_TAILQ_FOREACH(vq, &d->vdev.vqs, next) {
			rc |= virtqueue_ring_interrupt(vq);
		}
	}
	return rc;
}

static struct virtqueue *vpci_legacy_vq_setup(struct virtio_dev *vdev,
					      __u16 queue_id,
					      __u16 num_desc,
					      virtqueue_callback_t callback,
					      struct uk_alloc *a)
{
	struct virtio_pci_dev *vpdev = NULL;
	struct virtqueue *vq;
	__phys_addr addr;
	long flags;

	UK_ASSERT(vdev != NULL);

	vpdev = to_virtiopcidev(vdev);
	vq = virtqueue_create(queue_id, num_desc, VIRTIO_PCI_VRING_ALIGN,
			      callback, vpci_legacy_notify, vdev, a);
	if (PTRISERR(vq)) {
		uk_pr_err("Failed to create the virtqueue: %d\n",
			  PTR2ERR(vq));
		goto err_exit;
	}

	/* Physical address of the queue */
	addr = virtqueue_physaddr(vq);
	/* Select the queue of interest */
	virtio_cwrite16((void *)(unsigned long)vpdev->pci_base_addr,
			VIRTIO_PCI_QUEUE_SEL, queue_id);
	virtio_cwrite32((void *)(unsigned long)vpdev->pci_base_addr,
			VIRTIO_PCI_QUEUE_PFN,
			addr >> VIRTIO_PCI_QUEUE_ADDR_SHIFT);

	flags = ukplat_lcpu_save_irqf();
	UK_TAILQ_INSERT_TAIL(&vpdev->vdev.vqs, vq, next);
	ukplat_lcpu_restore_irqf(flags);

err_exit:
	return vq;
}

static int vpci_legacy_pci_vq_find(struct virtio_dev *vdev, __u16 num_vqs,
				   __u16 *qdesc_size)
{
	struct virtio_pci_dev *vpdev = NULL;
	int vq_cnt = 0, i = 0, rc = 0;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	/* Registering the interrupt for the queue */
	rc = ukplat_irq_register(vpdev->pdev->irq, virtio_pci_handle, vpdev);
	if (rc != 0) {
		uk_pr_err("Failed to register the interrupt\n");
		return rc;
	}

	for (i = 0; i < num_vqs; i++) {
		virtio_cwrite16((void *) (unsigned long)vpdev->pci_base_addr,
				VIRTIO_PCI_QUEUE_SEL, i);
		qdesc_size[i] = virtio_cread16(
				(void *) (unsigned long)vpdev->pci_base_addr,
				VIRTIO_PCI_QUEUE_SIZE);
		if (unlikely(!qdesc_size[i])) {
			uk_pr_err("Virtqueue %d not available\n", i);
			continue;
		}
		vq_cnt++;
	}
	return vq_cnt;
}

static int vpci_legacy_pci_config_set(struct virtio_dev *vdev, __u16 offset,
				      const void *buf, __u32 len)
{
	struct virtio_pci_dev *vpdev = NULL;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	_virtio_cwrite_bytes((void *)(unsigned long)vpdev->pci_base_addr,
			     VIRTIO_PCI_CONFIG_OFF + offset, buf, len, 1);

	return 0;
}

static int vpci_legacy_pci_config_get(struct virtio_dev *vdev, __u16 offset,
				      void *buf, __u32 len, __u8 type_len)
{
	struct virtio_pci_dev *vpdev = NULL;
	int rc = 0;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	/* Reading an entity less than 4 bytes are atomic */
	if (type_len == len && type_len <= 4) {
		_virtio_cread_bytes(
				(void *) (unsigned long)vpdev->pci_base_addr,
				VIRTIO_PCI_CONFIG_OFF + offset, buf, len,
				type_len);
	} else {
		rc = virtio_cread_bytes_many(
				(void *) (unsigned long)vpdev->pci_base_addr,
				VIRTIO_PCI_CONFIG_OFF + offset,	buf, len);
	}
	return rc;
}

static __u8 vpci_legacy_pci_status_get(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev = NULL;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);
	return virtio_cread8((void *) (unsigned long) vpdev->pci_base_addr,
			     VIRTIO_PCI_STATUS);
}

static void vpci_legacy_pci_status_set(struct virtio_dev *vdev, __u8 status)
{
	struct virtio_pci_dev *vpdev = NULL;
	__u8 curr_status = 0;

	/* Reset should be performed using the reset interface */
	UK_ASSERT(vdev || status != VIRTIO_CONFIG_STATUS_RESET);

	vpdev = to_virtiopcidev(vdev);
	curr_status = vpci_legacy_pci_status_get(vdev);
	status |= curr_status;
	virtio_cwrite8((void *)(unsigned long) vpdev->pci_base_addr,
		       VIRTIO_PCI_STATUS, status);
}

static void vpci_legacy_pci_dev_reset(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev = NULL;
	__u8 status;

	UK_ASSERT(vdev);

	vpdev = to_virtiopcidev(vdev);
	/**
	 * Resetting the device.
	 */
	virtio_cwrite8((void *) (unsigned long)vpdev->pci_base_addr,
		       VIRTIO_PCI_STATUS, VIRTIO_CONFIG_STATUS_RESET);
	/**
	 * Waiting for the resetting the device. Find a better way
	 * of doing this instead of repeating register read.
	 *
	 * NOTE! Spec (4.1.4.3.2)
	 * Need to check if we have to wait for the reset to happen.
	 */
	do {
		status = virtio_cread8(
				(void *)(unsigned long)vpdev->pci_base_addr,
				VIRTIO_PCI_STATUS);
	} while (status != VIRTIO_CONFIG_STATUS_RESET);
}

static __u64 vpci_legacy_pci_features_get(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev = NULL;
	__u64  features;

	UK_ASSERT(vdev);

	vpdev = to_virtiopcidev(vdev);
	features = virtio_cread32((void *) (unsigned long)vpdev->pci_base_addr,
				  VIRTIO_PCI_HOST_FEATURES);
	return features;
}

static void vpci_legacy_pci_features_set(struct virtio_dev *vdev,
					 __u64 features)
{
	struct virtio_pci_dev *vpdev = NULL;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);
	/* Mask out features not supported by the virtqueue driver */
	features = virtqueue_feature_negotiate(features);
	virtio_cwrite32((void *) (unsigned long)vpdev->pci_base_addr,
			VIRTIO_PCI_GUEST_FEATURES, (__u32)features);
}

static int virtio_pci_legacy_add_dev(struct pci_device *pci_dev,
				     struct virtio_pci_dev *vpci_dev)
{
	/* Check the valid range of the virtio legacy device */
	if (pci_dev->id.device_id < 0x1000 || pci_dev->id.device_id > 0x103f) {
		uk_pr_err("Invalid Virtio Devices %04x\n",
			  pci_dev->id.device_id);
		return -EINVAL;
	}

	vpci_dev->pci_isr_addr = vpci_dev->pci_base_addr + VIRTIO_PCI_ISR;

	/* Setting the configuration operation */
	vpci_dev->vdev.cops = &vpci_legacy_ops;

	uk_pr_info("Added virtio-pci device %04x\n",
		   pci_dev->id.device_id);

	/* Mapping the virtio device identifier */
	vpci_dev->vdev.id.virtio_device_id = pci_dev->id.subsystem_device_id;
	return 0;
}


static int virtio_pci_add_dev(struct pci_device *pci_dev)
{
	struct virtio_pci_dev *vpci_dev = NULL;
	int rc = 0;

	UK_ASSERT(pci_dev != NULL);

	vpci_dev = uk_malloc(a, sizeof(*vpci_dev));
	if (!vpci_dev) {
		uk_pr_err("Failed to allocate virtio-pci device\n");
		return -ENOMEM;
	}

	/* Fetch PCI Device information */
	vpci_dev->pdev = pci_dev;
	vpci_dev->pci_base_addr = pci_dev->base;

	/**
	 * Probing for the legacy virtio device. We separate the legacy probing
	 * with the possibility of supporting the modern PCI device in the
	 * future.
	 */
	rc = virtio_pci_legacy_add_dev(pci_dev, vpci_dev);
	if (rc != 0) {
		uk_pr_err("Failed to probe (legacy) pci device: %d\n", rc);
		goto free_pci_dev;
	}

	rc = virtio_bus_register_device(&vpci_dev->vdev);
	if (rc != 0) {
		uk_pr_err("Failed to register the virtio device: %d\n", rc);
		goto free_pci_dev;
	}

exit:
	return rc;

free_pci_dev:
	uk_free(a, vpci_dev);
	goto exit;
}

static int virtio_pci_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;
	return 0;
}

static const struct pci_device_id virtio_pci_ids[] = {
	{PCI_DEVICE_ID(VENDOR_QUMRANET_VIRTIO, PCI_ANY_ID)},
	/* End of Driver List */
	{PCI_ANY_DEVICE_ID},
};

static struct pci_driver virtio_pci_drv = {
	.device_ids = virtio_pci_ids,
	.init = virtio_pci_drv_init,
	.add_dev = virtio_pci_add_dev
};
PCI_REGISTER_DRIVER(&virtio_pci_drv);
