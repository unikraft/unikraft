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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <string.h>
#include <uk/print.h>
#include <uk/plat/common/cpu.h>
#include <platform_bus.h>
#include <libfdt.h>
#include <kvm/config.h>
#include <gic/gic-v2.h>
#include <ofw/fdt.h>

#define fdt_start (_libkvmplat_cfg.dtb)

struct pf_bus_handler {
	struct uk_bus b;
	struct uk_alloc *a;
	struct pf_driver_list drv_list;  /**< List of platform drivers */
	int drv_list_initialized;
	struct pf_device_list dev_list;  /**< List of platform devices */
};
static struct pf_bus_handler pfh;

static const char * const pf_device_list[] = {
	"virtio,mmio",
};

static inline int pf_device_id_match(const struct pf_device_id *id0,
					const struct pf_device_id *id1)
{
	int rc = 0;

	if (id0->device_id == id1->device_id)
		rc = 1;

	return rc;
}

static inline struct pf_driver *pf_find_driver(struct pf_device_id *id)
{
	struct pf_driver *drv;

	UK_TAILQ_FOREACH(drv, &pfh.drv_list, next) {
		if (pf_device_id_match(id, drv->device_ids)) {
			uk_pr_debug("pf driver found devid=%d\n", drv->device_ids->device_id);
			return drv; /* driver found */
		}
	}

	uk_pr_info("no pf driver found\n");

	return NULL; /* no driver found */
}

static inline int pf_driver_add_device(struct pf_driver *drv,
					struct pf_device_id *devid,
					__u64 dev_base,
					int dev_irq)
{
	struct pf_device *dev;
	int ret;

	UK_ASSERT(drv != NULL);
	UK_ASSERT(drv->add_dev != NULL);

	dev = (struct pf_device *) uk_calloc(pfh.a, 1, sizeof(*dev));
	if (!dev) {
		uk_pr_err("Platform : Failed to initialize: Out of memory!\n");
		return -ENOMEM;
	}

	memcpy(&dev->id, devid, sizeof(dev->id));
	uk_pr_debug("pf_driver_add_device dev->id=%d\n", dev->id.device_id);

	dev->base = dev_base;
	dev->irq = dev_irq;

	ret = drv->add_dev(dev);
	if (ret < 0) {
		uk_pr_err("Platform Failed to initialize device driver\n");
		uk_free(pfh.a, dev);
	}

	return ret;
}

static int pf_probe(void)
{
	struct pf_device_id devid;
	struct pf_driver *drv;
	int i;
	int end_offset = -1;
	int ret = -ENODEV;
	const fdt32_t *prop;
	int type, hwirq, prop_len;
	__u64 reg_base;
	__phys_addr dev_base;
	int dev_irq;

	uk_pr_info("Probe PF\n");

	/* We only support virtio_mmio as a platform device here.
	 * A loop here is needed for finding drivers if more devices
	 */
	devid.device_id = VIRTIO_MMIO_ID;

	drv = pf_find_driver(&devid);
	if (!drv) {
		uk_pr_info("<no driver>\n");
		return -ENODEV;
	}

	uk_pr_info("driver %p\n", drv);

	/* qemu creates virtio devices in reverse order */
	for (i = 0; i < UK_MAX_VIRTIO_MMIO_DEVICE; i++) {
		end_offset = fdt_node_offset_by_compatible_list(fdt_start,
							end_offset,
							pf_device_list);
		if (end_offset == -FDT_ERR_NOTFOUND) {
			uk_pr_info("device not found in fdt\n");
			goto error_exit;
		} else {
			prop = fdt_getprop(fdt_start, end_offset, "interrupts", &prop_len);
			if (!prop) {
				uk_pr_err("irq of device not found in fdt\n");
				goto error_exit;
			}

			type = fdt32_to_cpu(prop[0]);
			hwirq = fdt32_to_cpu(prop[1]);

			prop = fdt_getprop(fdt_start, end_offset, "reg", &prop_len);
			if (!prop) {
				uk_pr_err("reg of device not found in fdt\n");
				goto error_exit;
			}

			/* only care about base addr, ignore the size */
			reg_base = fdt32_to_cpu(prop[0]);
			reg_base = reg_base << 32 | fdt32_to_cpu(prop[1]);
		}

		dev_base = reg_base;
		dev_irq = gic_irq_translate(type, hwirq);

		ret = pf_driver_add_device(drv, &devid, dev_base, dev_irq);
	}

	return ret;

error_exit:
	return -ENODEV;
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

void _pf_register_driver(struct pf_driver *drv)
{
	UK_ASSERT(drv != NULL);
	uk_pr_debug("_pf_register_driver %p\n", drv);

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
UK_BUS_REGISTER(&pfh.b);
