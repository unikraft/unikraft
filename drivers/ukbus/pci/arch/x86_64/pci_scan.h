/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __PCI_SCAN_H__
#define __PCI_SCAN_H__

#include <uk/bus/pci.h>

struct pci_scan_ops {
	int (*read)(void *ctx, const struct pci_address *addr, __u16 offset,
		    __u8 width, __u32 *value);
	int (*visit)(void *ctx, const struct pci_address *addr,
		     const struct pci_device_id *id);
};

int pci_enumerate(const struct pci_scan_ops *ops, void *ctx);

#endif /* __PCI_SCAN_H__ */
