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
#include <pci/pci_bus.h>
#include <pci/pci_ecam.h>
#include <libfdt_env.h>

#define DEVFN(dev, fn) ((dev << PCI_FN_BIT_NBR) | fn)
#define SIZE_PER_PCI_DEV 0x20 /* legacy pci device size, no msi */

static int arch_pci_driver_add_device(struct pci_driver *drv,
				      struct pci_address *addr,
				      struct pci_device_id *devid, int irq,
				      __u64 base, struct uk_alloc *pha)
{
	struct pci_device *dev;
	int ret = 0;

	UK_ASSERT(drv != NULL);
	UK_ASSERT(drv->add_dev != NULL);
	UK_ASSERT(addr != NULL);
	UK_ASSERT(devid != NULL);
	UK_ASSERT(pha != NULL);

	dev = (struct pci_device *)uk_calloc(pha, 1, sizeof(*dev));
	if (!dev) {
		uk_pr_err("PCI %02x:%02x.%02x: Failed to initialize: Out of "
			  "memory!\n",
			  (int)addr->bus, (int)addr->devid,
			  (int)addr->function);
		return -ENOMEM;
	}

	memcpy(&dev->id, devid, sizeof(dev->id));
	memcpy(&dev->addr, addr, sizeof(dev->addr));
	dev->drv = drv;

	dev->base = base;
	dev->irq = irq;
	uk_pr_info("pci dev base(0x%lx) irq(%ld)\n", dev->base, dev->irq);

	if (drv->add_dev)
		ret = drv->add_dev(dev); // virtio pci
	if (ret < 0) {
		uk_pr_err(
		    "PCI %02x:%02x.%02x: Failed to initialize device driver\n",
		    (int)addr->bus, (int)addr->devid, (int)addr->function);
		uk_free(pha, dev);
	}

	return 0;
}

int arch_pci_probe(struct uk_alloc *pha)
{
	struct pci_address addr;
	struct pci_device_id devid;
	struct pci_driver *drv;
	uint32_t bus;
	uint8_t dev;
	int irq, pin = 0;
	__u64 base;
	int found_pci_device = 0;
	struct fdt_phandle_args out_irq;
	fdt32_t fdtaddr[3];

	uk_pr_debug("Probe PCI\n");

	for (bus = 0; bus < PCI_MAX_BUSES; ++bus) {
		for (dev = 0; dev < PCI_MAX_DEVICES; ++dev) {
			/* TODO: Retrieve the device identfier */
			addr.domain = 0x0;
			addr.bus = bus;
			addr.devid = dev;
			/* TODO: Retrieve the function bus,
			 * dev << PCI_DEV_BIT_NBR
			 */
			addr.function = 0x0;

			pci_generic_config_read(bus, DEVFN(dev, 0),
						PCI_VENDOR_ID, 2,
						(void *)&devid.vendor_id);
			if (devid.vendor_id == PCI_INVALID_ID) {
				/* Device doesn't exist */
				continue;
			}

			/* mark we found any pci device */
			found_pci_device = 1;

			pci_generic_config_read(bus, DEVFN(dev, 0),
						PCI_CLASS_REVISION, 4,
						(void *)&devid.class_id);
			pci_generic_config_read(bus, DEVFN(dev, 0),
						PCI_VENDOR_ID, 2,
						(void *)&devid.vendor_id);
			pci_generic_config_read(bus, DEVFN(dev, 0), PCI_DEV_ID,
						2, (void *)&devid.device_id);
			pci_generic_config_read(
			    bus, DEVFN(dev, 0), PCI_SUBSYSTEM_VID, 2,
			    (void *)&devid.subsystem_vendor_id);
			pci_generic_config_read(
			    bus, DEVFN(dev, 0), PCI_SUBSYSTEM_ID, 2,
			    (void *)&devid.subsystem_device_id);
			uk_pr_info("PCI %02x:%02x.%02x (%04x %04x:%04x): "
				   "sb=%d,sv=%4x\n",
				   (int)addr.bus, (int)addr.devid,
				   (int)addr.function, (int)devid.class_id,
				   (int)devid.vendor_id, (int)devid.device_id,
				   (int)devid.subsystem_device_id,
				   (int)devid.subsystem_vendor_id);

			/* TODO: gracefully judge it is a pci host bridge */
			if (bus == 0 && DEVFN(dev, 0) == 0) {
				pci_generic_config_write(
				    bus, 0, PCI_COMMAND, 2,
				    PCI_COMMAND_INTX_DISABLE);
				pci_generic_config_write(bus, 0, PCI_COMMAND, 2,
							 PCI_COMMAND_IO);
				continue;
			} else {
				base = pcw.pci_device_base
				       + (bus << 5 | dev) * SIZE_PER_PCI_DEV;
				pci_generic_config_write(
				    bus, DEVFN(dev, 0), PCI_COMMAND, 2,
				    PCI_COMMAND_INTX_DISABLE);
				pci_generic_config_write(
				    bus, DEVFN(dev, 0), PCI_BASE_ADDRESS_0, 4,
				    (bus << 5 | dev) * SIZE_PER_PCI_DEV);
				pci_generic_config_write(
				    bus, DEVFN(dev, 0), PCI_COMMAND, 2,
				    PCI_COMMAND_MASTER | PCI_COMMAND_IO);
			}

			drv = pci_find_driver(&devid);
			if (!drv) {
				uk_pr_info("<no driver> for dev id=%d\n",
					   devid.device_id);
				continue;
			}

			uk_pr_info("driver %p\n", drv);

			/* probe the irq info*/
			pci_generic_config_read(bus, DEVFN(dev, 0),
						PCI_INTERRUPT_PIN, 1,
						(void *)&pin);
			out_irq.args_count = 1;
			out_irq.args[0] = pin;
			fdtaddr[0] =
			    cpu_to_fdt32((bus << 16) | (DEVFN(dev, 0) << 8));
			fdtaddr[1] = fdtaddr[2] = cpu_to_fdt32(0);

			gen_pci_irq_parse(fdtaddr, &out_irq);
			irq = out_irq.args[0];

			arch_pci_driver_add_device(drv, &addr, &devid, irq,
						   base, pha);
		}
	}

	if (found_pci_device == 0)
		uk_pr_info("No pci device found!\n");

	return 0;
}
