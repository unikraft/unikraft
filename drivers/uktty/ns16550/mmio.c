/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "ns16550.h"

#include <uk/asm/lcpu.h>
#include <uk/assert.h>

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
