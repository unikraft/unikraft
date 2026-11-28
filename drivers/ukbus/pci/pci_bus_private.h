/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __PCI_BUS_PRIVATE_H__
#define __PCI_BUS_PRIVATE_H__

#include <uk/arch/types.h>
#include <uk/bus/pci.h>
#include <errno.h>

static inline __u64 pci_bar_io_size(__u32 mask)
{
	/* I/O BARs may implement only the low 16 address bits. */
	if (!(mask & 0xffff0000U))
		mask |= 0xffff0000U;
	return (__u32)(~mask + 1);
}

/* Preserve the old BAR0/base convention unless the consumer maps its BARs. */
static inline int pci_device_prepare_base(struct pci_device *dev)
{
	int rc;

	if (dev->drv->driver_maps_bars)
		return 0;
	rc = pci_device_map_bar(dev, 0);
	if (rc && rc != -ENODEV)
		return rc;
	if (dev->bar[0].type != PCI_BAR_NONE)
		dev->base = dev->bar[0].vbase;
	return 0;
}

#endif /* __PCI_BUS_PRIVATE_H__ */
