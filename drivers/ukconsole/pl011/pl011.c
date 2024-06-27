/* SPDX-License-Identifier: ISC */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2018 Arm Ltd.
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
#include <libfdt.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/init.h>
#include <uk/libparam.h>
#include <uk/ofw/fdt.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/console/driver.h>
#include <uk/compiler.h>
#include <uk/errptr.h>

#if CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#endif /* CONFIG_LIBUKALLOC */

#if CONFIG_PAGING
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#endif /* CONFIG_PAGING */

#if CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE */

/* PL011 UART registers and masks*/
/* Data register */
#define REG_UARTDR_OFFSET	0x00

/* Receive status register/error clear register */
#define REG_UARTRSR_OFFSET	0x04
#define REG_UARTECR_OFFSET	0x04

/* Flag register */
#define REG_UARTFR_OFFSET	0x18
#define FR_TXFF			UK_BIT(5)    /* Transmit FIFO/reg full */
#define FR_RXFE			UK_BIT(4)    /* Receive FIFO/reg empty */

/* Integer baud rate register */
#define REG_UARTIBRD_OFFSET	0x24
/* Fractional baud rate register */
#define REG_UARTFBRD_OFFSET	0x28

/* Line control register */
#define REG_UARTLCR_H_OFFSET	0x2C
#define LCR_H_WLEN8		(0x3 << 5)  /* Data width is 8-bits */

/* Control register */
#define REG_UARTCR_OFFSET	0x30
#define CR_RXE			UK_BIT(9)    /* Receive enable */
#define CR_TXE			UK_BIT(8)    /* Transmit enable */
#define CR_UARTEN		UK_BIT(0)    /* UART enable */

/* Interrupt FIFO level select register */
#define REG_UARTIFLS_OFFSET	0x34
/* Interrupt mask set/clear register */
#define REG_UARTIMSC_OFFSET	0x38
/* Raw interrupt status register */
#define REG_UARTRIS_OFFSET	0x3C
/* Masked interrupt status register */
#define REG_UARTMIS_OFFSET	0x40
/* Interrupt clear register */
#define REG_UARTICR_OFFSET	0x44

static const char * const fdt_compatible[] = {
	"arm,pl011", NULL
};

struct pl011_device {
	struct uk_console dev;
	__u64 base, size;
};

#if CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE
static struct pl011_device earlycon;

UK_LIBPARAM_PARAM_ALIAS(base, &earlycon.base, __u64, "pl011 base");
#endif /* CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE */

/* Macros to access PL011 Registers with base address */
#define PL011_REG(base, r)		((__u16 *)((base) + (r)))
#define PL011_REG_READ(base, r)		ioreg_read16(PL011_REG(base, r))
#define PL011_REG_WRITE(base, r, v)	ioreg_write16(PL011_REG(base, r), v)

static void pl011_putc(__u64 base, char a)
{
	/* Wait until TX FIFO becomes empty */
	while (PL011_REG_READ(base, REG_UARTFR_OFFSET) & FR_TXFF)
		;

	PL011_REG_WRITE(base, REG_UARTDR_OFFSET, a & 0xff);
}

/* Try to get data from pl011 UART without blocking */
static int pl011_getc(__u64 base)
{
	/* If RX FIFO is empty, return -1 immediately */
	if (PL011_REG_READ(base, REG_UARTFR_OFFSET) & FR_RXFE)
		return -1;

	return (int)(PL011_REG_READ(base, REG_UARTDR_OFFSET) & 0xff);
}

static __ssz pl011_out(struct uk_console *dev, const char *buf, __sz len)
{
	struct pl011_device *pl011_dev;
	__sz l = len;

	UK_ASSERT(dev);
	UK_ASSERT(buf);

	pl011_dev = __containerof(dev, struct pl011_device, dev);

	while (l--)
		pl011_putc(pl011_dev->base, *buf++);

	return len;
}

static __ssz pl011_in(struct uk_console *dev, char *buf, __sz len)
{
	int rc;
	struct pl011_device *pl011_dev;

	UK_ASSERT(dev);
	UK_ASSERT(buf);

	pl011_dev = __containerof(dev, struct pl011_device, dev);

	for (__sz i = 0; i < len; i++) {
		if ((rc = pl011_getc(pl011_dev->base)) < 0)
			return i;
		buf[i] = (char)rc;
	}

	return len;
}

static struct uk_console_ops pl011_ops = {
	.out = pl011_out,
	.in = pl011_in
};

static int pl011_setup(__u64 base)
{
	/* Mask all interrupts */
	PL011_REG_WRITE(base, REG_UARTIMSC_OFFSET,
			PL011_REG_READ(base, REG_UARTIMSC_OFFSET) & 0xf800);

	/* Clear all interrupts */
	PL011_REG_WRITE(base, REG_UARTICR_OFFSET, 0x07ff);

	/* Disable UART for configuration */
	PL011_REG_WRITE(base, REG_UARTCR_OFFSET, 0);

	/* Select 8-bits data transmit and receive */
	PL011_REG_WRITE(base, REG_UARTLCR_H_OFFSET,
			(PL011_REG_READ(base, REG_UARTLCR_H_OFFSET) & 0xff00) |
			 LCR_H_WLEN8);

	/* Just enable UART and data transmit/receive */
	PL011_REG_WRITE(base, REG_UARTCR_OFFSET, CR_TXE | CR_UARTEN);

	return 0;
}

#if CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE
static inline int config_fdt_chosen_stdout(const void *dtb)
{
	__u64 base, size;
	char *opt;
	int offs;
	int rc;

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

	earlycon.base = base;
	earlycon.size = size;

	return 0;
}

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
		if (unlikely(rc < 0 && rc != -FDT_ERR_NOTFOUND))  {
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
	rc = pl011_setup(earlycon.base);
	if (unlikely(rc))
		return rc;

	uk_console_init(&earlycon.dev, "PL011",  &pl011_ops,
			UK_CONSOLE_FLAG_STDOUT | UK_CONSOLE_FLAG_STDIN);
	uk_console_register(&earlycon.dev);

	/* Add an mrd to keep the device mapped past init */
	mrd.pbase = earlycon.base;
	mrd.vbase = earlycon.base;
	mrd.pg_off = 0;
	mrd.pg_count = 1;
	mrd.len = __PAGE_SIZE;
	mrd.type = UKPLAT_MEMRT_DEVICE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0)) {
		uk_pr_err("Could not insert mrd (%d)\n", rc);
		return rc;
	}

	return 0;
}
#endif /* CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE */

#if CONFIG_LIBUKALLOC
static int fdt_get_device(struct pl011_device *dev, const void *dtb,
			  int *offset)
{
	__u64 reg_base, reg_size;
	int rc;

	UK_ASSERT(dev);
	UK_ASSERT(dtb);
	UK_ASSERT(offset);

	*offset = fdt_node_offset_by_compatible_list(dtb, *offset,
						     fdt_compatible);
	if (unlikely(*offset < 0))
		return -ENOENT;

	rc = fdt_get_address(dtb, *offset, 0, &reg_base, &reg_size);
	if (unlikely(rc)) {
		uk_pr_err("Could not parse fdt node\n");
		return -EINVAL;
	}

	uk_pr_debug("pl011 @ 0x%lx - 0x%lx\n", reg_base, reg_base + reg_size);

	uk_console_init(&dev->dev, "PL011", &pl011_ops, 0);
	dev->base = reg_base;
	dev->size = reg_size;

	return 0;
}

static int register_device(struct pl011_device *dev, struct uk_alloc *a)
{
	struct pl011_device *console_dev;

	UK_ASSERT(dev);
	UK_ASSERT(a);

	console_dev = uk_malloc(a, sizeof(*console_dev));
	if (unlikely(!console_dev)) {
		uk_pr_err("Could not allocate pl011 device\n");
		return -ENOMEM;
	}

	console_dev->dev = dev->dev;
	console_dev->base = dev->base;
	console_dev->size = dev->size;

	uk_console_register(&console_dev->dev);
	uk_pr_info("con%"__PRIu16": pl011 @ %p\n", console_dev->dev.id,
		   &console_dev->base);

	return 0;
}
#endif /* CONFIG_LIBUKALLOC */

static int init(struct uk_init_ctx *ictx __unused)
{
#if CONFIG_LIBUKALLOC
	struct ukplat_bootinfo *bi;
	const void *dtb;
	int offset = -1;
	int rc;
	struct pl011_device dev;
	struct uk_alloc *a;

	bi = ukplat_bootinfo_get();
	UK_ASSERT(bi);

	dtb = (void *)bi->dtb;
	UK_ASSERT(dtb);

	a = uk_alloc_get_default();
	UK_ASSERT(a);

	uk_pr_debug("Probing pl011\n");

	while (1) {
		rc = fdt_get_device(&dev, dtb, &offset);
		if (unlikely(offset == -FDT_ERR_NOTFOUND))
			break; /* No more devices */

		if (unlikely(rc < 0)) {
			uk_pr_err("Could not get pl011 device\n");
			return rc;
		}

#if CONFIG_PAGING
		/* Map device region */
		dev.base = uk_bus_pf_devmap(dev.base, dev.size);
		if (unlikely(PTRISERR(dev.base))) {
			uk_pr_err("Could not map pl011\n");
			return PTR2ERR(dev.base);
		}
#endif /* !CONFIG_PAGING */

#if CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE
		/* `ukconsole` mandates that there is only a single
		 * `struct uk_console` registered per device.
		 */
		if (dev.base == earlycon.base) {
			uk_pr_info("Skipping pl011 device\n");
			continue;
		}
#endif /* CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE */

		rc = pl011_setup(dev.base);
		if (unlikely(rc)) {
			uk_pr_err("Could not initialize pl011\n");
			return rc;
		}

		rc = register_device(&dev, a);
		if (unlikely(rc < 0))
			return rc;
	}

#endif /* CONFIG_LIBUKALLOC */

	return 0;
}

#if CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE
UK_BOOT_EARLYTAB_ENTRY(early_init, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#endif /* CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE */

/* UK_PRIO_EARLIEST reserved for cmdline */
uk_plat_initcall_prio(init, 0, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
