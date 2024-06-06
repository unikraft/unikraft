/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Jianyong Wu <Jianyong.Wu@arm.com>
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
 *
 */
#include <string.h>
#include <uk/essentials.h>
#include <libfdt.h>
#include <uk/ofw/fdt.h>
#include <uk/intctlr.h>
#include <uk/print.h>
#include <arm/cpu.h>
#include <uk/rtc.h>
#include <uk/pl031.h>
#include <uk/bus/platform.h>
#include <uk/plat/common/bootinfo.h>

#if CONFIG_PAGING
#include <uk/errptr.h>
#endif /* CONFIG_PAGING */

static __u64 pl031_base_addr;
static int pl031_irq;
static struct uk_alloc *a;

/* FDT compatible property for PL031 device */
#define PL031_COMPATIBLE	"arm,pl031"

/* status mask to check if device is enabled */
#define PL031_RTC_CR_STATUS_MASK	1

/* offsets of PL031 registers */
#define RTC_DR		0x00	/* Data read */
#define RTC_MR		0x04	/* Match register */
#define RTC_LR		0x08	/* Load register */
#define RTC_CR		0x0c	/* Control register */
#define RTC_IMSC	0x10	/* Interrupt mask and set register */
#define RTC_RIS		0x14	/* Raw interrupt status */
#define RTC_MIS		0x18	/* Masked interrupt status */
#define RTC_ICR		0x1c	/* Interrupt clear register */

#define PL031_REG(r)	(void *)(pl031_base_addr + (r))

static __u32 pl031_read_raw(void)
{
	return ioreg_read32(PL031_REG(RTC_DR));
}

void pl031_read_time(struct rtc_time *rt)
{
	__u32 raw;

	raw = pl031_read_raw();
	rtc_raw_to_tm(raw, rt);
}

static void pl031_write_raw(__u32 val)
{
	ioreg_write32(PL031_REG(RTC_LR), val);
}

void pl031_write_time(struct rtc_time *rt)
{
	__u32 raw;

	raw = rtc_tm_to_raw(rt);
	pl031_write_raw(raw);
}

static void pl031_write_alarm_raw(__u32 alarm)
{
	ioreg_write32(PL031_REG(RTC_MR), alarm);
}

void pl031_write_alarm(struct rtc_time *rt)
{
	__u32 raw;

	raw = rtc_tm_to_raw(rt);
	pl031_write_alarm_raw(raw);
}

static __u32 pl031_read_alarm_raw(void)
{
	return ioreg_read32(PL031_REG(RTC_MR));
}

void pl031_read_alarm(struct rtc_time *rt)
{
	rtc_raw_to_tm(pl031_read_alarm_raw(), rt);
}

void pl031_enable(void)
{
	ioreg_write32(PL031_REG(RTC_CR), 1);
}

void pl031_disable(void)
{
	ioreg_write32(PL031_REG(RTC_CR), 0);
}

int pl031_get_status(void)
{
	int val;

	val = ioreg_read32(PL031_REG(RTC_CR));
	val &= PL031_RTC_CR_STATUS_MASK;
	return val;
}

void pl031_enable_intr(void)
{
	ioreg_write32(PL031_REG(RTC_IMSC), 1);
}

void pl031_disable_intr(void)
{
	ioreg_write32(PL031_REG(RTC_IMSC), 0);
}

static __u32 pl031_get_raw_intr_state(void)
{
	return ioreg_read32(PL031_REG(RTC_RIS));
}

void pl031_clear_intr(void)
{
	while (pl031_get_raw_intr_state())
		ioreg_write32(PL031_REG(RTC_ICR), 1);
}

int pl031_register_alarm_handler(int (*handler)(void *))
{
	return uk_intctlr_irq_register(pl031_irq, handler, NULL);
}

int pl031_init_rtc(void *dtb)
{
	__u64 size;
	int offs, rc;
	struct uk_intctlr_irq irq;

	uk_pr_info("Probing RTC...\n");

	offs = fdt_node_offset_by_compatible(dtb, -1, PL031_COMPATIBLE);
	if (unlikely(offs < 0)) {
		uk_pr_err("Could not find RTC node (%d)\n", offs);
		return -EINVAL;
	}

	rc = fdt_get_address(dtb, offs, 0, &pl031_base_addr, &size);
	if (unlikely(rc < 0)) {
		uk_pr_err("Could not get RTC address\n");
		return -EINVAL;
	}
	uk_pr_info("Found RTC at: 0x%lx\n", pl031_base_addr);

#if CONFIG_PAGING
	pl031_base_addr = uk_bus_pf_devmap(pl031_base_addr, size);
	if (unlikely(PTRISERR(pl031_base_addr)))
		return PTR2ERR(pl031_base_addr);
#endif /* CONFIG_PAGING */

	rc = uk_intctlr_irq_fdt_xlat(dtb, offs, 0, &irq);
	if (unlikely(rc))
		return rc;

	uk_intctlr_irq_configure(&irq);

	pl031_irq = irq.id;

	if (pl031_get_status() == PL031_STATUS_DISABLED)
		pl031_enable();

	if (unlikely(pl031_get_status() != PL031_STATUS_ENABLED)) {
		uk_pr_err("Fail to enable RTC\n");
		return -EINVAL;
	}
	uk_pr_info("RTC enabled\n");

	/* Enable RTC alarm IRQ at its reset. */
	pl031_enable_intr();

	return 0;
}

static int pl031_probe(struct pf_device *pfdev __unused)
{
	void *dtb;

	dtb = (void *)ukplat_bootinfo_get()->dtb;

	pl031_init_rtc(dtb);

	return 0;
}

static int pl031_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;

	return 0;
}

static int pl031_add_dev(struct pf_device *pfdev __unused)
{
	return 0;
}

static const struct device_match_table pl031_match_table[];

static int pl031_id_match_compatible(const char *compatible)
{
	for (int i = 0; pl031_match_table[i].compatible; i++)
		if (strcmp(pl031_match_table[i].compatible, compatible) == 0)
			return pl031_match_table[i].id->device_id;

	return -1;
}

static struct pf_device_id pl031_ids = {
		.device_id = PL031_RTC_ID
};

static struct pf_driver pl031_driver = {
	.device_ids = &pl031_ids,
	.init = pl031_drv_init,
	.probe = pl031_probe,
	.add_dev = pl031_add_dev,
	.match = pl031_id_match_compatible
};

static const struct device_match_table pl031_match_table[] = {
	{ .compatible = PL031_COMPATIBLE,
	  .id = &pl031_ids },
	{NULL}
};

PF_REGISTER_DRIVER(&pl031_driver);
