/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021 OpenSynergy GmbH
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
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/init.h>
#include <uk/libparam.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/console.h>
#include <uk/compiler.h>
#include <uk/errptr.h>

#include "ns16550.h"

#if CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#endif /* CONFIG_LIBUKALLOC */

#if CONFIG_PAGING
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#endif /* CONFIG_PAGING */

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

#define NS16550_THR_OFFSET	0x00U
#define NS16550_RBR_OFFSET	0x00U
#define NS16550_DLL_OFFSET	0x00U
#define NS16550_IER_OFFSET	0x01U
#define NS16550_DLM_OFFSET	0x00U
#define NS16550_IIR_OFFSET	0x02U
#define NS16550_FCR_OFFSET	0x02U
#define NS16550_LCR_OFFSET	0x03U
#define NS16550_MCR_OFFSET	0x04U
#define NS16550_LSR_OFFSET	0x05U
#define NS16550_MSR_OFFSET	0x06U

#define NS16550_LCR_WL		0x03U
#define NS16550_LCR_STOP	0x04U
#define NS16550_LCR_PARITY	0x38U
#define NS16550_LCR_BREAK	0x40U
#define NS16550_LCR_DLAB	0x80U

#define NS16550_LCR_8N1		0x03U

/* Assume 1.8432MHz clock */
#define NS16550_DLL_115200	0x01U
#define NS16550_DLM_115200	0x00U

#define NS16550_IIR_NO_INT	0x01U
#define NS16550_FCR_FIFO_EN	0x01U
#define NS16550_LSR_RX_EMPTY	0x01U
#define NS16550_LSR_TX_EMPTY	0x40U

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
static struct ns16550_device earlycon;

UK_LIBPARAM_PARAM_ALIAS(base, &earlycon.base, __u64, "ns15550 base");
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

static void _putc(struct ns16550_device *dev, char a)
{
	/* Wait until TX FIFO becomes empty */
	while (!(ns16550_io_read(dev, NS16550_LSR_OFFSET) &
		 NS16550_LSR_TX_EMPTY))
		;

	/* Reset DLAB and write to THR */
	ns16550_io_write(dev, NS16550_LCR_OFFSET,
			 ns16550_io_read(dev, NS16550_LCR_OFFSET) &
			 ~(NS16550_LCR_DLAB));
	ns16550_io_write(dev, NS16550_THR_OFFSET, a & 0xff);
}

static void ns16550_putc(struct ns16550_device *dev, char a)
{
	if (a == '\n')
		_putc(dev, '\r');
	_putc(dev, a);
}

/* Try to get data from ns16550 UART without blocking */
static int ns16550_getc(struct ns16550_device *dev)
{
	/* If RX FIFO is empty, return -1 immediately */
	if (!(ns16550_io_read(dev, NS16550_LSR_OFFSET) &
	      NS16550_LSR_RX_EMPTY))
		return -1;

	/* Reset DLAB and read from RBR */
	ns16550_io_write(dev, NS16550_LCR_OFFSET,
			 ns16550_io_read(dev, NS16550_LCR_OFFSET) &
			 ~(NS16550_LCR_DLAB));
	return (int)(ns16550_io_read(dev, NS16550_RBR_OFFSET) & 0xff);
}

static __ssz ns16550_out(struct uk_console *con, const char *buf, __sz len)
{
	struct ns16550_device *ns16550_dev;
	__sz l = len;

	UK_ASSERT(con);
	UK_ASSERT(buf);

	ns16550_dev = __containerof(con, struct ns16550_device, con);

	while (l--)
		ns16550_putc(ns16550_dev, *buf++);

	return len;
}

static __ssz ns16550_in(struct uk_console *con, char *buf, __sz len)
{
	struct ns16550_device *ns16550_dev;
	int rc;

	UK_ASSERT(con);
	UK_ASSERT(buf);

	ns16550_dev = __containerof(con, struct ns16550_device, con);

	for (__sz i = 0; i < len; i++) {
		if ((rc = ns16550_getc(ns16550_dev)) < 0)
			return i;
		buf[i] = (char)rc;
	}

	return len;
}

static struct uk_console_ops ns16550_ops = {
	.out  = ns16550_out,
	.in = ns16550_in
};

static int init_ns16550(struct ns16550_device *dev)
{
	__u32 lcr;

	UK_ASSERT(dev);

	/* Clear DLAB to access IER, FCR, LCR */
	ns16550_io_write(dev, NS16550_LCR_OFFSET,
			 ns16550_io_read(dev, NS16550_LCR_OFFSET) &
			 ~(NS16550_LCR_DLAB));

	/* Disable all interrupts */
	ns16550_io_write(dev, NS16550_IER_OFFSET,
			 ns16550_io_read(dev, NS16550_FCR_OFFSET) &
			 ~(NS16550_IIR_NO_INT));

	/* Disable FIFOs */
	ns16550_io_write(dev, NS16550_FCR_OFFSET,
			 ns16550_io_read(dev, NS16550_FCR_OFFSET) &
			 ~(NS16550_FCR_FIFO_EN));

	/* Set line control parameters (8n1) */
	lcr = ns16550_io_read(dev, NS16550_LCR_OFFSET) |
	      ~(NS16550_LCR_WL | NS16550_LCR_STOP | NS16550_LCR_PARITY);
	lcr |= NS16550_LCR_8N1;
	ns16550_io_write(dev, NS16550_LCR_OFFSET, lcr);

	/* Set DLAB to access DLL / DLM */
	ns16550_io_write(dev, NS16550_LCR_OFFSET,
			 ns16550_io_read(dev, NS16550_LCR_OFFSET) &
			 ~(NS16550_LCR_DLAB));

	/* Set baud (115200) */
	ns16550_io_write(dev, NS16550_DLL_OFFSET, NS16550_DLL_115200);
	ns16550_io_write(dev, NS16550_DLM_OFFSET, NS16550_DLM_115200);

	return 0;
}

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
static int early_init(struct ukplat_bootinfo *bi)
{
	struct ukplat_memregion_desc mrd = {0};
	int rc;

	UK_ASSERT(bi);

	/* If the base is not set by the cmdline, try
	 * the dtb chosen/stdout-path.
	 */
	if (!earlycon.base) {
		rc = config_fdt_chosen_stdout((void *)bi->dtb);
		if (unlikely(rc < 0 && rc != -FDT_ERR_NOTFOUND)) {
			uk_pr_err("Could not parse fdt (%d)", rc);
			return -EINVAL;
		}
	}

	/* Do not return an error if no config is detected, as
	 * another console driver may be enabled in Kconfig.
	 */
	if (!earlycon.base)
		return 0;

	/* Configure the port */
	rc = init_ns16550(earlycon.base);
	if (unlikely(rc)) {
		uk_pr_err("Could not initialize ns16550 (%d)\n", rc);
		return rc;
	}

	earlycon.dev = UK_CONSOLE("NS16550", &ns16550_ops,
				  UK_CONSOLE_FLAG_STDOUT |
				  UK_CONSOLE_FLAG_STDIN);
	uk_console_register(&earlycon.dev);

	mrd.pbase = earlycon.base;
	mrd.vbase = earlycon.base;
	mrd.pg_off = 0;
	mrd.pg_count = 1;
	mrd.len = 0x1000;
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
static int register_device(struct ns16550_device *dev, struct uk_alloc *a)
{
	struct ns16550_device *console_dev;

	UK_ASSERT(dev);
	UK_ASSERT(a);

	console_dev = uk_malloc(a, sizeof(*console_dev));
	if (unlikely(!console_dev)) {
		uk_pr_err("Could not allocate ns16550 device\n");
		return -ENOMEM;
	}

	console_dev->dev = dev->dev;
	console_dev->base = dev->base;
	console_dev->size = dev->size;

	uk_console_register(&console_dev->dev);
	uk_pr_info("tty: ns16550 (%p)\n", &console_dev->base);

	return 0;
}
#endif /* CONFIG_LIBUKALLOC */

static int init(struct uk_init_ctx *ictx __unused)
{
#if CONFIG_LIBUKALLOC
	struct ukplat_bootinfo *bi;
	const __u64 *regs;
	const void *dtb;
	int offset = -1, len;
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
		dev.base = uk_bus_pf_devmap(dev.base, dev.size);
		if (unlikely(PTRISERR(dev.base))) {
			uk_pr_err("Could not map ns16550\n");
			return PTR2ERR(dev.base);
		}
#endif /* !CONFIG_PAGING */

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
		/* `ukconsole` mandates that there is only a single
		 * `struct uk_console` registered per device.
		 */
		if (dev.base == earlycon.base) {
			uk_pr_info("Skipping ns16550 device\n");
			continue;
		}
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

		regs = fdt_getprop(dtb, offset, "reg-shift", &len);
		if (regs)
			ns16550_reg_shift = fdt32_to_cpu(regs[0]);

		regs = fdt_getprop(dtb, offset, "reg-io-width", &len);
		if (regs)
			ns16550_reg_width = fdt32_to_cpu(regs[0]);

		rc = init_ns16550(dev.base);
		if (unlikely(rc)) {
			uk_pr_err("Could not initialize ns16550\n");
			return rc;
		}

		rc = register_device(&dev, a);
		if (unlikely(rc < 0))
			return rc;
	}
#endif /* CONFIG_LIBUKALLOC */

	return 0;
}

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
UK_BOOT_EARLYTAB_ENTRY(early_init, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#endif /* !config_libuktty_ns16550_early_console */

/* UK_PRIO_EARLIEST reserved for cmdline */
uk_plat_initcall_prio(init, 0, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
