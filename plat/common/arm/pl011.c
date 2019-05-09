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
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <arm/cpu.h>

/* TODO: For now this file is KVM dependent. As soon as we have more
 * Arm platforms that are using this file, we need to introduce a
 * portable way to handover the DTB entry point to common platform code */
#include <kvm/config.h>

/* PL011 UART registers and masks*/
/* Data register */
#define REG_UARTDR_OFFSET	0x00

/* Receive status register/error clear register */
#define REG_UARTRSR_OFFSET	0x04
#define REG_UARTECR_OFFSET	0x04

/* Flag register */
#define REG_UARTFR_OFFSET	0x18
#define FR_TXFF			(1 << 5)    /* Transmit FIFO/reg full */
#define FR_RXFE			(1 << 4)    /* Receive FIFO/reg empty */

/* Integer baud rate register */
#define REG_UARTIBRD_OFFSET	0x24
/* Fractional baud rate register */
#define REG_UARTFBRD_OFFSET	0x28

/* Line control register */
#define REG_UARTLCR_H_OFFSET	0x2C
#define LCR_H_WLEN8		(0x3 << 5)  /* Data width is 8-bits */

/* Control register */
#define REG_UARTCR_OFFSET	0x30
#define CR_RXE			(1 << 9)    /* Receive enable */
#define CR_TXE			(1 << 8)    /* Transmit enable */
#define CR_UARTEN		(1 << 0)    /* UART enable */

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
 * as a valid address, so we can't use pl011_uart_bas == 0 to
 * indicate PL011 hasn't been initialized. In this case, we
 * use pl011_uart_initialized as an extra variable to check
 * whether the UART has been initialized.
 */
#if defined(CONFIG_EARLY_PRINT_PL011_UART_ADDR)
static uint8_t pl011_uart_initialized = 1;
static uint64_t pl011_uart_bas = CONFIG_EARLY_PRINT_PL011_UART_ADDR;
#else
static uint8_t pl011_uart_initialized = 0;
static uint64_t pl011_uart_bas = 0;
#endif

/* Macros to access PL011 Registers with base address */
#define PL011_REG(r)		((uint16_t *)(pl011_uart_bas + (r)))
#define PL011_REG_READ(r)	ioreg_read16(PL011_REG(r))
#define PL011_REG_WRITE(r, v)	ioreg_write16(PL011_REG(r), v)

static void init_pl011(uint64_t bas)
{
	pl011_uart_bas = bas;

	/* Mask all interrupts */
	PL011_REG_WRITE(REG_UARTIMSC_OFFSET, \
		PL011_REG_READ(REG_UARTIMSC_OFFSET) & 0xf800);

	/* Clear all interrupts */
	PL011_REG_WRITE(REG_UARTICR_OFFSET, 0x07ff);

	/* Disable UART for configuration */
	PL011_REG_WRITE(REG_UARTCR_OFFSET, 0);

	/* Select 8-bits data transmit and receive */
	PL011_REG_WRITE(REG_UARTLCR_H_OFFSET, \
		(PL011_REG_READ(REG_UARTLCR_H_OFFSET) & 0xff00) | LCR_H_WLEN8);

	/* Just enable UART and data transmit/receive */
	PL011_REG_WRITE(REG_UARTCR_OFFSET, CR_TXE | CR_UARTEN);
}

void _libkvmplat_init_console(void)
{
	int offset, len, naddr, nsize;
	const uint64_t *regs;
	uint64_t reg_uart_bas;

	uk_pr_info("Serial initializing\n");

	offset = fdt_node_offset_by_compatible(_libkvmplat_cfg.dtb, \
					-1, "arm,pl011");
	if (offset < 0)
		UK_CRASH("No console UART found!\n");

	naddr = fdt_address_cells(_libkvmplat_cfg.dtb, offset);
	if (naddr < 0 || naddr >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper address cells!\n");

	nsize = fdt_size_cells(_libkvmplat_cfg.dtb, offset);
	if (nsize < 0 || nsize >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper size cells!\n");

	regs = fdt_getprop(_libkvmplat_cfg.dtb, offset, "reg", &len);
	if (regs == NULL || (len < (int)sizeof(fdt32_t) * (naddr + nsize)))
		UK_CRASH("Bad 'reg' property: %p %d\n", regs, len);

	reg_uart_bas = fdt64_to_cpu(regs[0]);
	uk_pr_info("Found PL011 UART on: 0x%lx\n", reg_uart_bas);

	init_pl011(reg_uart_bas);
	uk_pr_info("PL011 UART initialized\n");
}

int ukplat_coutd(const char *str, uint32_t len)
{
	return ukplat_coutk(str, len);
}

static void pl011_write(char a)
{
	/*
	 * Avoid using the UART before base address initialized,
	 * or CONFIG_KVM_EARLY_DEBUG_PL011_UART doesn't be enabled.
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
	 * or CONFIG_KVM_EARLY_DEBUG_PL011_UART doesn't be enabled.
	 */
	if (!pl011_uart_initialized)
		return -1;

	/* If RX FIFO is empty, return -1 immediately */
	if (PL011_REG_READ(REG_UARTFR_OFFSET) & FR_RXFE)
		return -1;

	return (int) (PL011_REG_READ(REG_UARTDR_OFFSET) & 0xff);
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
		*(buf++) = (char) ret;
		num++;
	}

	return (int) num;
}
