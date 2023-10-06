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
#include <inttypes.h>
#include <uk/print.h>
#include <uk/plat/common/cpu.h>
#include <uk/bus/pci.h>
#include <uk/plat/io.h>
#include <uk/arch/limits.h>
#ifdef CONFIG_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#else
#include <uk/alloc.h>
#endif

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

static int pci_get_bar(struct pci_device *dev, uint8_t idx, __paddr_t *bar_out,
		       uint8_t *is_64bit)
{
	uint32_t cfg, offset, bar_low, bar_high;
	__paddr_t base;

	UK_ASSERT(dev);
	UK_ASSERT(bar_out);
	UK_ASSERT(idx < PCI_MAX_BARS);

	cfg = dev->config_addr;
	offset = PCI_BASE_ADDRESS_0 + idx * 4;

	PCI_CONF_READ_OFFSET(uint32_t, &bar_low, cfg, offset, 0, UINT32_MAX);
	if (bar_low & 0x1)
		return ENOTSUP;

	base = 0;
	switch ((bar_low >> 1) & 0x3) {
	case 0x2:
		/* 64-bit base address */
		PCI_CONF_READ_OFFSET(uint32_t, &bar_high, cfg, offset + 4, 0,
				     UINT32_MAX);
		base = (__paddr_t)bar_high << 32;
		/* fallthrough */
	case 0x0:
		/* 32-bit base address */
		base |= (__paddr_t)bar_low & ~0xf;
		break;
	default:
		return EINVAL;
	}

	if (is_64bit != NULL)
		*is_64bit = ((bar_low >> 1) & 0x3) == 0x2;
	*bar_out = base;
	return 0;
}

static int pci_set_bar(struct pci_device *dev, uint8_t idx, __paddr_t value)
{
	uint32_t cfg, offset, bar_low;

	UK_ASSERT(dev);
	UK_ASSERT(idx < PCI_MAX_BARS);

	cfg = dev->config_addr;
	offset = PCI_BASE_ADDRESS_0 + idx * 4;

	PCI_CONF_READ_OFFSET(uint32_t, &bar_low, cfg,
			     offset, 0, UINT32_MAX);
	if (bar_low & 0x1)
		return ENOTSUP;

	switch ((bar_low >> 1) & 0x3) {
	case 0x2:
		/* 64-bit base address */
		PCI_CONF_WRITE_OFFSET(uint32_t, cfg, offset + 4, 0, UINT32_MAX,
				      value >> 32);
		/* fallthrough */
	case 0x0:
		/* 32-bit base address */
		PCI_CONF_WRITE_OFFSET(uint32_t, cfg, offset, 0, UINT32_MAX,
				      value & UINT32_MAX);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int pci_bar_phys_region(struct pci_device *pci_dev, uint32_t idx,
			__paddr_t *addr_out, size_t *size_out)
{
	uint8_t is_64bit;
	uint64_t saved_value, size;
	int rc;

	UK_ASSERT(pci_dev);
	UK_ASSERT(addr_out);
	UK_ASSERT(size_out);
	UK_ASSERT(idx < PCI_MAX_BARS);

	rc = pci_get_bar(pci_dev, idx, &saved_value, &is_64bit);
	if (rc)
		return rc;

	/* Write all ones to base address register and read actual value back */
	rc = pci_set_bar(pci_dev, idx, UINT64_MAX);
	if (rc)
		return rc;
	rc = pci_get_bar(pci_dev, idx, &size, NULL);
	if (rc)
		return rc;

	/* Hardwired to zero means that it's not there. We also don't have to
	 * restore the old value in that case.
	 */
	if (size == 0)
		return -ENODEV;

	/* Restore old value */
	rc = pci_set_bar(pci_dev, idx, saved_value);
	if (rc)
		return rc;

	*addr_out = saved_value;
	if (is_64bit)
		*size_out = ~size + 1;
	else
		*size_out = ~((uint32_t)size) + 1;

	return 0;
}

/**
 * Try to allocate the physical memory at @a paddr. This will allocate memory at
 * another address if the region may be already in use.
 * @param[inout] paddr the start of the physical memory range. Will be set the
 *		       address to be used.
 * @param pages the count of pages to allocate.
 * @returns zero if the allocation at the specified address was successful, 1
 *	    if space at a different address was allocated, and a negative errno
 *	    in case the allocation was not successful.
 */
static int phys_alloc(__paddr_t *paddr, __sz pages)
{
#ifdef CONFIG_PAGING
	struct uk_pagetable *pt;
	int rc;

	pt = ukplat_pt_get_active();

	/* Allocate phys frames for the area. But don't allow that for NULL
	 * addresses
	 */
	if (*paddr != 0)
		rc = pt->fa->falloc(pt->fa, paddr, pages, 0);
	else
		rc = -ENOMEM;

	if (rc == -ENOMEM) {
		/* Range is already occupied. Move the PCI device */
		*paddr = __PADDR_ANY;
		rc = pt->fa->falloc(pt->fa, paddr, pages, 0);
		if (rc)
			return rc;
		return 1;
	} else if (rc && rc != -EFAULT) {
		/* Some other error happened (and that error was not that the
		 * range was outside the frame allocator range)
		 */
		return rc;
	}

	return 0;
#else
	void *a;

	/* We have no idea what is used without a frame allocator, therefore
	 * always allocate memory
	 */
	a = uk_malloc(uk_alloc_get_default(), pages * __PAGE_SIZE);
	if (a == NULL)
		return -ENOMEM;
	*paddr = (__paddr_t)a;
	return 1;
#endif
}

static int physmem_free(__paddr_t paddr, __sz pages)
{
#ifdef CONFIG_PAGING
	struct uk_pagetable *pt;

	pt = ukplat_pt_get_active();
	return pt->fa->ffree(pt->fa, paddr, pages);
#else
	(void)pages;
	uk_free(uk_alloc_get_default(), (void *)paddr);
	return 0;
#endif
}

/* FIXME: We may have to track the maps, because for example msix_* needs an
 * accessible MSI-X table.
 */

int pci_map_bar(struct pci_device *dev, __u8 idx, int attr,
		struct pci_bar_memory *mem)
{
	__paddr_t bar_phys;
	__vaddr_t bar_virt;
	__sz bar_size;
	__sz bar_pages;
	int rc;
#ifdef CONFIG_PAGING
	struct uk_pagetable *pt;
#endif

	UK_ASSERT(mem);

	/* Fetch BAR address and size of the memory region */
	rc = pci_bar_phys_region(dev, idx, &bar_phys, &bar_size);
	if (rc)
		return rc;
	bar_pages = DIV_ROUND_UP(bar_size, __PAGE_SIZE);

	rc = phys_alloc(&bar_phys, bar_pages);
	if (rc < 0)
		return rc;
	if (rc == 1) {
		rc = pci_set_bar(dev, idx, bar_phys);
		if (rc)
			return rc;
	}

	/* Map base address memory */
#ifdef CONFIG_PAGING
	/* TODO: Allocate virtual address space range */
	bar_virt = bar_phys;

	pt = ukplat_pt_get_active();
	uk_pr_debug("Mapping PCI device memory in virtual address space\n");
	rc = ukplat_page_map(pt, bar_virt, bar_phys, bar_pages, attr, 0);
	if (unlikely(rc)) {
		pt->fa->ffree(pt->fa, bar_phys, bar_pages);
		return rc;
	}
#else
	(void)attr;
	/* Without paging we can just use the physical address */
	bar_virt = bar_phys;
#endif

	uk_pr_debug("Mapped PCI BAR%d to addresses [%#" PRIx64 "-%#" PRIx64 ")\n",
		    idx, bar_virt, bar_virt + bar_size);
	mem->start = (void *)bar_virt;
	mem->size = bar_size;

	return 0;
}

int pci_unmap_bar(struct pci_device *dev __unused, __u8 idx __unused,
		  struct pci_bar_memory *mem)
{
	int rc = 0;
	size_t bar_pages;
#ifdef CONFIG_PAGING
	struct uk_pagetable *pt;
#endif

	/* TODO: Do some additional checks/asserts using dev+idx */
	bar_pages = DIV_ROUND_UP(mem->size, __PAGE_SIZE);

#ifdef CONFIG_PAGING
	pt = ukplat_pt_get_active();
	rc = ukplat_page_unmap(pt, (__vaddr_t)mem->start, bar_pages, 0);
#endif
	physmem_free(ukplat_virt_to_phys(mem->start), bar_pages);

	mem->start = NULL;
	mem->size = 0;

	return rc;
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
