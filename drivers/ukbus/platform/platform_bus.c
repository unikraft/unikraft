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

#include <string.h>
#include <uk/print.h>
#include <uk/plat/memory.h>
#include <uk/plat/common/cpu.h>
#include <uk/bus/platform.h>
#include <uk/plat/common/bootinfo.h>

#if CONFIG_PAGING
#include <uk/errptr.h>
#include <uk/plat/paging.h>
#endif /* CONFIG_PAGING */

#if CONFIG_LIBUKBUS_PLATFORM_FDT
#include <libfdt.h>
#include <uk/ofw/fdt.h>

static void *dtb;

static const char *pf_device_compatible_list[] = {
	"virtio,mmio",
	"pci-host-ecam-generic",
	"arm,pl031",
	NULL
};
#endif /* CONFIG_LIBUKBUS_PLATFORM_FDT */

static struct pf_bus_handler pfh;

struct pf_bus_handler {
	struct uk_bus b;
	struct uk_alloc *a;
	struct pf_driver_list drv_list;  /**< List of platform drivers */
	int drv_list_initialized;
	struct pf_device_list dev_list;  /**< List of platform devices */
};

static inline int pf_device_id_match(const struct pf_device_id *id0,
					const struct pf_device_id *id1)
{
	int rc = 0;

	if (id0->device_id == id1->device_id)
		rc = 1;

	return rc;
}

static inline struct pf_driver *pf_find_driver(const char *compatible)
{
	struct pf_driver *drv;
	struct pf_device_id id;

	UK_TAILQ_FOREACH(drv, &pfh.drv_list, next) {
		if (!drv->match)
			continue;

		id.device_id = (__u16)drv->match(compatible);
		if (id.device_id >= PLATFORM_DEVICE_ID_START &&
			id.device_id < PLATFORM_DEVICE_ID_END) {
			if (pf_device_id_match(&id, drv->device_ids)) {
				uk_pr_debug("pf driver found devid(0x%x)\n", id.device_id);

				return drv;
			}
		}
	}

	uk_pr_info("no pf driver found\n");

	return NULL; /* no driver found */
}

static inline int pf_driver_add_device(struct pf_driver *drv,
					struct pf_device *dev)
{
	int ret;

	UK_ASSERT(drv != NULL);
	UK_ASSERT(drv->add_dev != NULL);
	UK_ASSERT(dev != NULL);

	uk_pr_debug("pf_driver_add_device devid=%d\n", dev->id.device_id);

	ret = drv->add_dev(dev);
	if (ret < 0 && ret != -ENODEV)
		uk_pr_err("Platform Failed to initialize device driver, ret(%d)\n", ret);

	return ret;
}

static inline int pf_driver_probe_device(struct pf_driver *drv,
					struct pf_device *dev)
{
	int ret;

	UK_ASSERT(drv != NULL && dev != NULL);
	UK_ASSERT(drv->probe != NULL);

	uk_pr_info("pf_driver_probe_device devid=%d\n", dev->id.device_id);

	ret = drv->probe(dev);
	if (ret < 0) {
		uk_pr_err("Platform Failed to probe device driver\n");

		return ret;
	}

	return 0;
}

#if CONFIG_LIBUKBUS_PLATFORM_FDT
static int pf_probe_fdt(void)
{
	struct pf_driver *drv;
	int idx = 0;
	int ret = -ENODEV;
	struct pf_device *dev;
	int fdt_pf = -1;

	uk_pr_info("Probe PF\n");

	dtb = (void *)ukplat_bootinfo_get()->dtb;

	/* Search all the platform bus devices provided by fdt */
	do {
		fdt_pf = fdt_node_offset_idx_by_compatible_list(dtb,
						fdt_pf, pf_device_compatible_list, &idx);
		if (fdt_pf < 0) {
			uk_pr_info("End of searching platform devices\n");
			break;
		}

		/* Alloc dev */
		dev = (struct pf_device *) uk_calloc(pfh.a, 1, sizeof(*dev));
		if (unlikely(!dev)) {
			uk_pr_err("Platform : Failed to initialize: Out of memory!\n");
			return -ENOMEM;
		}

		dev->fdt_offset = fdt_pf;

		/* Find drv with compatible-id match table */
		drv = pf_find_driver(pf_device_compatible_list[idx]);
		if (!drv) {
			uk_free(pfh.a, dev);
			continue;
		}

		dev->id = *(struct pf_device_id *)drv->device_ids;
		uk_pr_info("driver %p\n", drv);

		ret = pf_driver_probe_device(drv, dev);
		if (ret < 0) {
			uk_free(pfh.a, dev);
			continue;
		}

		ret = pf_driver_add_device(drv, dev);
		if (ret < 0)
			uk_free(pfh.a, dev);
	} while (1);

	return ret;
}
#endif /* CONFIG_LIBUKBUS_PLATFORM_FDT */

static int pf_probe(void)
{
	int rc = -ENODEV;

#if CONFIG_LIBUKBUS_PLATFORM_FDT
	rc = pf_probe_fdt();
#endif /* CONFIG_LIBUKBUS_PLATFORM_FDT */

	return rc;
}

static int pf_init(struct uk_alloc *a)
{
	struct pf_driver *drv, *drv_next;
	int ret = 0;

	UK_ASSERT(a != NULL);

	pfh.a = a;

	if (!pfh.drv_list_initialized) {
		UK_TAILQ_INIT(&pfh.drv_list);
		pfh.drv_list_initialized = 1;
	}
	UK_TAILQ_INIT(&pfh.dev_list);

	UK_TAILQ_FOREACH_SAFE(drv, &pfh.drv_list, next, drv_next) {
		if (drv->init) {
			ret = drv->init(a);
			if (ret == 0)
				continue;
			uk_pr_err("Failed to initialize pf driver %p: %d\n",
				  drv, ret);
			UK_TAILQ_REMOVE(&pfh.drv_list, drv, next);
		}
	}
	return 0;
}

#if CONFIG_PAGING
__vaddr_t uk_bus_pf_devmap(__paddr_t base, __sz size)
{
	struct uk_pagetable *pt;
	__vaddr_t vaddr, paddr;
	unsigned long attr;
	unsigned long pages;
	int rc;

	attr = PAGE_ATTR_PROT_RW;
#ifdef CONFIG_ARCH_ARM_64
	attr |= PAGE_ATTR_TYPE_DEVICE_nGnRnE;
#endif /* CONFIG_ARCH_ARM_64 */

	pages = ALIGN_UP(size, __PAGE_SIZE) >> PAGE_SHIFT;

	pt = ukplat_pt_get_active();

	paddr = ALIGN_DOWN(base, __PAGE_SIZE);
	vaddr = ALIGN_DOWN(base, __PAGE_SIZE);

	/* We can't use uk_vma_map_dma here as the vmem API is initialized
	 * way too late for the interrupt controller which is required for
	 * bringing up secondary cores.
	 */
	rc = ukplat_page_map(pt, vaddr, paddr, pages, attr, 0);
	if (!rc)
		goto out;

	if (rc == -EEXIST)
		rc = ukplat_page_set_attr(pt, vaddr, pages, attr, 0);

	if (unlikely(rc))
		return (__vaddr_t)ERR2PTR(rc);
out:
	return (__vaddr_t)base;
}
#endif /* CONFIG_PAGING */

void _pf_register_driver(struct pf_driver *drv)
{
	UK_ASSERT(drv != NULL);

	if (!pfh.drv_list_initialized) {
		UK_TAILQ_INIT(&pfh.drv_list);
		pfh.drv_list_initialized = 1;
	}
	UK_TAILQ_INSERT_TAIL(&pfh.drv_list, drv, next);
}

/* Register this bus driver to libukbus:
 */
static struct pf_bus_handler pfh = {
	.b.init = pf_init,
	.b.probe = pf_probe
};

UK_BUS_REGISTER_PRIORITY(&pfh.b, 1);
