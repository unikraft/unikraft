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

static const char * const fdt_compatible[] = {
	"ns16550", "ns16550a", NULL
};

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
