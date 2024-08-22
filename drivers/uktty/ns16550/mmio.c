/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "ns16550.h"

#include <uk/asm/lcpu.h>
#include <uk/assert.h>
#include <libfdt.h>
#include <uk/ofw/fdt.h>
#include <uk/config.h>
#include <uk/libparam.h>

#if CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#endif /* CONFIG_LIBUKALLOC */

#if CONFIG_PAGING
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#endif /* CONFIG_PAGING */

static const char * const fdt_compatible[] = {
	"ns16550", "ns16550a", NULL
};

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
static struct ns16550_device earlycon = {
	.con = {0},
	.reg_shift = REG_SHIFT_DEFAULT,
	.reg_width = REG_WIDTH_DEFAULT,
	.io = {
		.mmio = {
			.base = 0,
			/* Default in case it's not set in early init */
			.size = 0x1000,
		}
	},
};

UK_LIBPARAM_PARAM_ALIAS(base, &earlycon.io.mmio.base, __u64,
			"ns15550 MMIO base");
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

#define NS16550_REG(dev, r) ((dev)->io.mmio.base + ((r) << (dev)->reg_shift))

__u32 ns16550_io_read(struct ns16550_device *dev, __u32 reg)
{
	__u32 ret;

	UK_ASSERT(dev);

	switch (dev->reg_width) {
	case 1:
		ret = ioreg_read8((__u8 *)NS16550_REG(dev, reg)) & 0xff;
		break;
	case 2:
		ret = ioreg_read16((__u16 *)NS16550_REG(dev, reg)) & 0xffff;
		break;
	case 4:
		ret = ioreg_read32((__u32 *)NS16550_REG(dev, reg));
		break;
	default:
		UK_CRASH("Invalid register width: %d\n", dev->reg_width);
	}
	return ret;
}

void ns16550_io_write(struct ns16550_device *dev, __u32 reg, __u32 value)
{
	UK_ASSERT(dev);

	switch (dev->reg_width) {
	case 1:
		ioreg_write8((__u8 *)NS16550_REG(dev, reg),
			     (__u8)(value & 0xff));
		break;
	case 2:
		ioreg_write16((__u16 *)NS16550_REG(dev, reg),
			      (__u16)(value & 0xffff));
		break;
	case 4:
		ioreg_write32((__u32 *)NS16550_REG(dev, reg), value);
		break;
	default:
		UK_CRASH("Invalid register width: %d\n", dev->reg_width);
	}
}

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
int config_fdt_chosen_stdout(struct ns16550_device *dev, const void *dtb)
{
	__u64 base, size;
	char *opt;
	int offs, len;
	const __u64 *regs;
	int rc;

	UK_ASSERT(dev);
	UK_ASSERT(dtb);

	/* Check if chosen/stdout-path is set to this device */
	rc = fdt_chosen_stdout_path(dtb, &offs, &opt);
	if (unlikely(rc)) /* no stdout-path or bad dtb */
		return rc;

	rc = fdt_node_check_compatible_list(dtb, offs, fdt_compatible);
	if (unlikely(rc))
		return rc; /* no compat match or bad dtb */

	rc = fdt_get_address(dtb, offs, 0, &base, &size);
	if (unlikely(rc)) /* could not parse node */
		return rc;

	regs = fdt_getprop(dtb, offs, "reg-shift", &len);
	dev->reg_shift = regs ? fdt32_to_cpu(regs[0]) : REG_SHIFT_DEFAULT;

	regs = fdt_getprop(dtb, offs, "reg-io-width", &len);
	dev->reg_width = regs ? fdt32_to_cpu(regs[0]) : REG_WIDTH_DEFAULT;

	dev->io.mmio.base = base;
	dev->io.mmio.size = size;

	uk_pr_debug("ns16550 @ 0x%lx - 0x%lx\n", base, base + size);

	return 0;
}
#endif /* !CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

#if CONFIG_LIBUKALLOC
static int fdt_get_device(struct ns16550_device *dev, const void *dtb,
			  int *offset)
{
	__u64 base, size;
	const __u64 *regs;
	int rc, len;

	UK_ASSERT(dev);
	UK_ASSERT(dtb);
	UK_ASSERT(offset);

	*offset = fdt_node_offset_by_compatible_list(dtb, *offset,
						     fdt_compatible);
	if (unlikely(*offset < 0))
		return -ENOENT;

	rc = fdt_get_address(dtb, *offset, 0, &base, &size);
	if (unlikely(rc)) {
		uk_pr_err("Could not parse fdt node\n");
		return -EINVAL;
	}

	regs = fdt_getprop(dtb, *offset, "reg-shift", &len);
	dev->reg_shift = regs ? fdt32_to_cpu(regs[0]) : REG_SHIFT_DEFAULT;

	regs = fdt_getprop(dtb, *offset, "reg-io-width", &len);
	dev->reg_width = regs ? fdt32_to_cpu(regs[0]) : REG_WIDTH_DEFAULT;

	dev->io.mmio.base = base;
	dev->io.mmio.size = size;

	uk_pr_debug("ns16550 @ 0x%lx - 0x%lx\n", base, base + size);

	return 0;
}
#endif /* CONFIG_LIBUKALLOC */

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
int ns16550_early_init(struct ukplat_bootinfo *bi)
{
	struct ukplat_memregion_desc mrd = {0};
	int rc;

	UK_ASSERT(bi);

	/* If the base address is not set by the cmdline, try
	 * the dtb chosen/stdout-path.
	 */
	if (!earlycon.io.mmio.base) {
		rc = config_fdt_chosen_stdout(&earlycon, (void *)bi->dtb);
		if (unlikely(rc < 0 && rc != -FDT_ERR_NOTFOUND)) {
			uk_pr_err("Could not parse fdt (%d)", rc);
			return -EINVAL;
		}
	}

	/* Do not return an error if no config is detected, as
	 * another console driver may be enabled in Kconfig.
	 */
	if (!earlycon.io.mmio.base)
		return 0;

	/* Configure the port */
	rc = ns16550_configure(&earlycon);
	if (unlikely(rc)) {
		uk_pr_err("Could not initialize ns16550 (%d)\n", rc);
		return rc;
	}

	ns16550_register_console(&earlycon, UK_CONSOLE_FLAG_STDOUT |
					    UK_CONSOLE_FLAG_STDIN);

	mrd.pbase = earlycon.io.mmio.base;
	mrd.vbase = earlycon.io.mmio.base;
	mrd.pg_off = 0;
	mrd.pg_count = earlycon.io.mmio.base / PAGE_SIZE;
	mrd.len = earlycon.io.mmio.size;
	mrd.type = UKPLAT_MEMRT_DEVICE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0)) {
		uk_pr_err("Could not insert mrd (%d)\n", rc);
		return rc;
	}

	return 0;
}
#endif /* !CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

#if CONFIG_LIBUKALLOC
static int register_device(struct ns16550_device dev, struct uk_alloc *a)
{
	struct ns16550_device *console_dev;

	UK_ASSERT(a);

	console_dev = uk_malloc(a, sizeof(*console_dev));
	if (unlikely(!console_dev)) {
		uk_pr_err("Could not allocate ns16550 device\n");
		return -ENOMEM;
	}

	console_dev->con = dev.con;
	console_dev->io.mmio.base = dev.io.mmio.base;
	console_dev->io.mmio.size = dev.io.mmio.size;

	ns16550_register_console(&console_dev->con, 0);
	uk_pr_info("tty: ns16550 (%p)\n", &console_dev->io.mmio.base);

	return 0;
}
#endif /* CONFIG_LIBUKALLOC */

int ns16550_late_init(struct uk_init_ctx *ictx __unused)
{
#if CONFIG_LIBUKALLOC
	struct ukplat_bootinfo *bi;
	const void *dtb;
	int offset = -1;
	int rc;
	struct uk_alloc *a;
	struct ns16550_device dev;

	bi = ukplat_bootinfo_get();
	UK_ASSERT(bi);

	dtb = (void *)bi->dtb;
	UK_ASSERT(dtb);

	a = uk_alloc_get_default();
	UK_ASSERT(a);

	uk_pr_debug("Probing ns16550\n");

	while (1) {
		rc = fdt_get_device(&dev, dtb, &offset);
		if (unlikely(offset == -FDT_ERR_NOTFOUND))
			break; /* No more devices */

		if (unlikely(rc < 0)) {
			uk_pr_err("Could not get ns16550 device\n");
			return rc;
		}

#if CONFIG_PAGING
		/* Map device region */
		dev.io.mmio.base = uk_bus_pf_devmap(dev.io.mmio.base,
						    dev.io.mmio.size);
		if (unlikely(PTRISERR(dev.io.mmio.base))) {
			uk_pr_err("Could not map ns16550\n");
			return PTR2ERR(dev.io.mmio.base);
		}
#endif /* !CONFIG_PAGING */

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
		/* `ukconsole` mandates that there is only a single
		 * `struct uk_console` registered per device.
		 */
		if (dev.io.mmio.base == earlycon.io.mmio.base) {
			uk_pr_info("Skipping ns16550 device\n");
			continue;
		}
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

		rc = ns16550_configure(&dev);
		if (unlikely(rc)) {
			uk_pr_err("Could not initialize ns16550\n");
			return rc;
		}

		rc = register_device(dev, a);
		if (unlikely(rc < 0))
			return rc;
	}
#endif /* CONFIG_LIBUKALLOC */

	return 0;
}
