/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, OpenSynergy GmbH. All rights reserved.
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
#include <errno.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <gic/gic.h>
#include <gic/gic-v2.h>
#include <gic/gic-v3.h>
#include <uk/plat/common/acpi.h>

/** Sanity check for GIC driver availability */
#if !defined(CONFIG_LIBGICV2) && !defined(CONFIG_LIBGICV3)
#error At least one GIC driver should be selected!
#endif

int init_gic(struct _gic_dev **dev)
{
	int rc;

#ifdef CONFIG_LIBGICV2
	/* First, try GICv2 */
	rc = gicv2_probe(dev);
	if (rc == 0)
		return 0;
#endif /* CONFIG_LIBGICV2 */

#ifdef CONFIG_LIBGICV3
	/* GICv2 is not present, try GICv3 */
	rc = gicv3_probe(dev);
	if (rc == 0)
		return 0;
#endif /* CONFIG_LIBGICV3 */

	*dev = NULL;
	return rc;
}

uint32_t gic_irq_translate(uint32_t type, uint32_t irq)
{
	UK_ASSERT(type == GIC_SPI_TYPE || type == GIC_PPI_TYPE);

	switch (type) {
	case GIC_SPI_TYPE:
		UK_ASSERT((irq + GIC_SPI_BASE) < GIC_MAX_IRQ);
		return irq + GIC_SPI_BASE;

	case GIC_PPI_TYPE:
		UK_ASSERT((irq + GIC_PPI_BASE) < GIC_SPI_BASE);
		return irq + GIC_PPI_BASE;
	}

	return (uint32_t)-1;
}

#if defined(CONFIG_UKPLAT_ACPI)
int acpi_get_gicd(struct _gic_dev *g)
{
	union {
		acpi_madt_gicd_t *gicd;
		acpi_subsdt_hdr_t *h;
	} m;
	acpi_madt_t *madt;
	__sz off, len;

	madt = acpi_get_madt();
	UK_ASSERT(madt);

	len = madt->hdr.tab_len - sizeof(*madt);
	for (off = 0; off < len; off += m.h->len) {
		m.h = (acpi_subsdt_hdr_t *)(madt->entries + off);

		if (m.h->type != ACPI_MADT_GICD)
			continue;

		if (m.gicd->version == ACPI_MADT_GICD_VERSION_2)
			g->dist_mem_size = GICD_V2_MEM_SZ;
		else if (m.gicd->version == ACPI_MADT_GICD_VERSION_3)
			g->dist_mem_size = GICD_V3_MEM_SZ;
		else
			return -ENOTSUP;

		g->dist_mem_addr = m.gicd->paddr;

		/* Only one GIC Distributor */
		return 0;
	}

	return -ENOENT;
}
#endif /* CONFIG_UKPLAT_ACPI */
