/* TODO: SPDX Header */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Hugo Lefeuvre <hugo.lefeuvre@neclab.eu>
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
/* Some code was derived from Solo5: */
/*
 * Copyright (c) 2015-2017 Contributors as noted in the AUTHORS file
 *
 * This file is part of Solo5, a unikernel base layer.
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

#include <string.h>

#include <uk/arch/util.h>
#include <uk/spinlock.h>
#include <uk/print.h>
#include <uk/bus/pci.h>
#include "../../pci_internal.h"
#include "pci_scan.h"

static struct uk_spinlock pci_cfg_lock = UK_SPINLOCK_INITIALIZER();

static __u32 pci_cfg_read_raw(__u32 address)
{
	unsigned long flags;
	__u32 value;

	uk_spin_lock_irqsave(&pci_cfg_lock, flags);
	uk_arch_x86_64_outl(PCI_CONFIG_ADDR, address);
	value = uk_arch_x86_64_inl(PCI_CONFIG_DATA);
	uk_spin_unlock_irqrestore(&pci_cfg_lock, flags);
	return value;
}

static __u32 pci_cfg_address(const struct pci_address *addr, __u16 offset)
{
	return PCI_CONFIG_ADDRESS_ENABLE |
		   ((__u32)addr->bus << PCI_BUS_OFFSET) |
		   ((__u32)addr->devid << PCI_SLOT_OFFSET) |
		   ((__u32)addr->function << PCI_FUNC_OFFSET) |
		   (offset & ~3U);
}

int arch_pci_cfg_read(const struct pci_address *addr, __u16 offset, __u8 width,
		      __u32 *value)
{
	__u32 data = pci_cfg_read_raw(pci_cfg_address(addr, offset));

	data >>= (offset & 3) * 8;
	*value = width == 4 ? data : data & ((1U << (width * 8)) - 1);
	return 0;
}

int arch_pci_cfg_write(const struct pci_address *addr, __u16 offset, __u8 width,
		       __u32 value)
{
	unsigned long flags;
	__u16 port = PCI_CONFIG_DATA + (offset & 3);

	uk_spin_lock_irqsave(&pci_cfg_lock, flags);
	uk_arch_x86_64_outl(PCI_CONFIG_ADDR, pci_cfg_address(addr, offset));
	if (width == 1)
		uk_arch_x86_64_outb(port, (__u8)value);
	else if (width == 2)
		uk_arch_x86_64_outw(port, (__u16)value);
	else
		uk_arch_x86_64_outl(port, value);
	uk_spin_unlock_irqrestore(&pci_cfg_lock, flags);
	return 0;
}

static inline int pci_driver_add_device(struct pci_driver *drv,
					const struct pci_address *addr,
					const struct pci_device_id *devid,
					struct uk_alloc *allocator)
{
	struct pci_device *dev;
	__u32 value;
	int ret;

	UK_ASSERT(drv != NULL);
	UK_ASSERT(drv->add_dev != NULL);
	UK_ASSERT(addr != NULL);
	UK_ASSERT(devid != NULL);

	dev = uk_calloc(allocator, 1, sizeof(*dev));
	if (!dev) {
		uk_pr_err("PCI %02x:%02x.%02x: Failed to initialize: Out of memory!\n",
			  (int) addr->bus,
			  (int) addr->devid,
			  (int) addr->function);
		return -ENOMEM;
	}

	memcpy(&dev->id,   devid, sizeof(dev->id));
	memcpy(&dev->addr, addr,  sizeof(dev->addr));
	dev->drv = drv;

	ret = pci_cfg_read(addr, PCI_BASE_ADDRESS_0, 4, &value);
	if (ret)
		goto err_free;
	dev->base = value & ~0x3U;
	ret = pci_cfg_read(addr, PCI_INTERRUPT_LINE, 1, &value);
	if (ret)
		goto err_free;
	dev->irq = value;

	ret = drv->add_dev(dev);
	if (ret < 0) {
		uk_pr_err("PCI %02x:%02x.%02x: Failed to initialize device driver\n",
			  (int) addr->bus,
			  (int) addr->devid,
			  (int) addr->function);
		uk_free(allocator, dev);
	}
	return 0;
err_free:
	uk_free(allocator, dev);
	return ret;
}

static int scan_read(void *ctx __unused, const struct pci_address *addr,
		     __u16 offset, __u8 width, __u32 *value)
{
	return pci_cfg_read(addr, offset, width, value);
}

static int scan_visit(void *ctx, const struct pci_address *addr,
		      const struct pci_device_id *id)
{
	struct pci_driver *driver = pci_find_driver(id);

	uk_pr_info("PCI %02x:%02x.%02x (%04x %04x:%04x): ",
		   (int)addr->bus, (int)addr->devid, (int)addr->function,
		   (int)id->class_id, (int)id->vendor_id, (int)id->device_id);
	if (!driver) {
		uk_pr_info("<no driver>\n");
		return 0;
	}
	uk_pr_info("driver %p\n", driver);
	return pci_driver_add_device(driver, addr,
				     id, ctx);
}

int arch_pci_probe(struct uk_alloc *allocator)
{
	const struct pci_scan_ops ops = {
		.read = scan_read,
		.visit = scan_visit
	};

	return pci_enumerate(&ops, allocator);
}
