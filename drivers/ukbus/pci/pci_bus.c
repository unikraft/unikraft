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
#include <errno.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/bus/pci.h>
#if CONFIG_LIBUKPAGING
#include <uk/bus/platform.h>
#endif /* CONFIG_LIBUKPAGING */

extern int arch_pci_probe(struct uk_alloc *pha);

static inline int pci_device_id_match(const struct pci_device_id *id0,
					const struct pci_device_id *id1)
{
	if ((id0->class_id != PCI_CLASS_ANY_ID) &&
	    (id1->class_id != PCI_CLASS_ANY_ID) &&
	    (id0->class_id != id1->class_id)) {
		return 0;
	}
	if ((id0->sub_class_id != PCI_CLASS_ANY_ID) &&
	    (id1->sub_class_id != PCI_CLASS_ANY_ID) &&
	    (id0->sub_class_id != id1->sub_class_id)) {
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
	    (id->sub_class_id == PCI_CLASS_ANY_ID) &&
	    (id->vendor_id == PCI_ANY_ID) &&
	    (id->device_id == PCI_ANY_ID) &&
	    (id->subsystem_vendor_id == PCI_ANY_ID) &&
	    (id->subsystem_device_id == PCI_ANY_ID)) {
		return 1;
	}
	return 0;
}

struct pci_driver *pci_find_driver(struct pci_device_id *id)
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

static inline __u64 pci_bar_size32(__u32 mask)
{
	if (!mask)
		return 0;

	return ((__u32)~mask) + 1;
}

static inline __u64 pci_bar_size64(__u64 mask)
{
	if (!mask)
		return 0;

	return (~mask) + 1;
}

static inline int pci_bar_size_valid(__u64 size, __u64 min_size)
{
	return size >= min_size && !(size & (size - 1));
}

int pci_device_probe_bar(struct pci_device *dev, unsigned int bar)
{
	struct pci_bar *pbar;
	__u32 attrs, mem_type, off, orig, probe;
	__u64 mask, pbase, size;

	UK_ASSERT(dev);

	if (bar >= PCI_BAR_COUNT)
		return -EINVAL;

	pbar = &dev->bar[bar];
	memset(pbar, 0, sizeof(*pbar));
	pbar->index = bar;

	off = PCI_BASE_ADDRESS_0 + bar * sizeof(__u32);
	orig = pci_config_read32(dev, off);
	if (orig == __U32_MAX)
		return 0;

	pci_config_write32(dev, off, __U32_MAX);
	probe = pci_config_read32(dev, off);
	pci_config_write32(dev, off, orig);
	if (!probe)
		return 0;
	attrs = orig ? orig : probe;

	if (attrs & PCI_BASE_ADDRESS_SPACE_IO) {
		mask = probe & PCI_BASE_ADDRESS_IO_MASK;
		pbase = orig & PCI_BASE_ADDRESS_IO_MASK;
		size = pci_bar_size32(mask);
		if (!pci_bar_size_valid(size, sizeof(__u32)) ||
		    (pbase & (size - 1)))
			return -EINVAL;
		if (!pbase)
			return 0;
		pbar->type = PCI_BAR_IO;
		pbar->pbase = pbase;
		pbar->vbase = (__vaddr_t)pbase;
		pbar->size = size;
		pbar->flags = attrs & ~PCI_BASE_ADDRESS_IO_MASK;
		return 0;
	}

	mem_type = attrs & PCI_BASE_ADDRESS_MEM_TYPE_MASK;
	if (mem_type != PCI_BASE_ADDRESS_MEM_TYPE_32 &&
	    mem_type != PCI_BASE_ADDRESS_MEM_TYPE_1M &&
	    mem_type != PCI_BASE_ADDRESS_MEM_TYPE_64)
		return -EINVAL;

	if (mem_type == PCI_BASE_ADDRESS_MEM_TYPE_64) {
		__u32 off_hi, orig_hi, probe_hi;

		if (bar == PCI_BAR_COUNT - 1)
			return -EINVAL;

		off_hi = off + sizeof(__u32);
		orig_hi = pci_config_read32(dev, off_hi);
		pci_config_write32(dev, off, __U32_MAX);
		pci_config_write32(dev, off_hi, __U32_MAX);
		probe = pci_config_read32(dev, off);
		probe_hi = pci_config_read32(dev, off_hi);
		pci_config_write32(dev, off_hi, orig_hi);
		pci_config_write32(dev, off, orig);

		mask = ((__u64)probe_hi << 32) |
		       (probe & PCI_BASE_ADDRESS_MEM_MASK);
		pbase = ((__u64)orig_hi << 32) |
			(orig & PCI_BASE_ADDRESS_MEM_MASK);
		pbar->is_64 = 1;
	} else {
		if (mem_type == PCI_BASE_ADDRESS_MEM_TYPE_1M) {
			mask = (probe & PCI_BASE_ADDRESS_MEM_1M_MASK) |
			       ~((__u64)PCI_BASE_ADDRESS_MEM_1M_MASK | 0xf);
			pbase = orig & PCI_BASE_ADDRESS_MEM_1M_MASK;
		} else {
			mask = probe & PCI_BASE_ADDRESS_MEM_MASK;
			pbase = orig & PCI_BASE_ADDRESS_MEM_MASK;
		}
	}
	size = pbar->is_64 ? pci_bar_size64(mask) : pci_bar_size32(mask);
	if (!pci_bar_size_valid(size, 16) || (pbase & (size - 1)))
		return -EINVAL;
	if (!pbase)
		return 0;

	pbar->type = PCI_BAR_MEM;
	pbar->pbase = pbase;
	pbar->size = size;
	pbar->flags = attrs & ~PCI_BASE_ADDRESS_MEM_MASK;
	return 0;
}

int pci_device_probe_bars(struct pci_device *dev)
{
	unsigned int i;
	__u16 cmd;
	int rc = 0;

	UK_ASSERT(dev);

	cmd = pci_config_read16(dev, PCI_COMMAND);
	if (cmd & PCI_COMMAND_DECODE_ENABLE)
		pci_config_write16(dev, PCI_COMMAND,
				   cmd & ~PCI_COMMAND_DECODE_ENABLE);

	for (i = 0; i < PCI_BAR_COUNT; i++) {
		rc = pci_device_probe_bar(dev, i);
		if (unlikely(rc))
			break;
		if (dev->bar[i].is_64)
			i++;
	}

	if (cmd & PCI_COMMAND_DECODE_ENABLE)
		pci_config_write16(dev, PCI_COMMAND, cmd);

	return rc;
}

int pci_device_map_bar(struct pci_device *dev, unsigned int bar)
{
	struct pci_bar *pbar;
#if CONFIG_LIBUKPAGING
	__vaddr_t vaddr;
#endif /* CONFIG_LIBUKPAGING */

	UK_ASSERT(dev);

	if (bar >= PCI_BAR_COUNT)
		return -EINVAL;

	pbar = &dev->bar[bar];
	if (pbar->type == PCI_BAR_NONE)
		return -ENODEV;
	if (!pbar->size || !pbar->pbase)
		return -EINVAL;

	if (pbar->type == PCI_BAR_IO) {
		if (pbar->pbase > __U16_MAX ||
		    pbar->size > (__u64)__U16_MAX + 1 - pbar->pbase)
			return -ERANGE;
		pbar->vbase = (__vaddr_t)pbar->pbase;
		return 0;
	}
	if (pbar->vbase)
		return 0;
	if (pbar->size > __U64_MAX - pbar->pbase)
		return -ERANGE;

#if CONFIG_LIBUKPAGING
	vaddr = uk_bus_pf_devmap(pbar->pbase, pbar->size);
	if (unlikely(PTRISERR(vaddr)))
		return PTR2ERR(vaddr);
	pbar->vbase = vaddr;
#else /* !CONFIG_LIBUKPAGING */
	pbar->vbase = (__vaddr_t)pbar->pbase;
#endif /* !CONFIG_LIBUKPAGING */

	return 0;
}

int pci_device_enable(struct pci_device *dev)
{
	__u16 cmd;
	unsigned int i;

	UK_ASSERT(dev);

	cmd = pci_config_read16(dev, PCI_COMMAND);
	cmd |= PCI_COMMAND_MASTER;
	for (i = 0; i < PCI_BAR_COUNT; i++) {
		if (dev->bar[i].type == PCI_BAR_IO)
			cmd |= PCI_COMMAND_IO;
		else if (dev->bar[i].type == PCI_BAR_MEM)
			cmd |= PCI_COMMAND_MEMORY;
	}
	pci_config_write16(dev, PCI_COMMAND, cmd);

	return 0;
}

void pci_device_intx(struct pci_device *dev, int enable)
{
	__u16 cmd;

	UK_ASSERT(dev);

	cmd = pci_config_read16(dev, PCI_COMMAND);
	if (enable)
		cmd &= ~PCI_COMMAND_INTX_DISABLE;
	else
		cmd |= PCI_COMMAND_INTX_DISABLE;
	pci_config_write16(dev, PCI_COMMAND, cmd);
}

static int pci_probe(void)
{
	return arch_pci_probe(ph.a);
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
