/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Jia He <justin.he@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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

#include <stdbool.h>
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/print.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/bootinfo.h>
#include <libfdt_env.h>
#include <uk/ofw/fdt.h>
#include <libfdt.h>

#include "pci_ecam.h"

static struct uk_alloc *a;
static struct pci_ecam_ops pci_generic_ecam_ops;
static const struct device_match_table gen_pci_match_table[];

struct pci_config_window pcw = {
	.ops = &pci_generic_ecam_ops
};

struct pci_range_parser {
	int offset;		/*  in fdt */
	const fdt32_t *range;	/* start offset in offset */
	const fdt32_t *end;	/* end offset in offset */
	int np;			/* number of property */
	int pna;		/* parent number of address cell */
};

struct pci_range {
	__u32 pci_space;
	__u64 pci_addr;
	__u64 cpu_addr;
	__u64 size;
	__u32 flags;
};

static void *dtb;
static int gen_pci_fdt; // start offset of pci-ecam in fdt
/*
 * Function to implement the pci_ops ->map_bus method
 */
static void *pci_ecam_map_bus(__u8 bus, __u8 devfn, int where)
{
	__u8 bus_shift = pcw.ops->bus_shift;
	__u8 devfn_shift = bus_shift - 8;
	__u8 bus_start = pcw.br.bus_start;
	__u8 bus_end = pcw.br.bus_end;
	void *base = (void *)pcw.config_base;

	if (bus < bus_start || bus > bus_end)
		return NULL;

	return base + (bus << bus_shift) + (devfn << devfn_shift) + where;
}

int pci_generic_config_read(__u8 bus, __u8 devfn,
			    int where, int size, void *val)
{
	void *addr;

	/* add rmb before io read */
	rmb();
	addr = pci_ecam_map_bus(bus, devfn, where);
	if (!addr) {
		*(int *)val = ~0;
		return -1;
	}

	if (size == 1)
		*(__u8 *)val = ioreg_read8(addr);
	else if (size == 2)
		*(__u16 *)val = ioreg_read16(addr);
	else if (size == 4)
		*(__u32 *)val = ioreg_read32(addr);
	else
		uk_pr_err("not support size pci config read\n");

	return 0;
}

int pci_generic_config_write(__u8 bus, __u8 devfn,
			     int where, int size, __u32 val)
{
	void *addr;

	addr = pci_ecam_map_bus(bus, devfn, where);
	if (!addr)
		return -1;

	if (size == 1)
		ioreg_write8(addr, val);
	else if (size == 2)
		ioreg_write16(addr, val);
	else if (size == 4)
		ioreg_write32(addr, val);
	else
		uk_pr_err("not support size pci config write\n");

	/* add wmb after io write */
	wmb();

	return 0;
}

/* ECAM cfg */
static struct pci_ecam_ops pci_generic_ecam_ops = {
	.bus_shift	= 20
};

static unsigned int pci_get_flags(const fdt32_t *addr)
{
	unsigned int flags = 0;
	__u32 w = fdt32_to_cpu(*addr);

	/* only IORESOURCE_IO is supported currently */
	switch ((w >> 24) & 0x03) {
	case 0x01:
		flags |= IORESOURCE_IO;
		break;
	case 0x02: /* 32 bits */
	case 0x03: /* 64 bits */
	default:
		uk_pr_err("only supported pci ioresource flags\n");
		break;
	}

	return flags;
}

static int gen_pci_parser_range(struct pci_range_parser *parser, int offset)
{
	const int na = 3, ns = 2;
	int rlen;

	parser->pna = fdt_address_cells(dtb, offset);
	parser->np = parser->pna + na + ns;
	parser->range = fdt_getprop(dtb, offset, "ranges", &rlen);
	if (parser->range == NULL)
		return -ENOENT;

	parser->end = parser->range + rlen / sizeof(fdt32_t);

	return 0;
}

struct pci_range *pci_range_parser_one(struct pci_range_parser *parser,
					struct pci_range *range, int offset)
{
	const int na = 3, ns = 2; /* address and size */

	if (!range)
		return NULL;

	if (!parser->range || parser->range + parser->np > parser->end)
		return NULL;

	range->pci_space = fdt32_to_cpu(*parser->range);
	range->flags = pci_get_flags(parser->range);
	range->pci_addr = fdt_reg_read_number(parser->range + 1, ns);
	range->cpu_addr = fdt_translate_address_by_ranges(dtb, offset,
							  parser->range + na);
	range->size = fdt_reg_read_number(parser->range + parser->pna + na, ns);

	parser->range += parser->np;

	return range;
}

static int irq_find_parent(const void *fdt, int child)
{
	int p;
	int plen;
	const fdt32_t *prop;
	fdt32_t parent;

	do {
		prop = fdt_getprop(fdt, child, "interrupt-parent", &plen);
		if (prop == NULL) {
			p = fdt_parent_offset(fdt, child);
		} else	{
			parent = fdt32_to_cpu(prop[0]);
			p = fdt_node_offset_by_phandle(fdt, parent);
		}
		child = p;
	} while (p >= 0 && fdt_getprop(fdt, p, "#interrupt-cells", NULL) == NULL);

	return p;
}

int gen_pci_irq_parse(const fdt32_t *addr, struct fdt_phandle_args *out_irq)
{
	int ipar, tnode, old = 0, newpar = 0;
	fdt32_t initial_match_array[16];
	const fdt32_t *match_array = initial_match_array;
	const fdt32_t *tmp, *imap, *imask;
	fdt32_t dummy_imask[17];
	int intsize, newintsize;
	int addrsize, newaddrsize = 0;
	int imaplen, match, i, rc = -EINVAL;
	int plen;
	const fdt32_t *prop;

	ipar = gen_pci_fdt;
	memset((void *)dummy_imask, cpu_to_fdt32(~0), sizeof(dummy_imask));

	/* First get the #interrupt-cells property of the current cursor
	 * that tells us how to interpret the passed-in intspec. If there
	 * is none, we are nice and just walk up the tree
	 */
	do {
		prop = fdt_getprop(dtb, ipar, "#interrupt-cells", &plen);
		if (prop != NULL)
			break;
		ipar = irq_find_parent(dtb, ipar);
	} while (ipar >= 0);

	if (ipar < 0) {
		uk_pr_info(" -> no parent found !\n");
		goto fail;
	}

	intsize = fdt32_to_cpu(prop[0]);
	uk_pr_info("irq_parse_raw: ipar=%x, size=%d\n", ipar, intsize);

	if (out_irq->args_count != intsize)
		goto fail;

	/* Look for this #address-cells. We have to implement the old linux
	 * trick of looking for the parent here as some device-trees rely on it
	 */
	old = ipar;
	do {
		tmp = fdt_getprop(dtb, old, "#address-cells", NULL);
		tnode = fdt_parent_offset(dtb, old);
		old = tnode;
	} while (old >= 0 && tmp == NULL);

	old = 0;
	addrsize = (tmp == NULL) ? 2 : fdt32_to_cpu(*tmp);
	uk_pr_info(" -> addrsize=%d\n", addrsize);

	/* Precalculate the match array - this simplifies match loop */
	for (i = 0; i < addrsize; i++)
		initial_match_array[i] = addr ? addr[i] : 0;
	for (i = 0; i < intsize; i++)
		initial_match_array[addrsize + i] = cpu_to_fdt32(out_irq->args[i]);

	/* Now start the actual "proper" walk of the interrupt tree */
	while (ipar >= 0) {
		/* Now check if cursor is an interrupt-controller and if it is
		 * then we are done
		 */
		if (fdt_prop_read_bool(dtb, ipar,
				       "interrupt-controller")) {
			uk_pr_info(" -> got it !\n");
			return 0;
		}

		/*
		 * interrupt-map parsing does not work without a reg
		 * property when #address-cells != 0
		 */
		if (addrsize && !addr) {
			uk_pr_info(" -> no reg passed in when needed !\n");
			goto fail;
		}

		/* Now look for an interrupt-map */
		imap = fdt_getprop(dtb, ipar, "interrupt-map", &imaplen);
		/* No interrupt map, check for an interrupt parent */
		if (imap == NULL) {
			uk_pr_info(" -> no map, getting parent\n");
			newpar = irq_find_parent(dtb, ipar);
			goto skiplevel;
		}
		imaplen /= sizeof(__u32);

		/* Look for a mask */
		imask = fdt_getprop(dtb, ipar,
				    "interrupt-map-mask", NULL);
		if (!imask)
			imask = dummy_imask;

		/* Parse interrupt-map */
		match = 0;
		while (imaplen > (addrsize + intsize + 1) && !match) {
			/* Compare specifiers */
			match = 1;
			for (i = 0; i < (addrsize + intsize); i++, imaplen--)
				match &= !((match_array[i] ^ *imap++) & imask[i]);

			uk_pr_info(" -> match=%d (imaplen=%d)\n", match, imaplen);

			/* Get the interrupt parent */
			newpar = fdt_node_offset_by_phandle(dtb, fdt32_to_cpu(*imap));
			imap++;
			--imaplen;

			/* Check if not found */
			if (newpar < 0) {
				uk_pr_info(" -> imap parent not found !\n");
				goto fail;
			}

			/* Get #interrupt-cells and #address-cells of new
			 * parent
			 */
			prop = fdt_getprop(dtb, newpar, "#interrupt-cells",
					   &plen);
			if (prop == NULL) {
				uk_pr_info(" -> parent lacks #interrupt-cells!\n");
				goto fail;
			}
			newintsize = fdt32_to_cpu(prop[0]);

			prop = fdt_getprop(dtb, newpar, "#address-cells",
					   &plen);
			if (prop == NULL) {
				uk_pr_info(" -> parent lacks #address-cells!\n");
				goto fail;
			}
			newaddrsize = fdt32_to_cpu(prop[0]);

			uk_pr_debug(" -> newintsize=%d, newaddrsize=%d\n",
			    newintsize, newaddrsize);

			/* Check for malformed properties */
			if ((newaddrsize + newintsize > 16)
			    || (imaplen < (newaddrsize + newintsize))) {
				rc = -EFAULT;
				goto fail;
			}

			imap += newaddrsize + newintsize;
			imaplen -= newaddrsize + newintsize;

			uk_pr_info(" -> imaplen=%d\n", imaplen);
		}
		if (!match)
			goto fail;

		/*
		 * Successfully parsed an interrupt-map translation; copy new
		 * interrupt specifier into the out_irq structure
		 */
		match_array = imap - newaddrsize - newintsize;
		for (i = 0; i < newintsize; i++)
			out_irq->args[i] = fdt32_to_cpu(*(imap - newintsize + i));
		out_irq->args_count = intsize = newintsize;
		addrsize = newaddrsize;

skiplevel:
		/* Iterate again with new parent */
		out_irq->np = newpar;
		uk_pr_info(" -> new parent: %x\n", newpar);

		ipar = newpar;
		newpar = 0;
	}

	rc = -ENOENT; /* No interrupt-map found */

fail:
	return rc;
}


static int gen_pci_probe(struct pf_device *pfdev __unused)
{
	const fdt32_t *prop;
	int prop_len;
	__u64 reg_base;
	__u64 reg_size;
	struct pci_range range;
	struct pci_range_parser parser;
	__vaddr_t vaddr __maybe_unused;
	const char *comp;
	int err;

	dtb = (void *)ukplat_bootinfo_get()->dtb;
	/* 1.Get the base and size of config space */
	comp = gen_pci_match_table[0].compatible;
	gen_pci_fdt = fdt_node_offset_by_compatible(dtb, -1, comp);
	if (gen_pci_fdt < 0) {
		uk_pr_info("Error in searching pci controller in fdt\n");
		goto error_exit;
	} else {
		prop = fdt_getprop(dtb, gen_pci_fdt, "reg", &prop_len);
		if (!prop) {
			uk_pr_err("reg of device not found in fdt\n");
			goto error_exit;
		}

		/* Get the base addr and the size */
		fdt_get_address(dtb, gen_pci_fdt, 0,
				&reg_base, &reg_size);
		reg_base = fdt32_to_cpu(prop[0]);
		reg_base = reg_base << 32 | fdt32_to_cpu(prop[1]);
		reg_size = fdt32_to_cpu(prop[2]);
		reg_size = reg_size << 32 | fdt32_to_cpu(prop[3]);
	}

#if CONFIG_PAGING
	vaddr = uk_bus_pf_devmap(reg_base, reg_size);
	if (unlikely(PTRISERR(vaddr))) {
		uk_pr_err("Could not map MMIO region at 0x%lx - 0x%lx (%d)\n",
			  reg_base, reg_base + reg_size, PTR2ERR(vaddr));
		return PTR2ERR(vaddr);
	}
	pcw.config_base = vaddr;
#else /* !CONFIG_PAGING */
	pcw.config_base = reg_base;
#endif /* !CONFIG_PAGING */

	pcw.config_space_size = reg_size;
	uk_pr_info("generic pci config base(0x%lx), size(0x%lx)\n",
		   reg_base, reg_size);

	/* 2.Get the bus range of pci controller */
	prop = fdt_getprop(dtb, gen_pci_fdt, "bus-range", &prop_len);
	if (!prop) {
		uk_pr_err("bus-range of device not found in fdt\n");
		goto error_exit;
	}

	pcw.br.bus_start = fdt32_to_cpu(prop[0]);
	pcw.br.bus_end = fdt32_to_cpu(prop[1]);
	uk_pr_info("generic pci bus start(%d),end(%d)\n",
				pcw.br.bus_start, pcw.br.bus_end);

	if (pcw.br.bus_start > pcw.br.bus_end) {
		uk_pr_err("bus-range detect error in fdt\n");
		goto error_exit;
	}

	/* 3.Get the pci addr base and limit size for pci devices */
	err = gen_pci_parser_range(&parser, gen_pci_fdt);
	if (err < 0) {
		uk_pr_err("bus-range detect error in fdt\n");
		goto error_exit;
	}

	do {
		pci_range_parser_one(&parser, &range, gen_pci_fdt);
		if (range.flags == IORESOURCE_IO) {
			pcw.pci_device_base = range.cpu_addr;
			pcw.pci_device_limit = range.size;
			break;
		}
		parser.range += parser.np;
	} while (parser.range + parser.np <= parser.end);
	uk_pr_info("generic pci range base(0x%lx),size(0x%lx)\n",
				pcw.pci_device_base, pcw.pci_device_limit);

	return 0;

error_exit:
	return -ENODEV;
}

static int gen_pci_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;

	return 0;
}

static int gen_pci_add_dev(struct pf_device *pfdev __unused)
{
	return 0;
}

static const struct device_match_table gen_pci_match_table[];

static int gen_pci_id_match_compatible(const char *compatible)
{
	for (int i = 0; gen_pci_match_table[i].compatible != NULL; i++)
		if (strcmp(gen_pci_match_table[i].compatible, compatible) == 0)
			return gen_pci_match_table[i].id->device_id;

	return -1;
}

static struct pf_device_id gen_pci_ids = {
		.device_id = GEN_PCI_ID
};

struct pf_driver gen_pci_driver = {
	.device_ids = &gen_pci_ids,
	.init = gen_pci_drv_init,
	.probe = gen_pci_probe,
	.add_dev = gen_pci_add_dev,
	.match = gen_pci_id_match_compatible
};

static const struct device_match_table gen_pci_match_table[] = {
	{ .compatible = "pci-host-ecam-generic",
	  .id = &gen_pci_ids },
	{NULL}
};

PF_REGISTER_DRIVER(&gen_pci_driver);
