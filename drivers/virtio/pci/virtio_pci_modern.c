/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

/* Generic modern PCI transport, VIRTIO 1.2 section 4.1.
 * Uses the existing device-class and split-virtqueue contracts.
 */
#include <errno.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/bus/pci.h>
#include <uk/errptr.h>
#include <uk/intctlr.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/spinlock.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_pci.h>
#include <virtio/virtqueue.h>
#include "virtio_pci_internal.h"

enum {
	COMMON = 1, NOTIFY = 2, ISR = 3, DEVICE = 4,
	C_FEATURE_SELECT = 0, C_FEATURE = 4,
	C_DRIVER_SELECT = 8, C_DRIVER_FEATURE = 12,
	C_MSIX = 16, C_QUEUES = 18, C_STATUS = 20, C_GENERATION = 21,
	C_QUEUE_SELECT = 22, C_QUEUE_SIZE = 24, C_QUEUE_MSIX = 26,
	C_QUEUE_ENABLE = 28, C_QUEUE_NOTIFY = 30,
	C_QUEUE_DESC = 32, C_QUEUE_DRIVER = 40, C_QUEUE_DEVICE = 48,
	COMMON_LENGTH = 56
};

struct modern_region {
	void *base;
	__u32 length;
};

struct modern_queue {
	void *notify;
	struct uk_alloc *alloc;
};

struct modern_pci {
	struct virtio_dev vdev;
	struct pci_device *pdev;
	struct modern_region region[5];
	struct pci_bar_region bars[6];
	void *mapped_bar[6];
	__u32 notify_multiplier;
	__u16 queues;
	struct modern_queue *queue;
	int irq_registered;
	struct uk_spinlock select_lock;
};

#define modern(v) __containerof(v, struct modern_pci, vdev)
#define mmio(base, off) ((char *)(base) + (off))
#define read8(b, o) virtio_mmio_cread8(mmio(b, o), 0)
#define read16(b, o) virtio_mmio_cread16(mmio(b, o), 0)
#define read32(b, o) virtio_mmio_cread32(mmio(b, o), 0)
#define write8(b, o, v) virtio_mmio_cwrite8(mmio(b, o), 0, v)
#define write16(b, o, v) virtio_mmio_cwrite16(mmio(b, o), 0, v)
#define write32(b, o, v) virtio_mmio_cwrite32(mmio(b, o), 0, v)

static void write64(void *base, __u32 off, __u64 value)
{
	write32(base, off, (__u32)value);
	write32(base, off + 4, (__u32)(value >> 32));
}

static int cfg_read(struct modern_pci *d, __u16 offset, __u8 width, __u32 *v)
{
	return pci_cfg_read(&d->pdev->addr, offset, width, v);
}

static int map_capability(struct modern_pci *d, __u32 cap, __u32 kind,
			 __u32 cap_length)
{
	__u32 bar, offset, length, multiplier = 0;
	struct pci_bar_region *r;
	__vaddr_t mapped;
	int rc;

	if (d->region[kind].base)
		return 0; /* First supported instance is preferred. */
	if (cap_length < (kind == NOTIFY ? 20U : 16U) ||
	    cap > 256U - cap_length)
		return -EINVAL;
	if ((rc = cfg_read(d, cap + 4, 1, &bar)))
		return rc;
	if (bar >= 6)
		return 0; /* Reserved BAR values must be ignored. */
	if ((rc = cfg_read(d, cap + 8, 4, &offset)) ||
	    (rc = cfg_read(d, cap + 12, 4, &length)))
		return rc;
	if (!length || (kind == COMMON && (length < COMMON_LENGTH || offset % 4)) ||
	    (kind == NOTIFY && (length < 2 || offset % 2)))
		return -EINVAL;
	if (kind == NOTIFY) {
		if ((rc = cfg_read(d, cap + 16, 4, &multiplier)))
			return rc;
		if (multiplier && (multiplier < 2 || (multiplier & (multiplier - 1))))
			return -EINVAL;
	}
	r = &d->bars[bar];
	if (!r->size) {
		rc = pci_bar_get(d->pdev, bar, r);
		if (rc)
			return rc;
	}
	if (r->io)
		return 0; /* Another capability can offer the MMIO alternative. */
	if (offset > r->size || length > r->size - offset)
		return -EINVAL;
	if (!d->mapped_bar[bar]) {
		mapped = uk_bus_devmap(r->base, r->size);
		if (PTRISERR((void *)mapped))
			return PTR2ERR((void *)mapped);
		d->mapped_bar[bar] = (void *)mapped;
	}
	d->region[kind].base = mmio(d->mapped_bar[bar], offset);
	d->region[kind].length = length;
	if (kind == NOTIFY)
		d->notify_multiplier = multiplier;
	uk_pr_debug("PCI capability type %u BAR %u offset %u length %u\n",
		    kind, bar, offset, length);
	return 0;
}

static int discover(struct modern_pci *d)
{
	__u64 visited[4] = { 0 };
	__u32 status, cap, head, kind, cap_length;
	int rc;

	if ((rc = cfg_read(d, PCI_STATUS_OFFSET, 2, &status)))
		return rc;
	if (!(status & 0x10))
		return -ENOENT;
	if ((rc = cfg_read(d, PCI_CAPABILITIES_PTR, 1, &cap)))
		return rc;
	while (cap) {
		if (cap < 0x40 || cap > 0xfc || cap % 4 ||
		    (visited[cap / 64] & (1ULL << (cap % 64))))
			return -EINVAL;
		visited[cap / 64] |= 1ULL << (cap % 64);
		if ((rc = cfg_read(d, cap, 4, &head)))
			return rc;
		kind = head >> 24;
		cap_length = (head >> 16) & 255;
		if ((head & 255) == 9 && kind >= COMMON && kind <= DEVICE) {
			rc = map_capability(d, cap, kind, cap_length);
			if (rc)
				return rc;
		}
		cap = (head >> 8) & 255;
	}
	if (!d->region[COMMON].base && !d->region[NOTIFY].base &&
	    !d->region[ISR].base)
		return -ENOENT;
	if (!d->region[COMMON].base || !d->region[NOTIFY].base ||
	    !d->region[ISR].base)
		return -EINVAL;
	return 0;
}

static __u64 features_get(struct virtio_dev *vdev)
{
	struct modern_pci *d = modern(vdev);
	void *c = d->region[COMMON].base;
	unsigned long flags;
	__u64 value;

	uk_spin_lock_irqsave(&d->select_lock, flags);
	write32(c, C_FEATURE_SELECT, 1);
	value = (__u64)read32(c, C_FEATURE) << 32;
	write32(c, C_FEATURE_SELECT, 0);
	value |= read32(c, C_FEATURE);
	uk_spin_unlock_irqrestore(&d->select_lock, flags);
	return value;
}

static void features_set(struct virtio_dev *vdev)
{
	struct modern_pci *d = modern(vdev);
	void *c = d->region[COMMON].base;
	unsigned long flags;

	vdev->features = virtqueue_feature_negotiate(vdev->features);
	if (!(vdev->features & (1ULL << VIRTIO_F_VERSION_1)))
		UK_CRASH("Modern PCI requires negotiated VIRTIO_F_VERSION_1\n");
	uk_spin_lock_irqsave(&d->select_lock, flags);
	write32(c, C_DRIVER_SELECT, 1);
	write32(c, C_DRIVER_FEATURE, (__u32)(vdev->features >> 32));
	write32(c, C_DRIVER_SELECT, 0);
	write32(c, C_DRIVER_FEATURE, (__u32)vdev->features);
	uk_spin_unlock_irqrestore(&d->select_lock, flags);
}

static __u8 status_get(struct virtio_dev *vdev)
{
	return read8(modern(vdev)->region[COMMON].base, C_STATUS);
}

static void status_set(struct virtio_dev *vdev, __u8 status)
{
	struct modern_pci *d = modern(vdev);
	unsigned long flags;

	uk_spin_lock_irqsave(&d->select_lock, flags);
	write8(d->region[COMMON].base, C_STATUS, status_get(vdev) | status);
	uk_spin_unlock_irqrestore(&d->select_lock, flags);
}

static void device_reset(struct virtio_dev *vdev)
{
	struct modern_pci *d = modern(vdev);
	__nsec deadline = ukplat_monotonic_clock() + 1000000000ULL;

	write8(d->region[COMMON].base, C_STATUS, 0);
	while (status_get(vdev)) {
		if (ukplat_monotonic_clock() >= deadline)
			UK_CRASH("Modern PCI device reset timed out\n");
	}
	if (d->queue)
		for (unsigned int i = 0; i < d->queues; ++i)
			__atomic_store_n(&d->queue[i].notify, NULL, __ATOMIC_RELEASE);
}

static int config_get(struct virtio_dev *vdev, __u16 offset, void *buf,
		      __u32 len, __u8 type_len)
{
	struct modern_pci *d = modern(vdev);
	void *c = d->region[COMMON].base;
	struct modern_region *r = &d->region[DEVICE];
	__nsec deadline = ukplat_monotonic_clock() + 1000000000ULL;
	__u8 generation;
	__u32 i, value, width = type_len == 8 ? 4 : type_len;

	if (!r->base || offset > r->length || len > r->length - offset)
		return -EFAULT;
	if ((type_len != 1 && type_len != 2 && type_len != 4 && type_len != 8) ||
	    offset % width || len % type_len)
		return -EINVAL;
	do {
		generation = read8(c, C_GENERATION);
		for (i = 0; i < len; i += width) {
			if (width == 1)
				value = read8(r->base, offset + i);
			else if (width == 2)
				value = read16(r->base, offset + i);
			else
				value = read32(r->base, offset + i);
			memcpy((char *)buf + i, &value, width);
		}
		if (generation == read8(c, C_GENERATION))
			return 0;
	} while (ukplat_monotonic_clock() < deadline);
	return -EAGAIN;
}

static int config_set(struct virtio_dev *vdev, __u16 offset, const void *buf,
		      __u32 len)
{
	struct modern_region *r = &modern(vdev)->region[DEVICE];
	__u32 width = len == 8 ? 4 : (len == 1 || len == 2 || len == 4 ? len : 1);
	__u32 value, i;

	if (!r->base || offset > r->length || len > r->length - offset)
		return -EFAULT;
	if (!len || offset % width)
		return -EINVAL;
	for (i = 0; i < len; i += width) {
		value = 0;
		memcpy(&value, (const char *)buf + i, width);
		if (width == 1)
			write8(r->base, offset + i, (__u8)value);
		else if (width == 2)
			write16(r->base, offset + i, (__u16)value);
		else
			write32(r->base, offset + i, value);
	}
	return 0;
}

static int irq_handle(void *arg)
{
	struct modern_pci *d = arg;

	return virtio_pci_irq_dispatch(&d->vdev, read8(d->region[ISR].base, 0));
}

static int notify_host(struct virtio_dev *vdev, __u16 queue)
{
	struct modern_pci *d = modern(vdev);

	void *address;

	if (queue >= d->queues)
		return -EINVAL;
	address = __atomic_load_n(&d->queue[queue].notify, __ATOMIC_ACQUIRE);
	if (!address)
		return -EINVAL;
	write16(address, 0, queue);
	return 0;
}

static int vqs_find(struct virtio_dev *vdev, __u16 count, __u16 *sizes)
{
	struct modern_pci *d = modern(vdev);
	void *c = d->region[COMMON].base;
	unsigned long flags;
	unsigned int i, found = 0;
	int rc;

	if (!d->irq_registered) {
		rc = uk_intctlr_irq_register(d->pdev->irq, irq_handle, d);
		if (rc)
			return rc;
		d->irq_registered = 1;
	}
	uk_spin_lock_irqsave(&d->select_lock, flags);
	write16(c, C_MSIX, 0xffff); /* Use INTx, not an unconfigured MSI-X vector. */
	for (i = 0; i < count; ++i) {
		sizes[i] = 0;
		if (i >= d->queues)
			continue;
		write16(c, C_QUEUE_SELECT, i);
		sizes[i] = read16(c, C_QUEUE_SIZE);
		if (sizes[i])
			++found;
	}
	uk_spin_unlock_irqrestore(&d->select_lock, flags);
	return found;
}

static struct virtqueue *vq_setup(struct virtio_dev *vdev, __u16 queue,
				 __u16 size, virtqueue_callback_t callback,
				 struct uk_alloc *a)
{
	struct modern_pci *d = modern(vdev);
	void *c = d->region[COMMON].base;
	struct virtqueue *vq;
	unsigned long flags;
	__u64 notify_offset;
	int rc = -EINVAL;

	if (queue >= d->queues || !size || (size & (size - 1)))
		return ERR2PTR(-EINVAL);
	vq = virtqueue_create(queue, size, VIRTIO_PCI_VRING_ALIGN,
			      callback, notify_host, vdev, a);
	if (PTRISERR(vq))
		return vq;
	uk_spin_lock_irqsave(&d->select_lock, flags);
	write16(c, C_QUEUE_SELECT, queue);
	if (read16(c, C_QUEUE_ENABLE) || size > read16(c, C_QUEUE_SIZE))
		goto fail;
	notify_offset = (__u64)read16(c, C_QUEUE_NOTIFY) * d->notify_multiplier;
	if (notify_offset > d->region[NOTIFY].length - 2)
		goto fail;
	write16(c, C_QUEUE_SIZE, size);
	write16(c, C_QUEUE_MSIX, 0xffff);
	write64(c, C_QUEUE_DESC, virtqueue_physaddr(vq));
	write64(c, C_QUEUE_DRIVER, virtqueue_get_avail_addr(vq));
	write64(c, C_QUEUE_DEVICE, virtqueue_get_used_addr(vq));
	d->queue[queue].alloc = a;
	__atomic_store_n(&d->queue[queue].notify,
		mmio(d->region[NOTIFY].base, notify_offset), __ATOMIC_RELEASE);
	UK_TAILQ_INSERT_TAIL(&vdev->vqs, vq, next);
	write16(c, C_QUEUE_ENABLE, 1);
	uk_spin_unlock_irqrestore(&d->select_lock, flags);
	uk_pr_debug("Modern PCI queue %u: %u descriptors, notify offset %lu\n",
		    queue, size, (unsigned long)notify_offset);
	return vq;
fail:
	uk_spin_unlock_irqrestore(&d->select_lock, flags);
	virtqueue_destroy(vq, a);
	return ERR2PTR(rc);
}

static void vq_release(struct virtio_dev *vdev, struct virtqueue *vq,
		       struct uk_alloc *a)
{
	struct modern_pci *d = modern(vdev);
	unsigned long flags;

	/* Without negotiated RING_RESET, the specification requires device reset
	 * before releasing queue memory. Writing zero to queue_enable is invalid.
	 * A release therefore quiesces this whole device before freeing any ring.
	 */
	device_reset(vdev);
	vdev->status = VIRTIO_DEV_STOPPED;
	uk_spin_lock_irqsave(&d->select_lock, flags);
	UK_TAILQ_REMOVE(&vdev->vqs, vq, next);
	uk_spin_unlock_irqrestore(&d->select_lock, flags);
	virtqueue_destroy(vq, a);
}

static struct virtio_config_ops modern_ops = {
	.device_reset = device_reset,
	.config_get = config_get, .config_set = config_set,
	.features_get = features_get, .features_set = features_set,
	.status_get = status_get, .status_set = status_set,
	.vqs_find = vqs_find, .vq_setup = vq_setup, .vq_release = vq_release
};

int virtio_pci_modern_add_dev(struct pci_device *pdev, struct uk_alloc *a)
{
    struct modern_pci *d = uk_calloc(a, 1, sizeof(*d));
    struct virtqueue *vq, *next;
    __u32 command, original_command = 0;
    int rc, enabled = 0;

    if (!d)
        return -ENOMEM;
    d->pdev = pdev;
    uk_spin_init(&d->select_lock);
    UK_TAILQ_INIT(&d->vdev.vqs);
    rc = discover(d);
    if (rc)
        goto fail;
    d->vdev.id.virtio_device_id = pdev->id.device_id >= 0x1040 ?
        pdev->id.device_id - 0x1040 : pdev->id.subsystem_device_id;
    d->vdev.cops = &modern_ops;
    if ((rc = cfg_read(d, PCI_COMMAND, 2, &original_command)))
        goto fail;
    command = (original_command | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER) &
              ~PCI_COMMAND_INTX_DISABLE;
    rc = pci_cfg_write(&pdev->addr, PCI_COMMAND, 2, command);
    if (rc)
        goto fail;
    enabled = 1;
    if (!(features_get(&d->vdev) & (1ULL << VIRTIO_F_VERSION_1))) {
        rc = -EOPNOTSUPP;
        goto fail;
    }
    d->queues = read16(d->region[COMMON].base, C_QUEUES);
    if (!d->queues) {
        rc = -ENODEV;
        goto fail;
    }
    d->queue = uk_calloc(a, d->queues, sizeof(*d->queue));
    if (!d->queue) {
        rc = -ENOMEM;
        goto fail;
    }
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
    uk_pr_info("Modern virtio PCI %04x: common=%p notify=%p isr=%p config=%p queues=%u\n",
               pdev->id.device_id, d->region[COMMON].base, d->region[NOTIFY].base,
               d->region[ISR].base, d->region[DEVICE].base, d->queues);
#endif
    rc = virtio_bus_register_device(&d->vdev);
    if (!rc)
        return 0;
fail:
    if (enabled)
        device_reset(&d->vdev);
    if (d->irq_registered)
        uk_intctlr_irq_unregister_arg(pdev->irq, irq_handle, d);
    UK_TAILQ_FOREACH_SAFE(vq, &d->vdev.vqs, next, next) {
        UK_TAILQ_REMOVE(&d->vdev.vqs, vq, next);
        virtqueue_destroy(vq, d->queue[vq->queue_id].alloc);
    }
    if (enabled)
        pci_cfg_write(&pdev->addr, PCI_COMMAND, 2, original_command);
    if (d->queue)
        uk_free(a, d->queue);
    uk_free(a, d);
    return rc;
}
