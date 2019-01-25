/* TODO: SPDX Header */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
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
#include <uk/print.h>
#include <cpu.h>
#include <pci/pci_bus.h>

struct pci_bus_handler {
	struct uk_bus b;
	struct uk_alloc *a;
	struct uk_list_head drv_list;  /**< List of PCI drivers */
	struct uk_list_head dev_list;  /**< List of PCI devices */
};
static struct pci_bus_handler ph;

#define PCI_INVALID_ID              (0xFFFF)
#define PCI_DEVICE_ID_MASK          (0xFFFF)

#define PCI_CONFIG_ADDR             (0xCF8)
#define PCI_CONFIG_DATA             (0xCFC)

/* 8 bits for bus number, 5 bits for devices */
#define PCI_MAX_BUSES               (1 << 8)
#define PCI_MAX_DEVICES             (1 << 5)

#define PCI_BUS_SHIFT               (16)
#define PCI_DEVICE_SHIFT            (11)
#define PCI_ENABLE_BIT              (1 << 31)

#define PCI_CONF_CLASS_ID          (0x08)
#define PCI_CONF_CLASS_ID_SHFT     (8)
#define PCI_CONF_CLASS_ID_MASK     (0x00FFFFFF)

#define PCI_CONF_VENDOR_ID          (0x00)
#define PCI_CONF_VENDOR_ID_SHFT     (0)
#define PCI_CONF_VENDOR_ID_MASK     (0x0000FFFF)

#define PCI_CONF_DEVICE_ID          (0x00)
#define PCI_CONF_DEVICE_ID_SHFT     (16)
#define PCI_CONF_DEVICE_ID_MASK     (0x0000FFFF)

#define PCI_CONF_SUBSYSVEN_ID          (0x2c)
#define PCI_CONF_SUBSYSVEN_ID_SHFT     (0)
#define PCI_CONF_SUBSYSVEN_ID_MASK     (0xFFFF)

#define PCI_CONF_SUBSYS_ID          (0x2c)
#define PCI_CONF_SUBSYS_ID_SHFT     (16)
#define PCI_CONF_SUBSYS_ID_MASK     (0xFFFF)

#define PCI_CONF_IRQ                (0X3C)
#define PCI_CONF_IRQ_SHFT           (0x0)
#define PCI_CONF_IRQ_MASK           (0XFF)

#define PCI_CONF_IOBAR              (0x10)
#define PCI_CONF_IOBAR_SHFT         (0x0)
#define PCI_CONF_IOBAR_MASK         (~0x3)

#define PCI_CONF_READ(type, ret, a, s)					\
	do {								\
		uint32_t _conf_data;					\
		outl(PCI_CONFIG_ADDR, (a) | PCI_CONF_##s);		\
		_conf_data = ((inl(PCI_CONFIG_DATA) >> PCI_CONF_##s##_SHFT) \
			      & PCI_CONF_##s##_MASK);			\
		*(ret) = (type) _conf_data;				\
	} while (0)

static inline int pci_device_id_match(const struct pci_device_id *id0,
					const struct pci_device_id *id1)
{
	if ((id0->class_id != PCI_CLASS_ANY_ID) &&
	    (id1->class_id != PCI_CLASS_ANY_ID) &&
	    (id0->class_id != id1->class_id)) {
		return 0;
	}
	if ((id0->vendor_id != PCI_ANY_ID) &&
	    (id1->vendor_id != PCI_ANY_ID) &&
	    (id0->vendor_id != id1->vendor_id)) {
		return 0;
	}
	if ((id0->device_id != PCI_ANY_ID) &&
	    (id1->device_id != PCI_ANY_ID) &&
	    (id0->device_id != id1->device_id)) {
		return 0;
	}
	if ((id0->subsystem_vendor_id != PCI_ANY_ID) &&
	    (id1->subsystem_vendor_id != PCI_ANY_ID) &&
	    (id0->subsystem_vendor_id != id1->subsystem_vendor_id)) {
		return 0;
	}
	if ((id0->subsystem_device_id != PCI_ANY_ID) &&
	    (id1->subsystem_device_id != PCI_ANY_ID) &&
	    (id0->subsystem_device_id != id1->subsystem_device_id)) {
		return 0;
	}
	return 1;
}

static inline int pci_device_id_is_any(const struct pci_device_id *id)
{
	if ((id->class_id == PCI_CLASS_ANY_ID) &&
	    (id->vendor_id == PCI_ANY_ID) &&
	    (id->device_id == PCI_ANY_ID) &&
	    (id->subsystem_vendor_id == PCI_ANY_ID) &&
	    (id->subsystem_device_id == PCI_ANY_ID)) {
		return 1;
	}
	return 0;
}

static inline struct pci_driver *pci_find_driver(struct pci_device_id *id)
{
	struct pci_driver *drv;
	const struct pci_device_id *drv_id;

	uk_list_for_each_entry(drv, &ph.drv_list, list) {
		for (drv_id = drv->device_ids;
		     !pci_device_id_is_any(drv_id);
		     drv_id++) {
			if (pci_device_id_match(id, drv_id))
				return drv;
		}
	}
	return NULL; /* no driver found */
}

static inline int pci_driver_add_device(struct pci_driver *drv,
					struct pci_address *addr,
					struct pci_device_id *devid)
{
	struct pci_device *dev;
	uint32_t config_addr;
	int ret;

	UK_ASSERT(drv != NULL);
	UK_ASSERT(drv->add_dev != NULL);
	UK_ASSERT(addr != NULL);
	UK_ASSERT(devid != NULL);

	dev = (struct pci_device *) uk_calloc(ph.a, 1, sizeof(*dev));
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

	config_addr = (PCI_ENABLE_BIT)
			| (addr->bus << PCI_BUS_SHIFT)
			| (addr->devid << PCI_DEVICE_SHIFT);
	PCI_CONF_READ(uint16_t, &dev->base, config_addr, IOBAR);
	PCI_CONF_READ(uint8_t, &dev->irq, config_addr, IRQ);

	ret = drv->add_dev(dev);
	if (ret < 0) {
		uk_pr_err("PCI %02x:%02x.%02x: Failed to initialize device driver\n",
			  (int) addr->bus,
			  (int) addr->devid,
			  (int) addr->function);
		uk_free(ph.a, dev);
	}
	return 0;
}

static int pci_probe(void)
{
	struct pci_address addr;
	struct pci_device_id devid;
	struct pci_driver *drv;
	uint32_t config_addr, config_data;
	uint32_t bus;
	uint8_t dev;

	uk_pr_debug("Probe PCI\n");

	for (bus = 0; bus < PCI_MAX_BUSES; ++bus) {
		for (dev = 0; dev < PCI_MAX_DEVICES; ++dev) {
			config_addr = (PCI_ENABLE_BIT)
					| (bus << PCI_BUS_SHIFT)
					| (dev << PCI_DEVICE_SHIFT);

			outl(PCI_CONFIG_ADDR, config_addr);
			config_data = inl(PCI_CONFIG_DATA);

			/* TODO: Retrieve the device identfier */
			addr.domain   = 0x0;
			addr.bus      = bus;
			addr.devid    = dev;
			 /* TODO: Retrieve the function */
			addr.function = 0x0;

			devid.vendor_id = config_data & PCI_DEVICE_ID_MASK;
			if (devid.vendor_id == PCI_INVALID_ID) {
				/* Device doesn't exist */
				continue;
			}

			PCI_CONF_READ(uint32_t, &devid.class_id,
					config_addr, CLASS_ID);
			PCI_CONF_READ(uint16_t, &devid.vendor_id,
					config_addr, VENDOR_ID);
			PCI_CONF_READ(uint16_t, &devid.device_id,
					config_addr, DEVICE_ID);
			PCI_CONF_READ(uint16_t, &devid.subsystem_device_id,
					config_addr, SUBSYS_ID);
			PCI_CONF_READ(uint16_t, &devid.subsystem_vendor_id,
					config_addr, SUBSYSVEN_ID);

			uk_pr_info("PCI %02x:%02x.%02x (%04x %04x:%04x): ",
				   (int) addr.bus,
				   (int) addr.devid,
				   (int) addr.function,
				   (int) devid.class_id,
				   (int) devid.vendor_id,
				   (int) devid.device_id);
			drv = pci_find_driver(&devid);
			if (!drv) {
				uk_pr_info("<no driver>\n");
				continue;
			}
			uk_pr_info("driver %p\n", drv);
			pci_driver_add_device(drv, &addr, &devid);
		}
	}
	return 0;
}


static int pci_init(struct uk_alloc *a)
{
	struct pci_driver *drv, *drv_next;
	int ret = 0;

	UK_ASSERT(a != NULL);

	ph.a = a;

	uk_list_for_each_entry_safe(drv, drv_next, &ph.drv_list, list) {
		if (drv->init) {
			ret = drv->init(a);
			if (ret == 0)
				continue;
			uk_pr_err("Failed to initialize driver %p: %d\n",
				  drv, ret);
			uk_list_del_init(&drv->list);
		}
	}
	return 0;
}

void _pci_register_driver(struct pci_driver *drv)
{
	UK_ASSERT(drv != NULL);
	uk_list_add_tail(&drv->list, &ph.drv_list);
}


/* Register this bus driver to libukbus:
 */
static struct pci_bus_handler ph = {
	.b.init = pci_init,
	.b.probe = pci_probe,
	.drv_list = UK_LIST_HEAD_INIT(ph.drv_list),
	.dev_list = UK_LIST_HEAD_INIT(ph.dev_list),
};
UK_BUS_REGISTER(&ph.b);
