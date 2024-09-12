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
#include <uk/plat/console.h>

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

/*
 * PL011 UART base address
 * As we are using the PA = VA mapping, some SoC would set PA 0
 * as a valid address, so we can't use pl011_base == 0 to
 * indicate PL011 hasn't been initialized. In this case, we
 * use pl011_uart_initialized as an extra variable to check
 * whether the UART has been initialized.
 */
static __u8 pl011_uart_initialized;
static __u64 pl011_base;

UK_LIBPARAM_PARAM_ALIAS(base, &pl011_base, __u64, "pl011 base");

/* Macros to access PL011 Registers with base address */
#define PL011_REG(r)		((__u16 *)(pl011_base + (r)))
#define PL011_REG_READ(r)	ioreg_read16(PL011_REG(r))
#define PL011_REG_WRITE(r, v)	ioreg_write16(PL011_REG(r), v)

static int pl011_setup(void)
{
	/* Mask all interrupts */
	PL011_REG_WRITE(REG_UARTIMSC_OFFSET,
			PL011_REG_READ(REG_UARTIMSC_OFFSET) & 0xf800);

	/* Clear all interrupts */
	PL011_REG_WRITE(REG_UARTICR_OFFSET, 0x07ff);

	/* Disable UART for configuration */
	PL011_REG_WRITE(REG_UARTCR_OFFSET, 0);

	/* Select 8-bits data transmit and receive */
	PL011_REG_WRITE(REG_UARTLCR_H_OFFSET,
			(PL011_REG_READ(REG_UARTLCR_H_OFFSET) & 0xff00) |
			 LCR_H_WLEN8);

	/* Just enable UART and data transmit/receive */
	PL011_REG_WRITE(REG_UARTCR_OFFSET, CR_TXE | CR_UARTEN);

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

	pl011_base = base;

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
	if (!pl011_base) {
		rc = config_fdt_chosen_stdout((void *)bi->dtb);
		if (unlikely(rc < 0 && rc != -FDT_ERR_NOTFOUND))  {
			uk_pr_err("Could not parse fdt (%d)", rc);
			return -EINVAL;
		}
	}

	/* Do not return an error if no config is detected, as
	 * another console driver may be enabled in Kconfig.
	 */
	if (!pl011_base)
		return 0;

	/* Configure the port */
	rc = pl011_setup();
	if (unlikely(rc))
		return rc;

	/* From this point we can print */
	pl011_uart_initialized = 1;

	mrd.pbase = pl011_base;
	mrd.vbase = pl011_base;
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

static int init(struct uk_init_ctx *ictx __unused)
{
	struct ukplat_bootinfo *bi;
	__sz size __maybe_unused;
	__u64 reg_base;
	__u64 reg_size;
	int offset;
	int rc;

	bi = ukplat_bootinfo_get();
	UK_ASSERT(bi);

	/* Pick the 1st device in the dtb */
	offset = fdt_node_offset_by_compatible_list((void *)bi->dtb, -1,
						    fdt_compatible);
	if (unlikely(offset < 0)) {
		uk_pr_err("pl011 not found in fdt\n");
		return -ENODEV;
	}

	rc = fdt_get_address((void *)bi->dtb, offset, 0, &reg_base, &reg_size);
	if (unlikely(rc)) {
		uk_pr_err("Could not parse fdt node\n");
		return rc;
	}

	uk_pr_debug("pl011 @ 0x%lx - 0x%lx\n", reg_base, reg_base + reg_size);

#if CONFIG_PAGING
	/* Map device region */
	pl011_base = uk_bus_pf_devmap(reg_base, reg_size);
	if (unlikely(PTRISERR(pl011_base))) {
		uk_pr_err("Could not map pl011\n");
		return PTR2ERR(pl011_base);
	}
#else /* !CONFIG_PAGING */
	pl011_base = reg_base;
#endif /* !CONFIG_PAGING */

	rc = pl011_setup();
	if (unlikely(rc)) {
		uk_pr_err("Could not initialize pl011\n");
		return rc;
	}

	pl011_uart_initialized = 1;
	uk_pr_info("console: pl011\n");

	return 0;
}

int ukplat_coutd(const char *str, __u32 len)
{
	return ukplat_coutk(str, len);
}

static void pl011_write(char a)
{
	/*
	 * Avoid using the UART before base address initialized,
	 * or CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE is not enabled.
	 */
	if (!pl011_uart_initialized)
		return;

	/* Wait until TX FIFO becomes empty */
	while (PL011_REG_READ(REG_UARTFR_OFFSET) & FR_TXFF)
		;

	PL011_REG_WRITE(REG_UARTDR_OFFSET, a & 0xff);
}

static void pl011_putc(char a)
{
	if (a == '\n')
		pl011_write('\r');
	pl011_write(a);
}

/* Try to get data from pl011 UART without blocking */
static int pl011_getc(void)
{
	/*
	 * Avoid using the UART before base address initialized,
	 * or CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE is not enabled.
	 */
	if (!pl011_uart_initialized)
		return -1;

	/* If RX FIFO is empty, return -1 immediately */
	if (PL011_REG_READ(REG_UARTFR_OFFSET) & FR_RXFE)
		return -1;

	return (int)(PL011_REG_READ(REG_UARTDR_OFFSET) & 0xff);
}

int ukplat_coutk(const char *buf, unsigned int len)
{
	for (unsigned int i = 0; i < len; i++)
		pl011_putc(buf[i]);
	return len;
}

int ukplat_cink(char *buf, unsigned int maxlen)
{
	int ret;
	unsigned int num = 0;

	while (num < maxlen && (ret = pl011_getc()) >= 0) {
		*(buf++) = (char)ret;
		num++;
	}

	return (int)num;
}

#if CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE
UK_BOOT_EARLYTAB_ENTRY(early_init, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#endif /* CONFIG_LIBUKCONSOLE_PL011_EARLY_CONSOLE */

/* UK_PRIO_EARLIEST reserved for cmdline */
uk_plat_initcall_prio(init, 0, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
