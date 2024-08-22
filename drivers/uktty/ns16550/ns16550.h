/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKTTY_NS16550_IO_H__
#define __UKTTY_NS16550_IO_H__

#include <uk/arch/types.h>
#include <uk/console.h>
#include <uk/init.h>
#include <uk/plat/common/bootinfo.h>

union ns16550_io {
	struct {
		__u64 base;
		__u64 size;
	} mmio;
	struct {
		__u16 port;
	} port_io;
};

struct ns16550_device {
	struct uk_console con;
	__u32 reg_shift;
	__u32 reg_width;
	union ns16550_io io;
};

/* The register shift. Default is 0 (device-tree spec v0.4 Sect. 4.2.2) */
#define REG_SHIFT_DEFAULT	0
/* The register width. Default is 1 (8-bit register width) */
#define REG_WIDTH_DEFAULT	1

__u32 ns16550_io_read(struct ns16550_device *dev, __u32 reg);
void ns16550_io_write(struct ns16550_device *dev, __u32 reg, __u32 value);

int ns16550_configure(struct ns16550_device *dev);
void ns16550_register(struct ns16550_device *dev, int flags);

int ns16550_early_init(struct ukplat_bootinfo *bi);

#endif /* __UKTTY_NS16550_IO_H__ */
