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
#include <uk/print.h>
#include <uk/plat/common/cpu.h>
#include <uk/bus/pci.h>

#define PCI_CONF_READ(type, ret, a, s)					\
	do {								\
		uint32_t _conf_data;					\
		outl(PCI_CONFIG_ADDR, (a) | PCI_CONF_##s);		\
		_conf_data = ((inl(PCI_CONFIG_DATA) >> PCI_CONF_##s##_SHFT) \
			      & PCI_CONF_##s##_MASK);			\
		*(ret) = (type) _conf_data;				\
	} while (0)

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

static void probe_bus(uint32_t);

/* Probe a function. Return 1 if the function does not exist in the device, 0
 * otherwise.
 */
static int probe_function(uint32_t bus, uint32_t device, uint32_t function)
{
	uint32_t config_addr, config_data, secondary_bus;
	struct pci_address addr;
	struct pci_device_id devid;
	struct pci_driver *drv;

	config_addr = (PCI_ENABLE_BIT)
			| (bus << PCI_BUS_SHIFT)
			| (device << PCI_DEVICE_SHIFT)
			| (function << PCI_FUNCTION_SHIFT);

	outl(PCI_CONFIG_ADDR, config_addr);
	config_data = inl(PCI_CONFIG_DATA);

	devid.vendor_id = config_data & PCI_DEVICE_ID_MASK;
	if (devid.vendor_id == PCI_INVALID_ID) {
		/* Device doesn't exist */
		return 1;
	}

	addr.domain   = 0x0;
	addr.bus      = bus;
	addr.devid    = device;
	addr.function = function;

	/* These I/O reads could be batched, but it practice this does not
	 * appear to make the code more performant.
	 */
	PCI_CONF_READ(uint32_t, &devid.class_id,
			config_addr, CLASS_ID);
	PCI_CONF_READ(uint32_t, &devid.sub_class_id,
			config_addr, SUBCLASS_ID);
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
	} else {
		uk_pr_info("driver %p\n", drv);
		pci_driver_add_device(drv, &addr, &devid);
	}

	/* 0x06 = Bridge Device, 0x04 = PCI-to-PCI bridge */
	if ((devid.class_id == 0x06) && (devid.sub_class_id == 0x04)) {
		PCI_CONF_READ(uint32_t, &secondary_bus,
				config_addr, SECONDARY_BUS);
		probe_bus(secondary_bus);
	}

	return 0;
}

/* Recursive PCI enumeration: this function is called recursively by
 * probe_function upon discovering PCI-to-PCI bridges.
 */
static void probe_bus(uint32_t bus)
{
	uint32_t config_addr, device, header_type, function = 0;

	for (device = 0; device < PCI_MAX_DEVICES; ++device) {
		if (!probe_function(bus, device, function))
			continue;

		config_addr = (PCI_ENABLE_BIT);
		PCI_CONF_READ(uint32_t, &header_type,
				config_addr, HEADER_TYPE);

		/* Is this a multi-function device? */
		if ((header_type & PCI_HEADER_TYPE_MSB_MASK) == 0)
			continue;

		/* Check remaining functions */
		for (function = 1; function < PCI_MAX_FUNCTIONS; function++)
			probe_function(bus, device, function);
	}
}

int arch_pci_probe(struct uk_alloc *pha)
{
	uint32_t config_addr, function, header_type, vendor_id;

	uk_pr_debug("Probe PCI\n");

	ph.a = pha;

	config_addr = (PCI_ENABLE_BIT);
	PCI_CONF_READ(uint32_t, &header_type,
			config_addr, HEADER_TYPE);

	if ((header_type & PCI_HEADER_TYPE_MSB_MASK) == 0) {
		/* Single PCI host controller */
		probe_bus(0);
	} else {
		/* Multiple PCI host controllers */
		for (function = 0; function < PCI_MAX_FUNCTIONS; function++) {
			config_addr = (PCI_ENABLE_BIT) |
					(function << PCI_FUNCTION_SHIFT);

			PCI_CONF_READ(uint32_t, &vendor_id,
					config_addr, VENDOR_ID);

			if (vendor_id == PCI_INVALID_ID)
				break;

			probe_bus(function);
		}
	}

	return 0;
}
