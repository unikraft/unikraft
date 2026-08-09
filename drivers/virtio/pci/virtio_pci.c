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

#include <uk/config.h>
#include <uk/arch/types.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/arch.h>
#include <uk/arch/time.h>
#include <uk/bitops.h>
#include <uk/print.h>
#include <uk/lcpu.h>
#include <uk/plat/time.h>
#include <uk/intctlr.h>
#include <uk/intctlr/limits.h>
#include <uk/bus/pci.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_pci.h>
#include "virtio_pci_private.h"

static struct uk_alloc *a;

#define VPCI_MODERN_RESET_TIMEOUT_NSEC ukarch_time_msec_to_nsec(100)

/**
 * Static function declaration.
 */
static int vpci_legacy_pci_dev_reset(struct virtio_dev *vdev);
static int vpci_legacy_pci_config_set(struct virtio_dev *vdev, __u16 offset,
				      const void *buf, __u32 len);
static int vpci_legacy_pci_config_get(struct virtio_dev *vdev, __u16 offset,
				      void *buf, __u32 len, __u8 type_len);
static __u64 vpci_legacy_pci_features_get(struct virtio_dev *vdev);
static void vpci_legacy_pci_features_set(struct virtio_dev *vdev);
static int vpci_legacy_pci_vq_find(struct virtio_dev *vdev, __u16 num_vq,
				   __u16 *qdesc_size);
static void vpci_legacy_pci_status_set(struct virtio_dev *vdev, __u8 status);
static __u8 vpci_legacy_pci_status_get(struct virtio_dev *vdev);
static struct virtqueue *vpci_legacy_vq_setup(struct virtio_dev *vdev,
					      __u16 queue_id,
					      __u16 num_desc,
					      virtqueue_callback_t callback,
					      struct uk_alloc *a);
static void vpci_legacy_vq_release(struct virtio_dev *vdev,
		struct virtqueue *vq, struct uk_alloc *a);
static int virtio_pci_handle(void *arg);
static int vpci_legacy_notify(struct virtio_dev *vdev, __u16 queue_id);
static int virtio_pci_legacy_add_dev(struct pci_device *pci_dev,
				     struct virtio_pci_dev *vpci_dev);
static int virtio_pci_modern_add_dev(struct pci_device *pci_dev,
				     struct virtio_pci_dev *vpci_dev);
static int virtio_pci_modern_parse_caps(struct pci_device *pci_dev,
					struct virtio_pci_modern_caps *caps);
static int vpci_modern_dev_reset(struct virtio_dev *vdev);
static void vpci_modern_status_set(struct virtio_dev *vdev, __u8 status);
static __u8 vpci_modern_status_get(struct virtio_dev *vdev);
static void vpci_modern_vq_release(struct virtio_dev *vdev,
				   struct virtqueue *vq,
				   struct uk_alloc *a);

/**
 * Configuration operations legacy PCI device.
 */
VPCI_TEST_VISIBLE struct virtio_config_ops vpci_legacy_ops = {
	.device_reset = vpci_legacy_pci_dev_reset,
	.config_get   = vpci_legacy_pci_config_get,
	.config_set   = vpci_legacy_pci_config_set,
	.features_get = vpci_legacy_pci_features_get,
	.features_set = vpci_legacy_pci_features_set,
	.status_get   = vpci_legacy_pci_status_get,
	.status_set   = vpci_legacy_pci_status_set,
	.vqs_find     = vpci_legacy_pci_vq_find,
	.vq_setup     = vpci_legacy_vq_setup,
	.vq_release   = vpci_legacy_vq_release,
};

VPCI_TEST_VISIBLE struct virtio_config_ops vpci_modern_ops = {
	.device_reset = vpci_modern_dev_reset,
	.config_get   = vpci_modern_config_get,
	.config_set   = vpci_modern_config_set,
	.features_get = vpci_modern_features_get,
	.features_set = vpci_modern_features_set,
	.status_get   = vpci_modern_status_get,
	.status_set   = vpci_modern_status_set,
	.vqs_find     = vpci_modern_vq_find,
	.vq_setup     = vpci_modern_vq_setup,
	.vq_release   = vpci_modern_vq_release,
};

static __u8 vpci_region_read8(const struct virtio_pci_region *r, __u32 off)
{
	void *addr = (void *)(unsigned long)(r->base + off);

#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
	if (r->test_read8)
		return r->test_read8(r->test_cookie, off);
#endif /* CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL */
	if (r->type == PCI_BAR_IO)
		return virtio_cread8(addr, 0);
	return virtio_mmio_cread8(addr, 0);
}

static __u16 vpci_region_read16(const struct virtio_pci_region *r, __u32 off)
{
	void *addr = (void *)(unsigned long)(r->base + off);

#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
	if (r->test_read16)
		return r->test_read16(r->test_cookie, off);
#endif /* CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL */
	if (r->type == PCI_BAR_IO)
		return virtio_cread16(addr, 0);
	return virtio_mmio_cread16(addr, 0);
}

static __u32 vpci_region_read32(const struct virtio_pci_region *r, __u32 off)
{
	void *addr = (void *)(unsigned long)(r->base + off);

#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
	if (r->test_read32)
		return r->test_read32(r->test_cookie, off);
#endif /* CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL */
	if (r->type == PCI_BAR_IO)
		return virtio_cread32(addr, 0);
	return virtio_mmio_cread32(addr, 0);
}

static void vpci_region_write8(const struct virtio_pci_region *r,
			       __u32 off, __u8 val)
{
	void *addr = (void *)(unsigned long)(r->base + off);

#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
	if (r->test_write8) {
		r->test_write8(r->test_cookie, off, val);
		return;
	}
#endif /* CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL */
	if (r->type == PCI_BAR_IO)
		virtio_cwrite8(addr, 0, val);
	else
		virtio_mmio_cwrite8(addr, 0, val);
}

static void vpci_region_write16(const struct virtio_pci_region *r,
				__u32 off, __u16 val)
{
	void *addr = (void *)(unsigned long)(r->base + off);

#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
	if (r->test_write16) {
		r->test_write16(r->test_cookie, off, val);
		return;
	}
#endif /* CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL */
	if (r->type == PCI_BAR_IO)
		virtio_cwrite16(addr, 0, val);
	else
		virtio_mmio_cwrite16(addr, 0, val);
}

static void vpci_region_write32(const struct virtio_pci_region *r,
				__u32 off, __u32 val)
{
	void *addr = (void *)(unsigned long)(r->base + off);

#if CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL
	if (r->test_write32) {
		r->test_write32(r->test_cookie, off, val);
		return;
	}
#endif /* CONFIG_LIBVIRTIO_PCI_TEST || CONFIG_LIBUKTEST_ALL */
	if (r->type == PCI_BAR_IO)
		virtio_cwrite32(addr, 0, val);
	else
		virtio_mmio_cwrite32(addr, 0, val);
}

VPCI_TEST_VISIBLE void vpci_region_write64(const struct virtio_pci_region *r,
					   __u32 off, __u64 val)
{
	vpci_region_write32(r, off, (__u32)val);
	vpci_region_write32(r, off + sizeof(__u32), (__u32)(val >> 32));
}

static int vpci_region_rw_valid(const struct virtio_pci_region *r,
				__u32 off, __u32 len)
{
	return off <= r->length && len <= r->length - off;
}

VPCI_TEST_VISIBLE int virtio_pci_device_id_supported(__u16 pci_device_id)
{
	return (pci_device_id >= VIRTIO_PCI_LEGACY_DEVICEID_START &&
		pci_device_id <= VIRTIO_PCI_LEGACY_DEVICEID_END) ||
	       (pci_device_id > VIRTIO_PCI_MODERN_DEVICEID_START &&
		pci_device_id <= VIRTIO_PCI_MODERN_DEVICEID_END);
}

VPCI_TEST_VISIBLE int virtio_pci_modern_virtio_id(__u16 pci_device_id,
						  __u16 *virtio_device_id)
{
	__u16 id;

	if (pci_device_id < VIRTIO_PCI_MODERN_DEVICEID_START ||
	    pci_device_id > VIRTIO_PCI_MODERN_DEVICEID_END)
		return -EINVAL;

	id = pci_device_id - VIRTIO_PCI_MODERN_DEVICEID_START;
	if (!id)
		return -EINVAL;
	*virtio_device_id = id;
	return 0;
}

VPCI_TEST_VISIBLE int
virtio_pci_modern_notify_addr(struct virtio_pci_dev *vpdev,
			      __u16 queue_notify_off, __u64 *addr)
{
	__u64 notify_off;
	__u64 notify_addr;

	notify_off = (__u64)queue_notify_off *
		     vpdev->notify_off_multiplier;
	if (notify_off > vpdev->notify_cfg.length ||
	    sizeof(__u16) > vpdev->notify_cfg.length - notify_off ||
	    vpdev->notify_cfg.base > __U64_MAX - notify_off)
		return -EINVAL;
	notify_addr = vpdev->notify_cfg.base + notify_off;
	if (notify_addr & (sizeof(__u16) - 1))
		return -EINVAL;

	*addr = notify_addr;
	return 0;
}

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
	__u8 isr_status;
	struct virtqueue *vq;
	long flags;
	int rc;

	UK_ASSERT(arg);

	/* Reading the isr status is used to acknowledge the interrupt */
	if (d->transport == VIRTIO_PCI_TRANSPORT_MODERN) {
		isr_status = vpci_region_read8(&d->isr_cfg, 0);
		uk_pr_debug_isr("Modern virtio PCI ISR status %#x\n",
				isr_status);
	} else {
		isr_status = virtio_cread8((void *)d->pci_isr_addr, 0);
	}
	if (!isr_status)
		return 0;
	rc = 1;

	if (isr_status & VIRTIO_PCI_ISR_CONFIG) {
		/* We don't support configuration interrupt on the device */
		uk_pr_warn_isr("Unsupported config change interrupt received on virtio-pci device %p\n",
			       d);
	}

	if (isr_status & VIRTIO_PCI_ISR_HAS_INTR) {
		flags = uk_lcpu_save_irqf();
		UK_TAILQ_FOREACH(vq, &d->vdev.vqs, next)
			virtqueue_ring_interrupt(vq);
		uk_lcpu_restore_irqf(flags);
	}

	return rc;
}

VPCI_TEST_VISIBLE int vpci_intx_routing_valid(__u8 pin, unsigned long irq)
{
	return (!pin || pin > 4 || irq > UK_INTCTLR_MAX_IRQ) ? -EINVAL : 0;
}

static int vpci_intx_valid(struct virtio_pci_dev *vpdev)
{
	__u8 pin;

	UK_ASSERT(vpdev);
	UK_ASSERT(vpdev->pdev);

	pin = pci_config_read8(vpdev->pdev, PCI_INTERRUPT_PIN);
	if (vpci_intx_routing_valid(pin, vpdev->pdev->irq)) {
		uk_pr_err("Invalid virtio PCI INTx routing: pin %" __PRIu8
			  " irq %lu\n", pin, vpdev->pdev->irq);
		return -EINVAL;
	}

	return 0;
}

static struct virtqueue *vpci_legacy_vq_setup(struct virtio_dev *vdev,
					      __u16 queue_id,
					      __u16 num_desc,
					      virtqueue_callback_t callback,
					      struct uk_alloc *a)
{
	struct virtio_pci_dev *vpdev = NULL;
	struct virtqueue *vq;
	__paddr_t addr;
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

	flags = uk_lcpu_save_irqf();
	UK_TAILQ_INSERT_TAIL(&vpdev->vdev.vqs, vq, next);
	uk_lcpu_restore_irqf(flags);

err_exit:
	return vq;
}

static void vpci_legacy_vq_release(struct virtio_dev *vdev,
		struct virtqueue *vq, struct uk_alloc *a)
{
	struct virtio_pci_dev *vpdev = NULL;
	long flags;

	UK_ASSERT(vq != NULL);
	UK_ASSERT(a != NULL);
	vpdev = to_virtiopcidev(vdev);

	/* Select and deactivate the queue */
	virtio_cwrite16((void *)(unsigned long)vpdev->pci_base_addr,
			VIRTIO_PCI_QUEUE_SEL, vq->queue_id);
	virtio_cwrite32((void *)(unsigned long)vpdev->pci_base_addr,
			VIRTIO_PCI_QUEUE_PFN, 0);

	flags = uk_lcpu_save_irqf();
	UK_TAILQ_REMOVE(&vpdev->vdev.vqs, vq, next);
	uk_lcpu_restore_irqf(flags);

	virtqueue_destroy(VIRTIO_PCI_VRING_ALIGN, vq, a);
}

static int vpci_legacy_pci_vq_find(struct virtio_dev *vdev, __u16 num_vqs,
				   __u16 *qdesc_size)
{
	struct virtio_pci_dev *vpdev = NULL;
	int vq_cnt = 0, i = 0, rc = 0;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

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

	if (!vpdev->irq_registered && vq_cnt) {
		rc = vpci_intx_valid(vpdev);
		if (unlikely(rc))
			return rc;
		rc = uk_intctlr_irq_register(vpdev->pdev->irq,
					     virtio_pci_handle, vpdev);
		if (rc != 0) {
			uk_pr_err("Failed to register the interrupt\n");
			return rc;
		}
		vpdev->irq_registered = 1;
		pci_device_intx(vpdev->pdev, 1);
	}
	return vq_cnt;
}

static int vpci_legacy_pci_config_set(struct virtio_dev *vdev, __u16 offset,
				      const void *buf, __u32 len)
{
	struct virtio_pci_dev *vpdev = NULL;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	virtio_cwrite_bytes((void *)(unsigned long)vpdev->pci_base_addr,
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
		virtio_cread_bytes(
				(void *) (unsigned long)vpdev->pci_base_addr,
				VIRTIO_PCI_CONFIG_OFF + offset, buf, len,
				type_len);
	} else {
		__u32 len_bytes;

		if (__builtin_umul_overflow(len, type_len, &len_bytes))
			return -EFAULT;

		rc = virtio_cread_bytes_many(
				(void *) (unsigned long)vpdev->pci_base_addr,
				VIRTIO_PCI_CONFIG_OFF + offset,	buf, len_bytes);
		if (unlikely(rc != (int) len_bytes))
			return -EFAULT;
	}

	return 0;
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

static int vpci_legacy_pci_dev_reset(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev = NULL;
	__nsec deadline;
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
	deadline = ukplat_monotonic_clock() + VPCI_MODERN_RESET_TIMEOUT_NSEC;
	do {
		status = virtio_cread8((void *)(unsigned long)
					      vpdev->pci_base_addr,
					      VIRTIO_PCI_STATUS);
		if (status == VIRTIO_CONFIG_STATUS_RESET)
			return 0;
		uk_arch_spinwait();
	} while (ukplat_monotonic_clock() < deadline);

	uk_pr_err("Timed out waiting for legacy virtio PCI reset\n");
	return -ETIMEDOUT;
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

static void vpci_legacy_pci_features_set(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev = NULL;

	UK_ASSERT(vdev);

	vpdev = to_virtiopcidev(vdev);

	/* Mask out features not supported by the virtqueue driver */
	vdev->features = virtqueue_feature_negotiate(vdev->features);

	virtio_cwrite32((void *) (unsigned long)vpdev->pci_base_addr,
			VIRTIO_PCI_GUEST_FEATURES, (__u32)vdev->features);
}

static __u8 vpci_modern_status_get(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	return vpci_region_read8(&vpdev->common_cfg,
				 VPCI_CFG_OFF(device_status));
}

static void vpci_modern_status_set(struct virtio_dev *vdev, __u8 status)
{
	struct virtio_pci_dev *vpdev;
	__u8 curr_status;

	UK_ASSERT(vdev);

	if (status == VIRTIO_CONFIG_STATUS_RESET) {
		if (unlikely(vpci_modern_dev_reset(vdev)))
			uk_pr_err("Modern virtio PCI reset failed\n");
		return;
	}

	vpdev = to_virtiopcidev(vdev);
	if ((status & VIRTIO_CONFIG_STATUS_FEATURES_OK) &&
	    !vpdev->features_written) {
		uk_pr_err("Refusing FEATURES_OK before modern virtio PCI features\n");
		return;
	}
	curr_status = vpci_modern_status_get(vdev);
	status |= curr_status;
	vpci_region_write8(&vpdev->common_cfg,
			   VPCI_CFG_OFF(device_status), status);
	uk_pr_debug("Modern virtio PCI status %#x\n", status);

	if ((status & VIRTIO_CONFIG_STATUS_FEATURES_OK) &&
	    !(vpci_modern_status_get(vdev) &
	      VIRTIO_CONFIG_STATUS_FEATURES_OK)) {
		uk_pr_err("Modern virtio PCI device rejected FEATURES_OK\n");
	}
}

static int vpci_modern_dev_reset(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev;
	__nsec deadline;
	unsigned int i;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	vpci_region_write8(&vpdev->common_cfg,
			   VPCI_CFG_OFF(device_status),
			   VIRTIO_CONFIG_STATUS_RESET);

	for (i = 0; i < MAX_TRY_COUNT; i++) {
		if (vpci_modern_status_get(vdev) ==
		    VIRTIO_CONFIG_STATUS_RESET) {
			vpdev->features_written = 0;
			return 0;
		}
	}

	deadline = ukplat_monotonic_clock() + VPCI_MODERN_RESET_TIMEOUT_NSEC;
	while (ukplat_monotonic_clock() < deadline) {
		uk_arch_spinwait();
		if (vpci_modern_status_get(vdev) ==
		    VIRTIO_CONFIG_STATUS_RESET) {
			vpdev->features_written = 0;
			return 0;
		}
	}

	uk_pr_err("Timed out waiting for modern virtio PCI reset\n");
	return -ETIMEDOUT;
}

VPCI_TEST_VISIBLE __u64 vpci_modern_features_get(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev;
	const __u32 feature_off = VPCI_CFG_OFF(device_feature);
	__u64 features;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	vpci_region_write32(&vpdev->common_cfg,
			    VPCI_CFG_OFF(device_feature_select),
			    0);
	features = vpci_region_read32(&vpdev->common_cfg, feature_off);
	vpci_region_write32(&vpdev->common_cfg,
			    VPCI_CFG_OFF(device_feature_select),
			    1);
	features |= ((__u64)vpci_region_read32(&vpdev->common_cfg,
						feature_off)
		     << 32);

	return features;
}

VPCI_TEST_VISIBLE void vpci_modern_features_set(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev;
	__u64 device_features;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);
	vpdev->features_written = 0;

	vdev->features = virtqueue_feature_negotiate(vdev->features);
	if (!(vdev->features & UK_BIT_ULL(VIRTIO_F_VERSION_1))) {
		uk_pr_err("Modern virtio PCI requires VIRTIO_F_VERSION_1\n");
		return;
	}
	device_features = vpci_modern_features_get(vdev);
	if (vdev->features & ~device_features) {
		uk_pr_err("Modern virtio PCI driver selected unsupported features %#"
			  __PRIx64 "\n", vdev->features & ~device_features);
		return;
	}

	vpci_region_write32(&vpdev->common_cfg,
			    VPCI_CFG_OFF(driver_feature_select),
			    0);
	vpci_region_write32(&vpdev->common_cfg,
			    VPCI_CFG_OFF(driver_feature),
			    (__u32)vdev->features);
	vpci_region_write32(&vpdev->common_cfg,
			    VPCI_CFG_OFF(driver_feature_select),
			    1);
	vpci_region_write32(&vpdev->common_cfg,
			    VPCI_CFG_OFF(driver_feature),
			    (__u32)(vdev->features >> 32));
	vpdev->features_written = 1;
}

static int vpci_modern_features_ready(struct virtio_dev *vdev)
{
	struct virtio_pci_dev *vpdev = to_virtiopcidev(vdev);

	if (!vpdev->features_written ||
	    !(vpci_modern_status_get(vdev) &
	      VIRTIO_CONFIG_STATUS_FEATURES_OK)) {
		uk_pr_err("Modern virtio PCI feature negotiation is incomplete\n");
		return -EINVAL;
	}

	return 0;
}

static int vpci_modern_config_get64(struct virtio_pci_dev *vpdev,
				    __u16 offset, void *buf)
{
	__u8 gen_before, gen_after;
	__u32 lo, hi;
	__u64 val;
	int retry;

	for (retry = 0; retry < MAX_TRY_COUNT; retry++) {
		gen_before = vpci_region_read8(&vpdev->common_cfg,
					       VPCI_CFG_OFF(config_generation));
		lo = vpci_region_read32(&vpdev->device_cfg, offset);
		hi = vpci_region_read32(&vpdev->device_cfg,
					offset + sizeof(__u32));
		gen_after = vpci_region_read8(&vpdev->common_cfg,
					      VPCI_CFG_OFF(config_generation));
		if (gen_before == gen_after) {
			val = ((__u64)hi << 32) | lo;
			memcpy(buf, &val, sizeof(val));
			return 0;
		}
	}

	return -EFAULT;
}

VPCI_TEST_VISIBLE int vpci_modern_config_get(struct virtio_dev *vdev,
					     __u16 offset, void *buf,
					     __u32 len,
					     __u8 type_len __unused)
{
	struct virtio_pci_dev *vpdev;
	__u8 *ptr = buf;
	__u8 gen_before, gen_after;
	__u32 val32;
	__u16 val16;
	__u32 i;
	int retry;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	if (!vpdev->device_cfg.length)
		return -ENODEV;
	if (!vpci_region_rw_valid(&vpdev->device_cfg, offset, len))
		return -EINVAL;

	switch (len) {
	case 1:
		*(__u8 *)buf = vpci_region_read8(&vpdev->device_cfg, offset);
		return 0;
	case 2:
		val16 = vpci_region_read16(&vpdev->device_cfg, offset);
		memcpy(buf, &val16, sizeof(val16));
		return 0;
	case 4:
		val32 = vpci_region_read32(&vpdev->device_cfg, offset);
		memcpy(buf, &val32, sizeof(val32));
		return 0;
	case 8:
		return vpci_modern_config_get64(vpdev, offset, buf);
	default:
		break;
	}

	for (retry = 0; retry < MAX_TRY_COUNT; retry++) {
		gen_before = vpci_region_read8(&vpdev->common_cfg,
					       VPCI_CFG_OFF(config_generation));
		for (i = 0; i < len; i++)
			ptr[i] = vpci_region_read8(&vpdev->device_cfg,
						   offset + i);
		gen_after = vpci_region_read8(&vpdev->common_cfg,
					      VPCI_CFG_OFF(config_generation));
		if (gen_before == gen_after)
			return 0;
	}

	return -EFAULT;
}

VPCI_TEST_VISIBLE int vpci_modern_config_set(struct virtio_dev *vdev,
					     __u16 offset, const void *buf,
					     __u32 len)
{
	struct virtio_pci_dev *vpdev;
	const __u8 *ptr = buf;
	__u32 val32;
	__u16 val16;
	__u64 val64;
	__u32 i;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	if (!vpdev->device_cfg.length)
		return -ENODEV;
	if (!vpci_region_rw_valid(&vpdev->device_cfg, offset, len))
		return -EINVAL;

	switch (len) {
	case 1:
		vpci_region_write8(&vpdev->device_cfg, offset, *ptr);
		break;
	case 2:
		memcpy(&val16, buf, sizeof(val16));
		vpci_region_write16(&vpdev->device_cfg, offset, val16);
		break;
	case 4:
		memcpy(&val32, buf, sizeof(val32));
		vpci_region_write32(&vpdev->device_cfg, offset, val32);
		break;
	case 8:
		memcpy(&val64, buf, sizeof(val64));
		vpci_region_write64(&vpdev->device_cfg, offset, val64);
		break;
	default:
		for (i = 0; i < len; i++)
			vpci_region_write8(&vpdev->device_cfg,
					   offset + i, ptr[i]);
	}

	return 0;
}

static int vpci_modern_notify(struct virtio_dev *vdev, __u16 queue_id)
{
	struct virtio_pci_dev *vpdev;
	struct virtio_pci_region notify;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);

	if (queue_id >= vpdev->notify_addrs_count ||
	    !vpdev->notify_addrs[queue_id])
		return -EINVAL;

	notify = vpdev->notify_cfg;
	notify.base = vpdev->notify_addrs[queue_id];
	vpci_region_write16(&notify, 0, queue_id);
	uk_pr_debug_isr("Modern virtio PCI notify queue %" __PRIu16
			" addr 0x%" __PRIx64 "\n",
			queue_id, notify.base);

	return 0;
}

VPCI_TEST_VISIBLE int vpci_modern_vq_find(struct virtio_dev *vdev,
					  __u16 num_vqs,
					  __u16 *qdesc_size)
{
	struct virtio_pci_dev *vpdev;
	__u16 num_queues, notify_off;
	__u64 notify_addr;
	__u64 *notify_addrs;
	int i, rc, vq_cnt = 0;

	UK_ASSERT(vdev);
	UK_ASSERT(qdesc_size);
	vpdev = to_virtiopcidev(vdev);

	rc = vpci_modern_features_ready(vdev);
	if (unlikely(rc))
		return rc;
	if (!num_vqs)
		return 0;
	if (!UK_TAILQ_EMPTY(&vdev->vqs))
		return -EBUSY;

	notify_addrs = uk_calloc(a, num_vqs, sizeof(*notify_addrs));
	if (!notify_addrs)
		return -ENOMEM;

	num_queues = vpci_region_read16(&vpdev->common_cfg,
					VPCI_CFG_OFF(num_queues));
	for (i = 0; i < num_vqs; i++) {
		if (i >= num_queues) {
			qdesc_size[i] = 0;
			continue;
		}

		vpci_region_write16(&vpdev->common_cfg,
				    VPCI_CFG_OFF(queue_select),
				    i);
		qdesc_size[i] = vpci_region_read16(&vpdev->common_cfg,
						   VPCI_CFG_OFF(queue_size));
		if (unlikely(!qdesc_size[i])) {
			uk_pr_err("Virtqueue %d not available\n", i);
			continue;
		}

		notify_off = vpci_region_read16(&vpdev->common_cfg,
						VPCI_CFG_OFF(queue_notify_off));
		rc = virtio_pci_modern_notify_addr(vpdev, notify_off,
						   &notify_addr);
		if (unlikely(rc))
			goto err_free_notify;

		notify_addrs[i] = notify_addr;
		vq_cnt++;
	}

	if (!vpdev->irq_registered && vq_cnt) {
		rc = vpci_intx_valid(vpdev);
		if (unlikely(rc))
			goto err_free_notify;
		rc = uk_intctlr_irq_register(vpdev->pdev->irq,
					     virtio_pci_handle, vpdev);
		if (rc != 0) {
			uk_pr_err("Failed to register the interrupt\n");
			goto err_free_notify;
		}
		vpdev->irq_registered = 1;
		pci_device_intx(vpdev->pdev, 1);
	}

	uk_free(a, vpdev->notify_addrs);
	vpdev->notify_addrs = notify_addrs;
	vpdev->notify_addrs_count = num_vqs;

	return vq_cnt;

err_free_notify:
	uk_free(a, notify_addrs);
	return rc;
}

static int vpci_power_of_2(__u16 val)
{
	return val && !(val & (val - 1));
}

VPCI_TEST_VISIBLE struct virtqueue *
vpci_modern_vq_setup(struct virtio_dev *vdev, __u16 queue_id,
		     __u16 num_desc, virtqueue_callback_t callback,
		     struct uk_alloc *a)
{
	struct virtio_pci_dev *vpdev;
	struct virtqueue *vq;
	__u16 queue_size, notify_off;
	__u64 desc_addr, driver_addr, device_addr, notify_addr;
	long flags;
	int rc;

	UK_ASSERT(vdev);
	vpdev = to_virtiopcidev(vdev);
	rc = vpci_modern_features_ready(vdev);
	if (unlikely(rc))
		goto err_exit;
	if (queue_id >= vpdev->notify_addrs_count ||
	    queue_id >= vpci_region_read16(&vpdev->common_cfg,
					     VPCI_CFG_OFF(num_queues))) {
		rc = -EINVAL;
		goto err_exit;
	}

	vpci_region_write16(&vpdev->common_cfg,
			    VPCI_CFG_OFF(queue_select),
			    queue_id);
	if (vpci_region_read16(&vpdev->common_cfg,
			       VPCI_CFG_OFF(queue_enable))) {
		rc = -EBUSY;
		goto err_exit;
	}

	queue_size = vpci_region_read16(&vpdev->common_cfg,
					VPCI_CFG_OFF(queue_size));
	if (!vpci_power_of_2(queue_size) || !vpci_power_of_2(num_desc) ||
	    num_desc > queue_size) {
		rc = -EINVAL;
		goto err_exit;
	}

	notify_off = vpci_region_read16(&vpdev->common_cfg,
					VPCI_CFG_OFF(queue_notify_off));
	rc = virtio_pci_modern_notify_addr(vpdev, notify_off, &notify_addr);
	if (unlikely(rc))
		goto err_exit;

	vq = virtqueue_create(queue_id, num_desc, VIRTIO_PCI_VRING_ALIGN,
			      callback, vpci_modern_notify, vdev, a);
	if (PTRISERR(vq)) {
		uk_pr_err("Failed to create the virtqueue: %d\n",
			  PTR2ERR(vq));
		return vq;
	}

	vpci_region_write16(&vpdev->common_cfg,
			    VPCI_CFG_OFF(queue_size),
			    num_desc);
	vpci_region_write16(&vpdev->common_cfg,
			    VPCI_CFG_OFF(queue_msix_vector),
			    VIRTIO_MSI_NO_VECTOR);

	desc_addr = virtqueue_physaddr(vq);
	vpci_region_write64(&vpdev->common_cfg,
			    VPCI_CFG_OFF(queue_desc),
			    desc_addr);
	driver_addr = virtqueue_get_avail_addr(vq);
	vpci_region_write64(&vpdev->common_cfg,
			    VPCI_CFG_OFF(queue_driver),
			    driver_addr);
	device_addr = virtqueue_get_used_addr(vq);
	vpci_region_write64(&vpdev->common_cfg,
			    VPCI_CFG_OFF(queue_device),
			    device_addr);

	vpdev->notify_addrs[queue_id] = notify_addr;

	vpci_region_write16(&vpdev->common_cfg,
			    VPCI_CFG_OFF(queue_enable),
			    1);
	uk_pr_debug("Modern virtio PCI queue %" __PRIu16
		    ": size %" __PRIu16
		    " desc 0x%" __PRIx64
		    " driver 0x%" __PRIx64
		    " device 0x%" __PRIx64
		    " notify 0x%" __PRIx64 "\n",
		    queue_id, num_desc, desc_addr, driver_addr, device_addr,
		    notify_addr);

	flags = uk_lcpu_save_irqf();
	UK_TAILQ_INSERT_TAIL(&vpdev->vdev.vqs, vq, next);
	uk_lcpu_restore_irqf(flags);

	return vq;

err_exit:
	return ERR2PTR(rc);
}

static void vpci_modern_vq_release(struct virtio_dev *vdev,
				   struct virtqueue *vq,
				   struct uk_alloc *a)
{
	struct virtio_pci_dev *vpdev;
	long flags;

	UK_ASSERT(vdev);
	UK_ASSERT(vq);
	UK_ASSERT(a);

	vpdev = to_virtiopcidev(vdev);
	if (unlikely(vpci_modern_dev_reset(vdev))) {
		uk_pr_err("Refusing to release live modern virtio PCI queue %"
			  __PRIu16 "\n", vq->queue_id);
		return;
	}

	flags = uk_lcpu_save_irqf();
	UK_TAILQ_REMOVE(&vpdev->vdev.vqs, vq, next);
	uk_lcpu_restore_irqf(flags);

	if (vq->queue_id < vpdev->notify_addrs_count)
		vpdev->notify_addrs[vq->queue_id] = 0;
	virtqueue_destroy(VIRTIO_PCI_VRING_ALIGN, vq, a);
}

static __u16 virtio_pci_cfg_read16(const __u8 *cfg, __u8 pos)
{
	return ((__u16)cfg[pos]) | ((__u16)cfg[pos + 1] << 8);
}

static __u32 virtio_pci_cfg_read32(const __u8 *cfg, __u8 pos)
{
	return ((__u32)cfg[pos]) |
	       ((__u32)cfg[pos + 1] << 8) |
	       ((__u32)cfg[pos + 2] << 16) |
	       ((__u32)cfg[pos + 3] << 24);
}

static void virtio_pci_cap_read(const __u8 *cfg, __u8 pos,
				struct virtio_pci_cap *cap)
{
	cap->cap_vndr = cfg[pos];
	cap->cap_next = cfg[pos + 1];
	cap->cap_len = cfg[pos + 2];
	cap->cfg_type = cfg[pos + 3];
	cap->bar = cfg[pos + 4];
	cap->id = cfg[pos + 5];
	cap->padding[0] = cfg[pos + 6];
	cap->padding[1] = cfg[pos + 7];
	cap->offset = virtio_pci_cfg_read32(cfg, pos + 8);
	cap->length = virtio_pci_cfg_read32(cfg, pos + 12);
}

static int virtio_pci_cap_valid(const struct pci_bar bars[PCI_BAR_COUNT],
				const struct virtio_pci_cap *cap)
{
	const struct pci_bar *bar;

	if (cap->bar >= PCI_BAR_COUNT)
		return -EINVAL;

	bar = &bars[cap->bar];
	if (bar->type == PCI_BAR_NONE || !bar->size)
		return -EINVAL;
	if (cap->offset > bar->size || cap->length > bar->size - cap->offset)
		return -EINVAL;

	return 0;
}

VPCI_TEST_VISIBLE int
virtio_pci_modern_parse_caps_raw(const __u8 cfg[256],
				 const struct pci_bar bars[PCI_BAR_COUNT],
				 struct virtio_pci_modern_caps *caps)
{
	__u8 pos, next, visited[256];
	struct virtio_pci_cap cap;
	__u32 notify_multiplier;
	int malformed = 0;
	int rc;

	UK_ASSERT(caps);

	memset(caps, 0, sizeof(*caps));
	memset(visited, 0, sizeof(visited));

	if (!(virtio_pci_cfg_read16(cfg, PCI_STATUS) & PCI_STATUS_CAP_LIST))
		return -ENODEV;

	pos = cfg[PCI_CAPABILITIES_PTR];
	if (!pos)
		return -ENODEV;

	while (pos) {
		if (pos < 0x40 || pos > 0xfc || (pos & 0x3))
			return -EINVAL;
		if (visited[pos])
			return -EINVAL;
		visited[pos] = 1;

		next = cfg[pos + 1];
		if (cfg[pos] != VIRTIO_PCI_CAP_VENDOR) {
			pos = next;
			continue;
		}

		if ((__u16)pos + sizeof(struct virtio_pci_cap) > 256 ||
		    (__u16)pos + cfg[pos + 2] > 256)
			return -EINVAL;
		if (cfg[pos + 2] < sizeof(struct virtio_pci_cap)) {
			malformed = 1;
			pos = next;
			continue;
		}

		virtio_pci_cap_read(cfg, pos, &cap);
		if (cap.bar >= PCI_BAR_COUNT) {
			pos = next;
			continue;
		}
		switch (cap.cfg_type) {
		case VIRTIO_PCI_CAP_COMMON_CFG:
			if (caps->has_common)
				break;
			rc = virtio_pci_cap_valid(bars, &cap);
			if (unlikely(rc) ||
			    cap.length < VPCI_COMMON_CFG_MIN_LEN ||
			    (cap.offset & (sizeof(__u32) - 1))) {
				malformed = 1;
				break;
			}
			caps->common = cap;
			caps->has_common = 1;
			break;
		case VIRTIO_PCI_CAP_NOTIFY_CFG:
			if (caps->has_notify)
				break;
			if (cap.cap_len <
			    sizeof(struct virtio_pci_notify_cap)) {
				malformed = 1;
				break;
			}
			rc = virtio_pci_cap_valid(bars, &cap);
			notify_multiplier =
				virtio_pci_cfg_read32(cfg, pos + 16);
			if (unlikely(rc) || cap.length < sizeof(__u16) ||
			    (cap.offset & (sizeof(__u16) - 1))) {
				malformed = 1;
				break;
			}
			caps->notify.cap = cap;
			caps->notify.notify_off_multiplier = notify_multiplier;
			caps->has_notify = 1;
			break;
		case VIRTIO_PCI_CAP_ISR_CFG:
			if (caps->has_isr)
				break;
			rc = virtio_pci_cap_valid(bars, &cap);
			if (unlikely(rc) || cap.length < sizeof(__u8)) {
				malformed = 1;
				break;
			}
			caps->isr = cap;
			caps->has_isr = 1;
			break;
		case VIRTIO_PCI_CAP_DEVICE_CFG:
			if (caps->has_device)
				break;
			rc = virtio_pci_cap_valid(bars, &cap);
			if (unlikely(rc) || !cap.length ||
			    (cap.offset & (sizeof(__u32) - 1)))
				break;
			caps->device = cap;
			caps->has_device = 1;
			break;
		default:
			break;
		}

		pos = next;
	}

	if (!caps->has_common || !caps->has_notify || !caps->has_isr)
		return malformed ? -EINVAL : -ENODEV;

	return 0;
}

static int virtio_pci_modern_parse_caps(struct pci_device *pci_dev,
					struct virtio_pci_modern_caps *caps)
{
	__u8 cfg[256];
	unsigned int i;

	UK_ASSERT(pci_dev);
	UK_ASSERT(caps);

	for (i = 0; i < sizeof(cfg); i++)
		cfg[i] = pci_config_read8(pci_dev, i);

	return virtio_pci_modern_parse_caps_raw(cfg, pci_dev->bar, caps);
}

static int virtio_pci_region_init(struct virtio_pci_region *region,
				  struct pci_device *pci_dev,
				  const struct virtio_pci_cap *cap)
{
	struct pci_bar *bar;
	int rc;

	UK_ASSERT(region);
	UK_ASSERT(pci_dev);
	UK_ASSERT(cap);

	rc = pci_device_map_bar(pci_dev, cap->bar);
	if (unlikely(rc))
		return rc;

	bar = &pci_dev->bar[cap->bar];
	region->type = bar->type;
	region->bar = cap->bar;
	region->base = bar->vbase + cap->offset;
	region->length = cap->length;

	return 0;
}

VPCI_TEST_VISIBLE int
virtio_pci_modern_init_dev(struct pci_device *pci_dev,
			   struct virtio_pci_dev *vpci_dev,
			   const struct virtio_pci_modern_caps *caps)
{
	__u16 virtio_id;
	int rc;

	UK_ASSERT(pci_dev);
	UK_ASSERT(vpci_dev);
	UK_ASSERT(caps);

	if (caps->common.length < VPCI_COMMON_CFG_MIN_LEN ||
	    caps->notify.cap.length < sizeof(__u16) ||
	    caps->isr.length < sizeof(__u8)) {
		uk_pr_err("Invalid modern virtio PCI capability length\n");
		return -EINVAL;
	}
	if (caps->common.offset & (sizeof(__u32) - 1)) {
		uk_pr_err("Unaligned modern virtio PCI common capability\n");
		return -EINVAL;
	}
	if (caps->notify.cap.offset & (sizeof(__u16) - 1)) {
		uk_pr_err("Unaligned modern virtio PCI notify capability\n");
		return -EINVAL;
	}
	if (caps->has_device &&
	    (caps->device.offset & (sizeof(__u32) - 1))) {
		uk_pr_err("Unaligned modern virtio PCI device capability\n");
		return -EINVAL;
	}

	rc = virtio_pci_region_init(&vpci_dev->common_cfg, pci_dev,
				    &caps->common);
	if (unlikely(rc))
		return rc;
	rc = virtio_pci_region_init(&vpci_dev->notify_cfg, pci_dev,
				    &caps->notify.cap);
	if (unlikely(rc))
		return rc;
	rc = virtio_pci_region_init(&vpci_dev->isr_cfg, pci_dev, &caps->isr);
	if (unlikely(rc))
		return rc;
	if (caps->has_device) {
		rc = virtio_pci_region_init(&vpci_dev->device_cfg, pci_dev,
					    &caps->device);
		if (unlikely(rc))
			return rc;
	}

	if (pci_dev->id.device_id >= VIRTIO_PCI_MODERN_DEVICEID_START) {
		rc = virtio_pci_modern_virtio_id(pci_dev->id.device_id,
						 &virtio_id);
		if (unlikely(rc))
			return rc;
	} else {
		virtio_id = pci_dev->id.subsystem_device_id;
	}

	vpci_dev->transport = VIRTIO_PCI_TRANSPORT_MODERN;
	vpci_dev->notify_off_multiplier = caps->notify.notify_off_multiplier;
	vpci_dev->vdev.cops = &vpci_modern_ops;
	vpci_dev->vdev.id.virtio_device_id = virtio_id;

	uk_pr_info("Added modern virtio-pci device %04x (id %" __PRIu16 ")\n",
		   pci_dev->id.device_id, virtio_id);

	return 0;
}

static int virtio_pci_modern_add_dev(struct pci_device *pci_dev,
				     struct virtio_pci_dev *vpci_dev)
{
	struct virtio_pci_modern_caps caps;
	int rc;

	rc = virtio_pci_modern_parse_caps(pci_dev, &caps);
	if (unlikely(rc))
		return rc;

	return virtio_pci_modern_init_dev(pci_dev, vpci_dev, &caps);
}

static int virtio_pci_legacy_bar_valid(struct pci_device *pci_dev)
{
	if (!pci_dev->base)
		return -EINVAL;
#if CONFIG_ARCH_X86_64
	if (pci_dev->bar[0].type != PCI_BAR_NONE &&
	    pci_dev->bar[0].type != PCI_BAR_IO)
		return -EINVAL;
#endif /* CONFIG_ARCH_X86_64 */

	return 0;
}

static int virtio_pci_legacy_add_dev(struct pci_device *pci_dev,
				     struct virtio_pci_dev *vpci_dev)
{
	int rc;

	/* Check the valid range of the virtio legacy device */
	if (pci_dev->id.device_id < VIRTIO_PCI_LEGACY_DEVICEID_START ||
	    pci_dev->id.device_id > VIRTIO_PCI_LEGACY_DEVICEID_END) {
		uk_pr_err("Invalid Virtio Devices %04x\n",
			  pci_dev->id.device_id);
		return -EINVAL;
	}
	rc = virtio_pci_legacy_bar_valid(pci_dev);
	if (unlikely(rc)) {
		uk_pr_err("Invalid legacy virtio PCI BAR0\n");
		return rc;
	}

	vpci_dev->transport = VIRTIO_PCI_TRANSPORT_LEGACY;
	vpci_dev->pci_isr_addr = vpci_dev->pci_base_addr + VIRTIO_PCI_ISR;

	/* Setting the configuration operation */
	vpci_dev->vdev.cops = &vpci_legacy_ops;

	uk_pr_info("Added virtio-pci device %04x\n",
		   pci_dev->id.device_id);
	uk_pr_info("Added virtio-pci subsystem_device_id %04x\n",
		   pci_dev->id.subsystem_device_id);

	/* Mapping the virtio device identifier */
	vpci_dev->vdev.id.virtio_device_id = pci_dev->id.subsystem_device_id;
	return 0;
}

VPCI_TEST_VISIBLE int
virtio_pci_probe_transport(struct pci_device *pci_dev,
			   struct virtio_pci_dev *vpci_dev,
			   virtio_pci_modern_add_func_t modern_add)
{
	int modern_rc = -ENODEV, rc = 0;

	UK_ASSERT(pci_dev);
	UK_ASSERT(vpci_dev);
	UK_ASSERT(modern_add);

	if (pci_dev->id.device_id >= VIRTIO_PCI_MODERN_DEVICEID_START &&
	    pci_dev->id.device_id <= VIRTIO_PCI_MODERN_DEVICEID_END) {
#if CONFIG_ARCH_X86_64
		rc = modern_add(pci_dev, vpci_dev);
		if (rc != 0) {
			uk_pr_err("Failed to probe modern virtio-pci device: %d\n",
				  rc);
		}
#else /* !CONFIG_ARCH_X86_64 */
		uk_pr_err("Modern virtio-pci is not supported on this architecture\n");
		rc = -ENOTSUP;
#endif /* !CONFIG_ARCH_X86_64 */
	} else if (pci_dev->id.device_id >= VIRTIO_PCI_LEGACY_DEVICEID_START &&
		   pci_dev->id.device_id <= VIRTIO_PCI_LEGACY_DEVICEID_END) {
#if CONFIG_ARCH_X86_64
		modern_rc = modern_add(pci_dev, vpci_dev);
		if (modern_rc != 0) {
			if (modern_rc != -ENODEV && modern_rc != -EINVAL)
				return modern_rc;

			memset(&vpci_dev->common_cfg, 0,
			       sizeof(vpci_dev->common_cfg));
			memset(&vpci_dev->notify_cfg, 0,
			       sizeof(vpci_dev->notify_cfg));
			memset(&vpci_dev->isr_cfg, 0,
			       sizeof(vpci_dev->isr_cfg));
			memset(&vpci_dev->device_cfg, 0,
			       sizeof(vpci_dev->device_cfg));
			vpci_dev->notify_off_multiplier = 0;

			rc = virtio_pci_legacy_add_dev(pci_dev, vpci_dev);
			if (rc != 0) {
				uk_pr_err("Failed to probe legacy virtio-pci device: %d (modern: %d)\n",
					  rc, modern_rc);
			}
		}
#else /* !CONFIG_ARCH_X86_64 */
		rc = virtio_pci_legacy_add_dev(pci_dev, vpci_dev);
#endif /* !CONFIG_ARCH_X86_64 */
	} else {
		rc = -EINVAL;
	}

	return rc;
}

static int virtio_pci_add_dev(struct pci_device *pci_dev)
{
	struct virtio_pci_dev *vpci_dev = NULL;
	int rc = 0;

	UK_ASSERT(pci_dev != NULL);

	if (pci_dev->id.vendor_id != VIRTIO_PCI_VENDOR_ID)
		return -EINVAL;
	if (!virtio_pci_device_id_supported(pci_dev->id.device_id)) {
		uk_pr_err("Unsupported virtio-pci device ID %04x\n",
			  pci_dev->id.device_id);
		return -EINVAL;
	}

	vpci_dev = uk_malloc(a, sizeof(*vpci_dev));
	if (!vpci_dev) {
		uk_pr_err("Failed to allocate virtio-pci device\n");
		return -ENOMEM;
	}
	memset(vpci_dev, 0, sizeof(*vpci_dev));

	/* Fetch PCI Device information */
	vpci_dev->pdev = pci_dev;
	vpci_dev->pci_base_addr = pci_dev->base;

	rc = virtio_pci_probe_transport(pci_dev, vpci_dev,
					virtio_pci_modern_add_dev);
	if (rc != 0) {
		uk_pr_err("Failed to probe virtio-pci device: %d\n", rc);
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
	if (vpci_dev->irq_registered) {
		pci_device_intx(vpci_dev->pdev, 0);
		uk_intctlr_irq_unregister_arg(vpci_dev->pdev->irq,
					      virtio_pci_handle, vpci_dev);
	}
	uk_free(a, vpci_dev->notify_addrs);
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
	{PCI_DEVICE_ID(VIRTIO_PCI_VENDOR_ID, PCI_ANY_ID)},
	/* End of Driver List */
	{PCI_ANY_DEVICE_ID},
};

static struct pci_driver virtio_pci_drv = {
	.device_ids = virtio_pci_ids,
	.init = virtio_pci_drv_init,
	.add_dev = virtio_pci_add_dev
};
PCI_REGISTER_DRIVER(&virtio_pci_drv);
