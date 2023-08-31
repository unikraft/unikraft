/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Jia He <justin.he@arm.com>
 *
 * Copyright (c) 2020, Arm Ltd. All rights reserved.
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
#include <string.h>
#include <uk/alloc.h>
#include <uk/print.h>
#include <uk/plat/lcpu.h>
#include <uk/intctlr.h>
#include <uk/bus.h>
#include <uk/bitops.h>
#include <uk/plat/common/bootinfo.h>

#include <uk/bus/platform.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_mmio.h>

#if CONFIG_LIBVIRTIO_MMIO_FDT
#include <uk/ofw/fdt.h>
#include <libfdt.h>
#endif /* CONFIG_LIBVIRTIO_MMIO_FDT */

/*
 * The alignment to use between consumer and producer parts of vring.
 * Currently hardcoded to the page size.
 */
#define VIRTIO_MMIO_VRING_ALIGN		__PAGE_SIZE

static struct uk_alloc *a;
struct virtio_mmio_device_id {
	uint16_t device_id;
	uint32_t vendor;
};

struct virtio_mmio_device {
	struct virtio_dev vdev;
	char *name;
	struct virtio_mmio_device_id  id;
	struct virtio_mmio_driver     *drv;
	unsigned long version;
	unsigned long irq;
	void *base;
	struct pf_device *pfdev;
};

#define to_virtio_mmio_device(_dev) \
	__containerof(_dev, struct virtio_mmio_device, vdev)

struct virtio_mmio_vq_info {
	/* the actual virtqueue */
	struct virtqueue *vq;
};
typedef void vq_callback_t(struct virtqueue *);

/* Configuration interface */

static __u64 vm_get_features(struct virtio_dev *vdev)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
	__u64 features = 0;

	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
	features = virtio_mmio_cread32(vm_dev->base,
				       VIRTIO_MMIO_DEVICE_FEATURES);
	features <<= 32;

	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
	features |= virtio_mmio_cread32(vm_dev->base,
					VIRTIO_MMIO_DEVICE_FEATURES);

	return features;
}

static void vm_set_features(struct virtio_dev *vdev)
{
	struct virtio_mmio_device *vm_dev;

	UK_ASSERT(vdev);

	vm_dev = to_virtio_mmio_device(vdev);

	/* Give virtio_ring a chance to accept features. */
	vdev->features = virtqueue_feature_negotiate(vdev->features);

	/* Make sure there are no mixed devices */
	if (vm_dev->version == 2 &&
	    !uk_test_bit(VIRTIO_F_VERSION_1, &vdev->features)) {
		uk_pr_err("Modern virtio devices must set VIRTIO_F_VERSION_1\n");
		return;
	}

	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_DRIVER_FEATURES,
			     (__u32)(vdev->features >> 32));

	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_DRIVER_FEATURES,
			     (__u32)vdev->features);
}

static int vm_get(struct virtio_dev *vdev, __u16 offset,
		   void *buf, __u32 len, __u8 type_len __unused)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
	void *base = vm_dev->base + VIRTIO_MMIO_CONFIG;
	__u8 b;
	__u16 w;
	__u32 l;

	if (vm_dev->version == 1) {
		__u8 *ptr = buf;
		__u8 i;

		for (i = 0; i < len; i++)
			ptr[i] = virtio_mmio_cread8(base, offset + i);
		return 0;
	}

	switch (len) {
	case 1:
		b = virtio_mmio_cread8(base, offset);
		memcpy(buf, &b, sizeof(b));
		break;
	case 2:
		w = (virtio_mmio_cread16(base, offset));
		memcpy(buf, &w, sizeof(w));
		break;
	case 4:
		l = (virtio_mmio_cread32(base, offset));
		memcpy(buf, &l, sizeof(l));
		break;
	case 8:
		l = (virtio_mmio_cread32(base, offset));
		memcpy(buf, &l, sizeof(l));
		l = (virtio_mmio_cread32(base, offset + sizeof(l)));
		memcpy(buf + sizeof(l), &l, sizeof(l));
		break;
	default:
		virtio_mmio_cread_bytes(base, offset, buf, len, 1);
		uk_pr_warn("Unaligned io read: %d bytes\n", len);
	}

	return 0;
}

static int vm_set(struct virtio_dev *vdev, __u16 offset,
		   const void *buf, __u32 len)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
	void *base = vm_dev->base + VIRTIO_MMIO_CONFIG;
	__u8 b;
	__u16 w;
	__u32 l;

	if (vm_dev->version == 1) {
		const __u8 *ptr = buf;
		__u32 i;

		for (i = 0; i < len; i++)
			virtio_mmio_cwrite8(base, offset + i, ptr[i]);
		return 0;
	}

	switch (len) {
	case 1:
		memcpy(&b, buf, sizeof(b));
		virtio_mmio_cwrite8(base, offset, b);
		break;
	case 2:
		memcpy(&w, buf, sizeof(w));
		virtio_mmio_cwrite16(base, offset, w);
		break;
	case 4:
		memcpy(&l, buf, sizeof(l));
		virtio_mmio_cwrite32(base, offset, l);
		break;
	case 8:
		memcpy(&l, buf, sizeof(l));
		virtio_mmio_cwrite32(base, offset, l);
		memcpy(&l, buf + sizeof(l), sizeof(l));
		virtio_mmio_cwrite32(base, offset + sizeof(l), l);
		break;
	default:
		virtio_mmio_cwrite_bytes(base, offset, buf, len, 1);
		uk_pr_warn("Unaligned io write: %d bytes\n", len);
	}

	return 0;
}

static __u8 vm_get_status(struct virtio_dev *vdev)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

	return virtio_mmio_cread32(vm_dev->base, VIRTIO_MMIO_STATUS) & 0xff;
}

static void vm_set_status(struct virtio_dev *vdev, __u8 status)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

	/* We should never be setting status to 0. */
	UK_BUGON(status == 0);

	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_STATUS, status);
}

static void vm_reset(struct virtio_dev *vdev)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

	/* 0 status means a reset. */
	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_STATUS, 0);
}

/* Transport interface */

/* the notify function used when creating a virt queue */
static int vm_notify(struct virtio_dev *vdev, __u16 queue_id)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);

	/*
	 * We write the queue's selector into the notification register to
	 * signal the other end
	 */
	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_NOTIFY, queue_id);
	return 1;
}

/* Notify all virtqueues on an interrupt. */
static int vm_interrupt(void *opaque)
{
	struct virtio_mmio_device *vm_dev = opaque;
	unsigned long status;
	unsigned long flags;
	int rc = 0;
	struct virtqueue *vq;

	/* Read and acknowledge interrupts */
	status = virtio_mmio_cread32(vm_dev->base,
				     VIRTIO_MMIO_INTERRUPT_STATUS);

	/* It is possible that more than one device shares the same
	 * interrupt line. Because of that, do not acknowledge the
	 * interrupt unless one of the status bits is set, as writing
	 * zero to the ACK register instructs the device to reset.
	 */
	if (likely(status))
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_INTERRUPT_ACK,
				     status);

	if (unlikely(status & VIRTIO_MMIO_INT_CONFIG)) {
		uk_pr_warn("Unsupported config change interrupt received on virtio-mmio device %p\n",
			   vm_dev);
	}

	if (likely(status & VIRTIO_MMIO_INT_VRING)) {
		flags = ukplat_lcpu_save_irqf();
		UK_TAILQ_FOREACH(vq, &vm_dev->vdev.vqs, next) {
			rc |= virtqueue_ring_interrupt(vq);
		}
		ukplat_lcpu_restore_irqf(flags);

		/* If this is a virtio interrupt, then it MUST
		 * be handled by one of the drivers.
		 */
		UK_ASSERT(rc);
	}

	return rc;
}


static struct virtqueue *vm_setup_vq(struct virtio_dev *vdev,
					__u16 queue_id,
					__u16 num_desc,
					virtqueue_callback_t callback,
					struct uk_alloc *a)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
	struct virtqueue *vq;
	unsigned long flags;

	/* Create the vring */
	vq = virtqueue_create(queue_id, num_desc, VIRTIO_MMIO_VRING_ALIGN,
			      callback, vm_notify, vdev, a);

	if (PTRISERR(vq)) {
		uk_pr_err("Failed to create the virtqueue: %d\n",
			  PTR2ERR(vq));

		goto err_exit;
	}

	/* Select the queue we're interested in */
	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_SEL, queue_id);

	/* Activate the queue */
	virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_NUM, num_desc);
	if (vm_dev->version == 1) {
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_ALIGN,
				     __PAGE_SIZE);
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_PFN,
				     virtqueue_physaddr(vq) >> __PAGE_SHIFT);
	} else {
		__u64 addr;

		addr = virtqueue_physaddr(vq);
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_DESC_LOW,
				     (__u32)addr);
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_DESC_HIGH,
				     (__u32)(addr >> 32));

		addr = virtqueue_get_avail_addr(vq);
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_AVAIL_LOW,
				     (__u32)addr);
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_AVAIL_HIGH,
				     (__u32)(addr >> 32));

		addr =  virtqueue_get_used_addr(vq);
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_USED_LOW,
				     (__u32)addr);
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_USED_HIGH,
				     (__u32)(addr >> 32));

		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_READY, 1);
	}

	flags = ukplat_lcpu_save_irqf();
	UK_TAILQ_INSERT_TAIL(&vm_dev->vdev.vqs, vq, next);
	ukplat_lcpu_restore_irqf(flags);

err_exit:
	return vq;
}

static int vm_find_vqs(struct virtio_dev *vdev, __u16 num_vqs, __u16 *qdesc_size)
{
	struct virtio_mmio_device *vm_dev = to_virtio_mmio_device(vdev);
	unsigned int irq = vm_dev->pfdev->irq;
	int i, err;
	int vq_cnt = 0;

	err = uk_intctlr_irq_register(irq, vm_interrupt, vm_dev);
	if (err)
		return err;

	for (i = 0; i < num_vqs; ++i) {
		/* Select the queue we're interested in */
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_QUEUE_SEL, i);

		/* Queue shouldn't already be set up. */
		if (virtio_mmio_cread32(vm_dev->base, (vm_dev->version == 1 ?
				VIRTIO_MMIO_QUEUE_PFN : VIRTIO_MMIO_QUEUE_READY))) {
			uk_pr_err("vm_find_vqs error mmio queue not ready\n");
			err = -ENOENT;
			goto error_exit;
		}

		qdesc_size[i] = virtio_mmio_cread32(vm_dev->base,
						    VIRTIO_MMIO_QUEUE_NUM_MAX);
		if (qdesc_size[i] == 0) {
			err = -ENOENT;
			goto error_exit;
		}

		vq_cnt++;
	}

	return vq_cnt;
error_exit:
	uk_pr_err("err in vm_find_vqs :%d\n", err);
	return err;
}

static struct virtio_config_ops virtio_mmio_config_ops = {
	.config_get	= vm_get,
	.config_set	= vm_set,
	.status_get	= vm_get_status,
	.status_set	= vm_set_status,
	.device_reset	= vm_reset,
	.features_get	= vm_get_features,
	.features_set	= vm_set_features,
	.vqs_find	= vm_find_vqs,
	.vq_setup	= vm_setup_vq,
};

#if CONFIG_LIBVIRTIO_MMIO_FDT
static int virtio_mmio_probe_fdt(struct pf_device *pfdev)
{
	int rc;
	const fdt32_t *prop;
	int prop_len, offs;
	__u64 reg_base, reg_size;
	void *dtb;
	struct uk_intctlr_irq irq;

	dtb = (void *)ukplat_bootinfo_get()->dtb;
	offs = pfdev->fdt_offset;

	if (unlikely(offs < 0))
		return -EINVAL;

	rc = uk_intctlr_irq_fdt_xlat(dtb, offs, 0, &irq);
	if (unlikely(rc < 0))
		return -EINVAL;

	uk_intctlr_irq_configure(&irq);

	prop = fdt_getprop(dtb, offs, "reg", &prop_len);
	if (unlikely(!prop))
		return -EINVAL;

	fdt_get_address(dtb, offs, 0, &reg_base, &reg_size);

	pfdev->base = reg_base;
	pfdev->size = reg_size;
	pfdev->irq = irq.id;

	uk_pr_info("virtio mmio probe base(0x%lx) irq(%ld)\n",
				pfdev->base, pfdev->irq);

	return 0;
}
#endif /* CONFIG_LIBVIRTIO_MMIO_FDT */

static int virtio_mmio_probe(struct pf_device __maybe_unused *pfdev)
{
	int rc = -ENODEV;

#if CONFIG_LIBVIRTIO_MMIO_FDT
	rc = virtio_mmio_probe_fdt(pfdev);
#endif /* CONFIG_LIBVIRTIO_MMIO_FDT */

	return rc;
}

int virtio_mmio_add_dev(struct pf_device *pfdev)
{
	struct virtio_mmio_device *vm_dev;
	unsigned int magic;
	int rc;

	UK_ASSERT(pfdev != NULL);

#if CONFIG_PAGING
	pfdev->base = uk_bus_pf_devmap(pfdev->base, pfdev->size);
	if (unlikely(PTRISERR(pfdev->base))) {
		uk_pr_err("Could not map the device (%d)\n",
			  PTR2ERR(pfdev->base));
		return PTR2ERR(pfdev->base);
	}
#endif /* CONFIG_PAGING */

	vm_dev = uk_malloc(a, sizeof(*vm_dev));
	if (!vm_dev) {
		uk_pr_err("Failed to allocate virtio-mmio device\n");
		rc = -ENOMEM;
		goto free_vmdev;
	}

	/* Fetch Pf Device information */
	vm_dev->pfdev = pfdev;
	vm_dev->base = (void *)pfdev->base;
	vm_dev->vdev.cops = &virtio_mmio_config_ops;
	vm_dev->name = "virtio_mmio";

	if (vm_dev->base == NULL) {
		rc = -EFAULT;
		goto free_vmdev;
	}

	magic = virtio_mmio_cread32(vm_dev->base, VIRTIO_MMIO_MAGIC_VALUE);
	if (magic != ('v' | 'i' << 8 | 'r' << 16 | 't' << 24)) {
		uk_pr_err("Wrong magic value 0x%x!\n", magic);
		rc = -ENODEV;
		goto free_vmdev;
	}

	/* Check device version */
	vm_dev->version = virtio_mmio_cread32(vm_dev->base,
					      VIRTIO_MMIO_VERSION);
	if (vm_dev->version < 1 || vm_dev->version > 2) {
		uk_pr_err("Version %ld not supported!\n", vm_dev->version);
		rc = -ENXIO;
		goto free_vmdev;
	}

	vm_dev->vdev.id.virtio_device_id = virtio_mmio_cread32(vm_dev->base,
							       VIRTIO_MMIO_DEVICE_ID);
	if (vm_dev->vdev.id.virtio_device_id == 0) {
		/*
		 * virtio-mmio device with an ID 0 is a (dummy) placeholder
		 * with no function. End probing now with no error reported.
		 */
		uk_pr_info("virtio_mmio device is dummy\n");

		rc = -ENODEV;
		goto free_vmdev;
	}
	vm_dev->id.vendor = virtio_mmio_cread32(vm_dev->base,
						VIRTIO_MMIO_VENDOR_ID);

	if (vm_dev->version <= 1)
		virtio_mmio_cwrite32(vm_dev->base, VIRTIO_MMIO_GUEST_PAGE_SIZE,
				     __PAGE_SIZE);

	rc = virtio_bus_register_device(&vm_dev->vdev);
	if (rc != 0) {
		uk_pr_err("Failed to register the virtio device: %d\n", rc);
		goto free_vmdev;
	}

	uk_pr_info("finish probing a virtio mmio dev\n");

	return 0;

free_vmdev:
	return rc;
}

static int virtio_mmio_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;

	return 0;
}

static const struct device_match_table virtio_mmio_match_table[];

static int virtio_mmio_id_match_compatible(const char *compatible)
{
	int i;

	for (i = 0; virtio_mmio_match_table[i].compatible != NULL; i++)
		if (strcmp(virtio_mmio_match_table[i].compatible, compatible) == 0)
			return virtio_mmio_match_table[i].id->device_id;

	return -1;
}

static struct pf_device_id virtio_mmio_ids = {
		.device_id = VIRTIO_MMIO_ID
};

static struct pf_driver virtio_mmio_drv = {
	.device_ids = &virtio_mmio_ids,
	.init = virtio_mmio_drv_init,
	.probe = virtio_mmio_probe,
	.add_dev = virtio_mmio_add_dev,
	.match = virtio_mmio_id_match_compatible
};

static const struct device_match_table virtio_mmio_match_table[] = {
#if CONFIG_LIBVIRTIO_MMIO_FDT
	{ .compatible = "virtio,mmio",
	  .id = &virtio_mmio_ids },
#endif /* CONFIG_LIBVIRTIO_MMIO_FDT */
	{NULL}
};

PF_REGISTER_DRIVER(&virtio_mmio_drv);
