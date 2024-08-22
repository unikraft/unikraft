/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "ns16550.h"

#include <uk/libparam.h>

#define COM1_PORT	0x3f8
#define COM2_PORT	0x2f8
#define COM3_PORT	0x3e8
#define COM4_PORT	0x2e8

#if CONFIG_LIBUKTTY_NS16550_COM1_EARLY
#define COM_EARLY_PORT	COM1_PORT
#endif /* CONFIG_LIBUKTTY_NS16550_COM1_EARLY */
#if CONFIG_LIBUKTTY_NS16550_COM2_EARLY
#define COM_EARLY_PORT	COM2_PORT
#endif /* CONFIG_LIBUKTTY_NS16550_COM2_EARLY */
#if CONFIG_LIBUKTTY_NS16550_COM3_EARLY
#define COM_EARLY_PORT	COM3_PORT
#endif /* CONFIG_LIBUKTTY_NS16550_COM3_EARLY */
#if CONFIG_LIBUKTTY_NS16550_COM4_EARLY
#define COM_EARLY_PORT	COM4_PORT
#endif /* CONFIG_LIBUKTTY_NS16550_COM4_EARLY */

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
static struct ns16550_device earlycon = {
	.con = {0},
	.reg_shift = REG_SHIFT_DEFAULT,
	.reg_width = REG_WIDTH_DEFAULT,
	.io = {
		.port_io = {
			.port = 0
		}
	},
};

UK_LIBPARAM_PARAM_ALIAS(port, &earlycon.io.port_io.port, __u16,
			"ns15550 IO port");
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

static inline void outb(__u16 port, __u8 v)
{
	__asm__ __volatile__("outb %0,%1" : : "a"(v), "dN"(port));
}

static inline __u8 inb(__u16 port)
{
	__u8 v;

	__asm__ __volatile__("inb %1,%0" : "=a"(v) : "dN"(port));
	return v;
}

static inline void outw(__u16 port, __u16 v)
{
	__asm__ __volatile__("outw %0,%1" : : "a"(v), "dN"(port));
}

static inline __u16 inw(__u16 port)
{
	__u16 v;

	__asm__ __volatile__("inw %1,%0" : "=a"(v) : "dN"(port));
	return v;
}

static inline void outl(__u16 port, __u32 v)
{
	__asm__ __volatile__("outl %0,%1" : : "a"(v), "dN"(port));
}

static inline __u32 inl(__u16 port)
{
	__u32 v;

	__asm__ __volatile__("inl %1,%0" : "=a"(v) : "dN"(port));
	return v;
}

#define NS16550_REG(dev, r) ((dev)->io.port_io.port + ((r) << (dev)->reg_shift))

__u32 ns16550_io_read(struct ns16550_device *dev, __u32 reg)
{
	__u32 ret;

	UK_ASSERT(dev);

	switch (dev->reg_width) {
	case 1:
		ret = inb(NS16550_REG(dev, reg));
		break;
	case 2:
		ret = inw(NS16550_REG(dev, reg));
		break;
	case 4:
		ret = inl(NS16550_REG(dev, reg));
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
		outb(NS16550_REG(dev, reg), (__u8)(value & 0xff));
		break;
	case 2:
		outw(NS16550_REG(dev, reg), (__u16)(value & 0xffff));
		break;
	case 4:
		outl(NS16550_REG(dev, reg), value);
		break;
	default:
		UK_CRASH("Invalid register width: %d\n", dev->reg_width);
	}
}

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
int ns16550_early_init(struct ukplat_bootinfo *bi __unused)
{
#if COM_EARLY_PORT
	/* Use COM_EARLY_PORT as the default if there was no
	 * command line parameter to set the early device port.
	 */
	if (!earlycon.io.port_io.port)
		earlycon.io.port_io.port = COM_EARLY_PORT;
#endif /* COM_EARLY_PORT */

	/* Do not return an error if no config is detected, as
	 * another console driver may be enabled in Kconfig.
	 */
	if (!earlycon.io.port_io.port)
		return 0;

	ns16550_configure(&earlycon);
	ns16550_register(&earlycon, UK_CONSOLE_FLAG_STDOUT |
			 UK_CONSOLE_FLAG_STDIN);

	return 0;
}
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */
