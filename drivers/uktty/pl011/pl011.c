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
#include <uk/bitops.h>
#include <uk/plat/console.h>
#include <uk/assert.h>

#if CONFIG_PAGING
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#endif /* CONFIG_PAGING */

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

/*
 * PL011 UART base address
 * As we are using the PA = VA mapping, some SoC would set PA 0
 * as a valid address, so we can't use pl011_base == 0 to
 * indicate PL011 hasn't been initialized. In this case, we
 * use pl011_uart_initialized as an extra variable to check
 * whether the UART has been initialized.
 */

#if defined(CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE_BASE)
static __u8 pl011_uart_initialized = 1;
static __u64 pl011_base = CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE_BASE;
#else /* !CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE_BASE */
static __u8 pl011_uart_initialized;
static __u64 pl011_base;
#endif /* !CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE_BASE */

/* Macros to access PL011 Registers with base address */
#define PL011_REG(r)		((__u16 *)(pl011_base + (r)))
#define PL011_REG_READ(r)	ioreg_read16(PL011_REG(r))
#define PL011_REG_WRITE(r, v)	ioreg_write16(PL011_REG(r), v)

static int init_pl011(void)
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

int pl011_console_init(void *dtb)
{
	int offset, len, naddr, nsize;
	__sz size __maybe_unused;
	const __u64 *regs;
	__u64 reg_base;
	__u64 reg_size;
	int rc;

	UK_ASSERT(dtb);

	uk_pr_debug("Probing pl011\n");

	offset = fdt_node_offset_by_compatible(dtb, -1, "arm,pl011");
	if (unlikely(offset < 0)) {
		uk_pr_err("pl011 not found in fdt\n");
		return -ENODEV;
	}

	naddr = fdt_address_cells(dtb, offset);
	if (unlikely(naddr < 0 || naddr >= FDT_MAX_NCELLS)) {
		uk_pr_err("Invalid address-cells\n");
		return -EINVAL;
	}

	nsize = fdt_size_cells(dtb, offset);
	if (unlikely(nsize < 0 || nsize >= FDT_MAX_NCELLS)) {
		uk_pr_err("Invalid size-cells\n");
		return -EINVAL;
	}

	regs = fdt_getprop(dtb, offset, "reg", &len);
	if (unlikely(!regs || (len < (int)sizeof(fdt32_t) * (naddr + nsize)))) {
		uk_pr_err("Invalid 'reg' property: %p %d\n", regs, len);
		return -EINVAL;
	}

	reg_base = fdt64_to_cpu(regs[0]);
	reg_size = ALIGN_UP(fdt64_to_cpu(regs[1]), __PAGE_SIZE);

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

	rc = init_pl011();
	if (unlikely(rc)) {
		uk_pr_err("Could not initialize pl011\n");
		return rc;
	}

	pl011_uart_initialized = 1;
	uk_pr_info("tty: pl011\n");

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
	 * or CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE is not enabled.
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
	 * or CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE is not enabled.
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
