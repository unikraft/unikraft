/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __PCI_INTERNAL_H__
#define __PCI_INTERNAL_H__

#include <uk/bus/pci.h>

int arch_pci_probe(struct uk_alloc *allocator);
int arch_pci_cfg_read(const struct pci_address *addr, __u16 offset, __u8 width,
		      __u32 *value);
int arch_pci_cfg_write(const struct pci_address *addr, __u16 offset, __u8 width,
		       __u32 value);

#endif /* __PCI_INTERNAL_H__ */
