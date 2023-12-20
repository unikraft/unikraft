/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Jia He <justin.he@arm.com>
 *
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

#ifndef __UK_BUS_PCI_ECAM_H__
#define __UK_BUS_PCI_ECAM_H__

#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/bus/pci.h>
#include <uk/bus/platform.h>
#include <libfdt.h>

struct fdt_phandle_args;
extern struct pci_config_window pcw;
extern int gen_pci_irq_parse(const fdt32_t *addr, struct fdt_phandle_args *out_irq);

/*
 * struct to hold bus shift of the config window
 * for a PCI controller.
 */
struct pci_ecam_ops {
	unsigned int			bus_shift;
};

/*
 * struct to hold the mappings of a config space window. This
 * is expected to be used for PCI controllers that
 * use ECAM.
 */
struct bus_range {
	__u8			bus_start;
	__u8			bus_end;
};

struct pci_config_window {
	__paddr_t		config_base;
	__u64			config_space_size;
	struct bus_range br;
	struct pci_ecam_ops		*ops;
	__paddr_t		pci_device_base;
	__u64			pci_device_limit;
};

struct fdt_phandle_args {
	int np;
	int args_count;
	__u32 args[16];
};

/*
 * IO resources have these defined flags.
 *
 * PCI devices expose these flags to userspace in the "resource" sysfs file,
 * so don't move them.
 */
#define IORESOURCE_BITS		0x000000ff	/* Bus-specific bits */

#define IORESOURCE_TYPE_BITS	0x00001f00	/* Resource type */
#define IORESOURCE_IO		0x00000100	/* PCI/ISA I/O ports */
#define IORESOURCE_MEM		0x00000200
#define IORESOURCE_REG		0x00000300	/* Register offsets */
#define IORESOURCE_IRQ		0x00000400
#define IORESOURCE_DMA		0x00000800
#define IORESOURCE_BUS		0x00001000


int pci_generic_config_read(__u8 bus, __u8 devfn,
			    int where, int size, void *val);

int pci_generic_config_write(__u8 bus, __u8 devfn,
			     int where, int size, __u32 val);

extern struct pf_driver gen_pci_driver;

#endif /* __UK_BUS_PCI_ECAM_H__ */
