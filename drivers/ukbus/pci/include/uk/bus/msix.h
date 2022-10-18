/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marco Schlumpp <marco@unikraft.io>
 *
 * Copyright (c) 2022, Unikraft GmbH. All rights reserved.
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

#ifndef __UKPLAT_COMMON_PCI_MSIX_H__
#define __UKPLAT_COMMON_PCI_MSIX_H__

#include <stddef.h>
#include <stdint.h>

#include <uk/bus/pci.h>

#define PCI_CONF_MSIX_MTAB 0x4

/* MXC register */
#define PCI_CONF_MSIX_TBL_SIZE 		0x0
#define PCI_CONF_MSIX_TBL_SIZE_SHFT 	16
#define PCI_CONF_MSIX_TBL_SIZE_MASK 	(0x7FFU)

#define PCI_CONF_MSIX_ENABLE 		0x0
#define PCI_CONF_MSIX_ENABLE_SHFT 	31
#define PCI_CONF_MSIX_ENABLE_MASK 	(0x1U)

/* MTAB register */
#define PCI_CONF_MSIX_TABLE_BAR		0x4
#define PCI_CONF_MSIX_TABLE_BAR_SHFT	0
#define PCI_CONF_MSIX_TABLE_BAR_MASK	(0x7U)

#define PCI_CONF_MSIX_TABLE_OFFSET	0x4
#define PCI_CONF_MSIX_TABLE_OFFSET_SHFT	0
#define PCI_CONF_MSIX_TABLE_OFFSET_MASK	(0xFFFFFFF8U)

struct pci_msix_table_entry {
	uint32_t addr_low;
	uint32_t addr_high;
	uint32_t msg_data;
	uint32_t vector_control;
};

size_t pci_msix_table_size(struct pci_device *dev);
uint8_t pci_msix_table_bar(struct pci_device *dev);
uint32_t pci_msix_table_offset(struct pci_device *dev);

int pci_msix_enable(struct pci_device *dev, unsigned int *irqs, uint16_t count);
int pci_msix_disable(struct pci_device *dev, unsigned int *irqs, uint16_t count);

void ukplat_pci_msix_setup_table_entry(struct pci_msix_table_entry *entry,
				       unsigned int vector);

#endif /* __UKPLAT_COMMON_PCI_MSIX_H__ */
