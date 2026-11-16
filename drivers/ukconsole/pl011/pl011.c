/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2018 Arm Ltd.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <libfdt.h>
#include <string.h>
#include <uk/assert.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
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

#if CONFIG_LIBUKPAGING
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#endif /* CONFIG_LIBUKPAGING */

#if CONFIG_LIBUKFIXMAP
#include <uk/fixmap.h>
#endif /* CONFIG_LIBUKFIXMAP */

#if CONFIG_LIBPL011_EARLY_CONSOLE
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBPL011_EARLY_CONSOLE */

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
	"arm,pl011", "arm,sbsa-uart", NULL
};

struct pl011_device {
	struct uk_console dev;
	__paddr_t pbase;
	__vaddr_t vbase;
	__sz size;
	__bool is_sbsa;
};

#if CONFIG_LIBPL011_EARLY_CONSOLE
static struct pl011_device earlycon;

UK_LIBPARAM_PARAM_ALIAS(base, &earlycon.pbase, __u64, "pl011 base");
#endif /* CONFIG_LIBPL011_EARLY_CONSOLE */

/* Macros to access PL011 Registers with base address */
#define PL011_REG(base, r)		((__u16 *)((base) + (r)))

#define PL011_REG_READ(base, r)		\
	uk_arch_arm64_ioreg_read16(PL011_REG(base, r))

#define PL011_REG_WRITE(base, r, v)	\
	uk_arch_arm64_ioreg_write16(PL011_REG(base, r), v)

#define PL011_REG_UPDATE(base, r, v, m) \
	PL011_REG_WRITE(base, r,	\
			(PL011_REG_READ(base, r) & (~(m))) | ((v) & (m)))

__isr static void pl011_putc(__u64 base, char a)
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

__isr static __ssz pl011_out(struct uk_console *dev, const char *buf, __sz len)
{
	struct pl011_device *pl011_dev;
	__sz l = len;

	UK_ASSERT(dev);
	UK_ASSERT(buf);

	pl011_dev = __containerof(dev, struct pl011_device, dev);

	while (l--)
		pl011_putc(pl011_dev->vbase, *buf++);

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
		if ((rc = pl011_getc(pl011_dev->vbase)) < 0)
			return i;
		buf[i] = (char)rc;
	}

	return len;
}

static const struct uk_console_ops pl011_ops = {
	.out = pl011_out,
	.in = pl011_in,
	.emerg_out = pl011_out,
};

#if CONFIG_LIBUKFIXMAP
UK_FIXMAP_IMPORT(pl011_fixmap, PL011);
#endif /* CONFIG_LIBUKFIXMAP */

static inline int pl011_map(struct pl011_device *dev,
			    __bool __maybe_unused early)
{
	__vaddr_t vaddr = dev->pbase;

#if CONFIG_LIBUKFIXMAP
	vaddr = uk_fixmap_map(pl011_fixmap, dev->pbase, dev->size);
#elif CONFIG_LIBUKPAGING
	if (!early)
		vaddr = uk_bus_pf_devmap(dev->pbase, dev->size);
#endif /* CONFIG_LIBUKPAGING */
	if (unlikely(PTRISERR(vaddr)))
		return PTR2ERR(vaddr);

	dev->vbase = vaddr;

	return 0;
}

static int pl011_setup(struct pl011_device *dev, __bool early)
{
	int rc = pl011_map(dev, early);

	if (unlikely(rc)) {
		uk_pr_err("Could not map pl011 (%d)\n", rc);
		return rc;
	}

	/* Mask all interrupts */
	PL011_REG_UPDATE(dev->vbase, REG_UARTIMSC_OFFSET, 0, 0x07ff);

	/* Clear all interrupts */
	PL011_REG_WRITE(dev->vbase, REG_UARTICR_OFFSET, 0x07ff);

	/* The SBSA UART has no line/control registers to program */
	if (dev->is_sbsa)
		return 0;

	/* Disable UART for configuration */
	PL011_REG_WRITE(dev->vbase, REG_UARTCR_OFFSET, 0);

	/* Select 8-bits data transmit and receive */
	PL011_REG_UPDATE(dev->vbase, REG_UARTLCR_H_OFFSET, LCR_H_WLEN8, 0x00ff);

	/* Just enable UART and data transmit/receive */
	PL011_REG_WRITE(dev->vbase, REG_UARTCR_OFFSET, CR_TXE | CR_UARTEN);

	return 0;
}

#if CONFIG_LIBPL011_EARLY_CONSOLE
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

	earlycon.pbase = base;
	earlycon.size = size;
	earlycon.is_sbsa = !fdt_node_check_compatible(dtb, offs,
						      "arm,sbsa-uart");

	return 0;
}

static int early_init(struct ukplat_bootinfo *bi)
{
	struct ukplat_memregion_desc mrd __maybe_unused = {0};
	int rc;

	UK_ASSERT(bi);

	/* If the base is not set by the cmdline, try
	 * the dtb chosen/stdout-path.
	 */
	if (!earlycon.pbase) {
		rc = config_fdt_chosen_stdout((void *)bi->dtb);
		if (unlikely(rc < 0 && rc != -FDT_ERR_NOTFOUND))  {
			uk_pr_err("Could not parse fdt (%d)", rc);
			return -EINVAL;
		}
	}

	/* Do not return an error if no config is detected, as
	 * another console driver may be enabled in Kconfig.
	 */
	if (!earlycon.pbase)
		return 0;

	/* The base may come from the cmdline, which does not give a size */
	if (!earlycon.size)
		earlycon.size = __PAGE_SIZE;

	/* Configure the port */
	rc = pl011_setup(&earlycon, true);
	if (unlikely(rc))
		return rc;

	uk_console_init(&earlycon.dev, "PL011",  &pl011_ops,
			UK_CONSOLE_FLAG_STDOUT | UK_CONSOLE_FLAG_STDIN,
			UK_CONSOLE_CLASS_UART);
	uk_console_register(&earlycon.dev);

#if !CONFIG_LIBUKFIXMAP
	/* Add an mrd to keep the device mapped past paging init. Fixmap
	 * slots stay mapped, so they do not need one.
	 */
	mrd.pbase = earlycon.pbase;
	mrd.vbase = earlycon.vbase;
	mrd.pg_off = 0;
	mrd.pg_count = DIV_ROUND_UP(earlycon.size, __PAGE_SIZE);
	mrd.len = earlycon.size;
	mrd.type = UKPLAT_MEMRT_DEVICE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0)) {
		uk_pr_err("Could not insert mrd (%d)\n", rc);
		return rc;
	}
#endif /* !CONFIG_LIBUKFIXMAP */

	return 0;
}
#endif /* CONFIG_LIBPL011_EARLY_CONSOLE */

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

	uk_console_init(&dev->dev, "PL011", &pl011_ops, 0,
			UK_CONSOLE_CLASS_UART);
	dev->pbase = reg_base;
	dev->size = reg_size;
	dev->is_sbsa = !fdt_node_check_compatible(dtb, *offset,
						  "arm,sbsa-uart");

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

	memcpy(console_dev, dev, sizeof(*console_dev));
	uk_console_register(&console_dev->dev);
	uk_pr_info("con%"__PRIu16": pl011 @ %p\n", console_dev->dev.id,
		   (void *)console_dev->pbase);

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
	struct pl011_device dev = {0};
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

#if CONFIG_LIBPL011_EARLY_CONSOLE
		/* `ukconsole` mandates that there is only a single
		 * `struct uk_console` registered per device.
		 */
		if (dev.pbase == earlycon.pbase) {
			uk_pr_info("Skipping earlycon pl011 device\n");
			continue;
		}
#endif /* CONFIG_LIBPL011_EARLY_CONSOLE */

		rc = pl011_setup(&dev, false);
		if (unlikely(rc)) {
			uk_pr_err("Could not initialize pl011 @ %p; skipping\n",
				  (void *)dev.pbase);
			continue;
		}

		rc = register_device(&dev, a);
		if (unlikely(rc < 0))
			return rc;
	}

#endif /* CONFIG_LIBUKALLOC */

	return 0;
}

#if CONFIG_LIBPL011_EARLY_CONSOLE
UK_BOOT_EARLYTAB_ENTRY(early_init, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#endif /* CONFIG_LIBPL011_EARLY_CONSOLE */

/* UK_PRIO_EARLIEST reserved for cmdline */
uk_plat_initcall_prio(init, 0, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
