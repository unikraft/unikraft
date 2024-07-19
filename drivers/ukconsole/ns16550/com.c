/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017 NEC Europe Ltd., NEC Corporation
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

#include <uk/arch/types.h>
#include <uk/console/driver.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/prio.h>
#include <uk/init.h>
#include <uk/compiler.h>
#include <uk/boot/earlytab.h>
#include <uk/bitops.h>

#if CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#endif /* CONFIG_LIBUKALLOC */

#include <uk/libparam.h>

#define COM1_PORT	0x3f8
#define COM2_PORT	0x2f8
#define COM3_PORT	0x3e8
#define COM4_PORT	0x2e8

#define COM_DATA(port_addr) ((port_addr) + 0)
#define COM_INTR(port_addr) ((port_addr) + 1)
#define COM_CTRL(port_addr) ((port_addr) + 3)
#define COM_STATUS(port_addr) ((port_addr) + 5)

/* only when DLAB is set */
#define COM_DIV_LO(port_addr) ((port_addr) + 0)
#define COM_DIV_HI(port_addr) ((port_addr) + 1)

/* baudrate divisor */
#define COM_BAUDDIV_HI 0x00

#if CONFIG_LIBUKCONSOLE_NS16550_COM1_BAUD_19200
#define COM1_BAUDDIV_LO 0x04
#elif CONFIG_LIBUKCONSOLE_NS16550_COM1_BAUD_38400
#define COM1_BAUDDIV_LO 0x03
#elif CONFIG_LIBUKCONSOLE_NS16550_COM1_BAUD_57600
#define COM1_BAUDDIV_LO 0x02
#elif CONFIG_LIBUKCONSOLE_NS16550_COM1_BAUD_115200
#define COM1_BAUDDIV_LO 0x01
#endif

#if CONFIG_LIBUKCONSOLE_NS16550_COM2_BAUD_19200
#define COM2_BAUDDIV_LO 0x04
#elif CONFIG_LIBUKCONSOLE_NS16550_COM2_BAUD_38400
#define COM2_BAUDDIV_LO 0x03
#elif CONFIG_LIBUKCONSOLE_NS16550_COM2_BAUD_57600
#define COM2_BAUDDIV_LO 0x02
#elif CONFIG_LIBUKCONSOLE_NS16550_COM2_BAUD_115200
#define COM2_BAUDDIV_LO 0x01
#endif

#if CONFIG_LIBUKCONSOLE_NS16550_COM3_BAUD_19200
#define COM3_BAUDDIV_LO 0x04
#elif CONFIG_LIBUKCONSOLE_NS16550_COM3_BAUD_38400
#define COM3_BAUDDIV_LO 0x03
#elif CONFIG_LIBUKCONSOLE_NS16550_COM3_BAUD_57600
#define COM3_BAUDDIV_LO 0x02
#elif CONFIG_LIBUKCONSOLE_NS16550_COM3_BAUD_115200
#define COM3_BAUDDIV_LO 0x01
#endif

#if CONFIG_LIBUKCONSOLE_NS16550_COM4_BAUD_19200
#define COM4_BAUDDIV_LO 0x04
#elif CONFIG_LIBUKCONSOLE_NS16550_COM4_BAUD_38400
#define COM4_BAUDDIV_LO 0x03
#elif CONFIG_LIBUKCONSOLE_NS16550_COM4_BAUD_57600
#define COM4_BAUDDIV_LO 0x02
#elif CONFIG_LIBUKCONSOLE_NS16550_COM4_BAUD_115200
#define COM4_BAUDDIV_LO 0x01
#endif

#if CONFIG_LIBUKCONSOLE_NS16550_COM1_EARLY
#define COM_EARLY_PORT	COM1_PORT
#define COM_EARLY_NAME	"COM1"
#define COM_EARLY_BAUDDIV_LO COM1_BAUDDIV_LO
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM1_EARLY */
#if CONFIG_LIBUKCONSOLE_NS16550_COM2_EARLY
#define COM_EARLY_PORT	COM2_PORT
#define COM_EARLY_NAME	"COM2"
#define COM_EARLY_BAUDDIV_LO COM2_BAUDDIV_LO
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM2_EARLY */
#if CONFIG_LIBUKCONSOLE_NS16550_COM3_EARLY
#define COM_EARLY_PORT	COM3_PORT
#define COM_EARLY_NAME	"COM3"
#define COM_EARLY_BAUDDIV_LO COM3_BAUDDIV_LO
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM3_EARLY */
#if CONFIG_LIBUKCONSOLE_NS16550_COM4_EARLY
#define COM_EARLY_PORT	COM4_PORT
#define COM_EARLY_NAME	"COM4"
#define COM_EARLY_BAUDDIV_LO COM4_BAUDDIV_LO
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM4_EARLY */

#define DLAB 0x80
#define PROT 0x03 /* 8N1 (8 bits, no parity, one stop bit) */

#define COM_STATUS_TX_READY_BIT UK_BIT(5)
#define COM_STATUS_RX_READY_BIT UK_BIT(0)

struct com_device {
	__u16 port;
	struct uk_console dev;
};

#if CONFIG_LIBUKCONSOLE_NS16550_EARLY_CONSOLE
static struct com_device early_dev;

#if CONFIG_LIBUKLIBPARAM
UK_LIBPARAM_PARAM_ALIAS(early_port, &early_dev.port, __u16,
			"early COM device port");
#endif /* CONFIG_LIBUKLIBPARAM */
#endif /* CONFIG_LIBUKCONSOLE_NS16550_EARLY_CONSOLE */

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

static inline void com_setup(int port_addr, __u8 bauddiv_lo, __u8 bauddiv_hi)
{
	/* Disable all interrupts */
	outb(COM_INTR(port_addr), 0x00);
	/* Enable DLAB (set baudrate divisor) */
	outb(COM_CTRL(port_addr), DLAB);
	/* Div (lo byte) */
	outb(COM_DIV_LO(port_addr), bauddiv_lo);
	/* Div (hi byte) */
	outb(COM_DIV_HI(port_addr), bauddiv_hi);
	/* Set 8N1, clear DLAB */
	outb(COM_CTRL(port_addr), PROT);
}

static inline int com_check_tx_empty(int port_addr)
{
	return (int)inb(COM_STATUS(port_addr)) & COM_STATUS_TX_READY_BIT;
}

static inline void com_write(int port_addr, char chr)
{
	while (!com_check_tx_empty(port_addr))
		;
	outb(COM_DATA(port_addr), chr);
}

static inline int com_check_rx_ready(int port_addr)
{
	return (int)inb(COM_STATUS(port_addr)) & COM_STATUS_RX_READY_BIT;
}

__ssz com_out(struct uk_console *dev, const char *buf, __sz len)
{
	struct com_device *com_dev;
	__sz leftover = len;

	UK_ASSERT(dev);
	UK_ASSERT(buf);

	com_dev = __containerof(dev, struct com_device, dev);

	while (leftover--) {
		if (*buf == '\n')
			com_write(com_dev->port, '\r');
		com_write(com_dev->port, *buf);
		buf++;
	}

	return len;
}

__ssz com_in(struct uk_console *dev, char *buf, __sz len)
{
	struct com_device *com_dev;
	__sz i;

	UK_ASSERT(dev);
	UK_ASSERT(buf);

	com_dev = __containerof(dev, struct com_device, dev);

	for (i = 0; i < len; i++) {
		if (!com_check_rx_ready(com_dev->port))
			return i;
		buf[i] = inb(COM_DATA(com_dev->port));
	}

	return len;
}

static struct uk_console_ops com_ops = {
	.out = com_out,
	.in = com_in
};

#if CONFIG_LIBUKCONSOLE_NS16550_EARLY_CONSOLE
int com_early_init(struct ukplat_bootinfo *bi __unused)
{
	/* Use COM_EARLY_PORT as the default if there was no
	 * command line parameter to set the early device port.
	 */
	if (!early_dev.port)
		early_dev.port = COM_EARLY_PORT;
	uk_console_init(&early_dev.dev, COM_EARLY_NAME, &com_ops,
			UK_CONSOLE_FLAG_STDOUT | UK_CONSOLE_FLAG_STDIN);
	com_setup(early_dev.port, COM_EARLY_BAUDDIV_LO, COM_BAUDDIV_HI);

	uk_console_register(&early_dev.dev);

	return 0;
}
#endif /* CONFIG_LIBUKCONSOLE_NS16550_EARLY_CONSOLE */

#if CONFIG_LIBUKALLOC
__maybe_unused
static int com_init_port(struct uk_alloc *a, const char *name, __u16 port,
			 __u8 bauddiv_lo, __u8 bauddiv_hi)
{
	struct com_device *com_dev = uk_malloc(a, sizeof(*com_dev));

	if (!com_dev)
		return -1;

	com_setup(port, bauddiv_lo, bauddiv_hi);
	com_dev->port = port;
	uk_console_init(&com_dev->dev, name, &com_ops, 0);

	uk_console_register(&com_dev->dev);

	return 0;
}
#endif /* CONFIG_LIBUKALLOC */

static int com_init(struct uk_init_ctx *ctx __unused)
{
	int rc __maybe_unused = 0;

#if CONFIG_LIBUKALLOC
	struct uk_alloc *a = uk_alloc_get_default();

	if (!a)
		return -1;

#if CONFIG_LIBUKCONSOLE_NS16550_COM1 && !CONFIG_LIBUKCONSOLE_NS16550_COM1_EARLY
	rc = com_init_port(a, "COM1", COM1_PORT,
			   COM1_BAUDDIV_LO, COM_BAUDDIV_HI);
	if (unlikely(rc < 0))
		return rc;
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM1 &&
	* !CONFIG_LIBUKCONSOLE_NS16550_COM1_EARLY
	*/
#if CONFIG_LIBUKCONSOLE_NS16550_COM2 && !CONFIG_LIBUKCONSOLE_NS16550_COM2_EARLY
	rc = com_init_port(a, "COM2", COM2_PORT,
			   COM2_BAUDDIV_LO, COM_BAUDDIV_HI);
	if (unlikely(rc < 0))
		return rc;
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM2 &&
	* !CONFIG_LIBUKCONSOLE_NS16550_COM2_EARLY
	*/
#if CONFIG_LIBUKCONSOLE_NS16550_COM3 && !CONFIG_LIBUKCONSOLE_NS16550_COM3_EARLY
	rc = com_init_port(a, "COM3", COM3_PORT,
			   COM3_BAUDDIV_LO, COM_BAUDDIV_HI);
	if (unlikely(rc < 0))
		return rc;
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM3 &&
	* !CONFIG_LIBUKCONSOLE_NS16550_COM3_EARLY
	*/
#if CONFIG_LIBUKCONSOLE_NS16550_COM4 && !CONFIG_LIBUKCONSOLE_NS16550_COM4_EARLY
	rc = com_init_port(a, "COM4", COM4_PORT,
			   COM4_BAUDDIV_LO, COM_BAUDDIV_HI);
	if (unlikely(rc < 0))
		return rc;
#endif /* CONFIG_LIBUKCONSOLE_NS16550_COM4 &&
	* !CONFIG_LIBUKCONSOLE_NS16550_COM4_EARLY
	*/

#endif /* CONFIG_LIBUKALLOC */

	return 0;
}

#if CONFIG_LIBUKCONSOLE_NS16550_EARLY_CONSOLE
UK_BOOT_EARLYTAB_ENTRY(com_early_init, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#endif /* CONFIG_LIBUKCONSOLE_NS16550_EARLY_CONSOLE */

uk_plat_initcall_prio(com_init, 0, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
