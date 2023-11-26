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
#include <uk/print.h>
#include <uk/plat/common/cpu.h>
#include <uk/bus/pci.h>

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


/* Register this bus driver to libukbus:
 */
static struct pci_bus_handler ph = {
	.b.init = pci_init,
	.b.probe = pci_probe,
	.drv_list = UK_LIST_HEAD_INIT(ph.drv_list),
	.dev_list = UK_LIST_HEAD_INIT(ph.dev_list),
};
UK_BUS_REGISTER(&ph.b);
