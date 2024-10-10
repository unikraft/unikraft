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

#ifndef __UKPLAT_COMMON_PCI_X86_MSIX_H__
#define __UKPLAT_COMMON_PCI_X86_MSIX_H__

#include <uk/essentials.h>
#include <uk/bus/pci.h>

#define PCI_MSIX_X86_DEST_MODE_PHYSICAL		0x0
#define PCI_MSIX_X86_DEST_MODE_LOGICAL 		0x1

#define PCI_MSIX_X86_DELIVERY_FIXED		0x0
#define PCI_MSIX_X86_DELIVERY_LOWER_PRIO	0x1
#define PCI_MSIX_X86_DELIVERY_SMI		0x2
#define PCI_MSIX_X86_DELIVERY_NMI		0x4
#define PCI_MSIX_X86_DELIVERY_INIT		0x5
#define PCI_MSIX_X86_DELIVERY_EXTINT		0x7

#define PCI_MSIX_X86_TRIGGER_EDGE		0x0
#define PCI_MSIX_X86_TRIGGER_LEVEL		0x1

#define PCI_MSIX_X86_LEVEL_ASSERT		0x0
#define PCI_MSIX_X86_LEVEL_DEASSERT		0x1

struct pci_msix_table_entry_x86 {
	__u8 dest_id;
	__u8 redirection_hint;
	__u8 destination_mode;

	__u8 vector;
	__u8 delivery_mode;
	__u8 level;
	__u8 trigger;
};

#endif /* __UKPLAT_COMMON_PCI_X86_MSIX_H__ */
