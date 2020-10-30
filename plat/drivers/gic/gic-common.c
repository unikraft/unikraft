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
#include <uk/print.h>
#include <gic/gic.h>
#include <gic/gic-v2.h>
#include <gic/gic-v3.h>

/** Sanity check for GIC driver availability */
#if !defined(CONFIG_LIBGICV2) && !defined(CONFIG_LIBGICV3)
#error At least one GIC driver should be selected!
#endif

/**
 * Initialize GIC driver from device tree
 * @param [in] fdt Pointer to fdt structure
 * @return Pointer to the corresponding driver
 */
struct _gic_dev *_dtb_init_gic(const void *fdt)
{
	int ret;
	struct _gic_dev *gdev;

	uk_pr_info("Probing GIC...\n");

#ifdef CONFIG_LIBGICV2
	/* First, try GICv2 */
	gdev = gicv2_probe(fdt, &ret);
	if (gdev)
		return gdev;
#endif

#ifdef CONFIG_LIBGICV3
	/* GICv2 is not present, try GICv3 */
	gdev = gicv3_probe(fdt, &ret);
	if (gdev)
		return gdev;
#endif

	return NULL;
}

int32_t gic_irq_translate(uint32_t type, uint32_t hw_irq)
{
	uint32_t irq;

	switch (type) {
	case GIC_SPI_TYPE:
		irq = hw_irq + GIC_SPI_BASE;
		if (irq >= GIC_SPI_BASE && irq < GIC_MAX_IRQ)
			return irq;
		break;
	case GIC_PPI_TYPE:
		irq = hw_irq + GIC_PPI_BASE;
		if (irq >= GIC_PPI_BASE && irq < GIC_SPI_BASE)
			return irq;
		break;
	default:
		uk_pr_warn("Invalid IRQ type [%d]\n", type);
	}

	uk_pr_err("irq is out of range\n");
	return -EINVAL;
}

