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
#include <libfdt.h>
#include <uk/config.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <arm/cpu.h>

#define NS16550_THR_OFFSET	0x00U
#define NS16550_RBR_OFFSET	0x00U
#define NS16550_IER_OFFSET	0x01U
#define NS16550_IIR_OFFSET	0x02U
#define NS16550_FCR_OFFSET	0x02U
#define NS16550_LCR_OFFSET	0x03U
#define NS16550_MCR_OFFSET	0x04U
#define NS16550_LSR_OFFSET	0x05U
#define NS16550_MSR_OFFSET	0x06U

#define NS16550_REG_SHIFT	0x02U

#define NS16550_LCR_DLAB	0x80U
#define NS16550_IIR_NO_INT	0x01U
#define NS16550_FCR_FIFO_EN	0x01U
#define NS16550_LSR_RX_EMPTY	0x01U
#define NS16550_LSR_TX_EMPTY	0x40U

/*
 * NS16550 UART base address
 * As we are using the PA = VA mapping, some SoC would set PA 0
 * as a valid address, so we can't use ns16550_uart_base == 0 to
 * indicate NS16550 hasn't been initialized. In this case, we
 * use ns16550_uart_initialized as an extra variable to check
 * whether the UART has been initialized.
 */
#if defined(CONFIG_EARLY_UART_NS16550) && defined(CONFIG_EARLY_UART_NS16550_BASE)
static uint8_t ns16550_uart_initialized = 1;
static uint64_t ns16550_uart_base = CONFIG_EARLY_UART_NS16550_BASE;
#else
static uint8_t ns16550_uart_initialized;
static uint64_t ns16550_uart_base;
#endif

/* Macros to access ns16550 registers with base address and reg shift */
#define NS16550_REG(r)			((uint16_t *)(ns16550_uart_base + \
					(r << NS16550_REG_SHIFT)))
#define NS16550_REG_READ(r)		ioreg_read16(NS16550_REG(r))
#define NS16550_REG_WRITE(r, v)	ioreg_write16(NS16550_REG(r), v)

static void init_ns16550(uint64_t base)
{
	ns16550_uart_base = base;
	ns16550_uart_initialized = 1;

	/* Disable all interrupts */
	NS16550_REG_WRITE(NS16550_IER_OFFSET,
		NS16550_REG_READ(NS16550_FCR_OFFSET) & ~(NS16550_IIR_NO_INT));

	/* Disable FIFOs */
	NS16550_REG_WRITE(NS16550_FCR_OFFSET,
		NS16550_REG_READ(NS16550_FCR_OFFSET) & ~(NS16550_FCR_FIFO_EN));
}

void ns16550_console_init(const void *dtb)
{
	int offset, len, naddr, nsize;
	const uint64_t *regs;
	uint64_t reg_uart_base;

	uk_pr_info("Serial initializing\n");

	if ((offset = fdt_node_offset_by_compatible(dtb, -1, "ns16550")) < 0 &&
	    (offset = fdt_node_offset_by_compatible(dtb, -1, "ns16550a")) < 0) {
		UK_CRASH("No console UART found!\n");
	}

	naddr = fdt_address_cells(dtb, offset);
	if (naddr < 0 || naddr >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper address cells!\n");

	nsize = fdt_size_cells(dtb, offset);
	if (nsize < 0 || nsize >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper size cells!\n");

	regs = fdt_getprop(dtb, offset, "reg", &len);
	if (regs == NULL || (len < (int)sizeof(fdt32_t) * (naddr + nsize)))
		UK_CRASH("Bad 'reg' property: %p %d\n", regs, len);

	reg_uart_base = fdt64_to_cpu(regs[0]);
	uk_pr_info("Found NS16550 UART on: 0x%lx\n", reg_uart_base);

	init_ns16550(reg_uart_base);

	uk_pr_info("NS16550 UART initialized\n");
}

int ukplat_coutd(const char *str, unsigned int len)
{
	return ukplat_coutk(str, len);
}

static void _putc(char a)
{
	/* Wait until TX FIFO becomes empty */
	while (!(NS16550_REG_READ(NS16550_LSR_OFFSET) & NS16550_LSR_TX_EMPTY))
		;

	/* Reset DLAB and write to THR */
	NS16550_REG_WRITE(NS16550_LCR_OFFSET,
			  NS16550_REG_READ(NS16550_LCR_OFFSET) &
			  ~(NS16550_LCR_DLAB));
	NS16550_REG_WRITE(NS16550_THR_OFFSET, a & 0xff);
}

static void ns16550_putc(char a)
{
	/*
	 * Avoid using the UART before base address initialized, or
	 * if CONFIG_EARLY_UART_NS16550 is not enabled.
	 */
	if (!ns16550_uart_initialized)
		return;

	if (a == '\n')
		_putc('\r');
	_putc(a);
}

/* Try to get data from ns16550 UART without blocking */
static int ns16550_getc(void)
{
	/*
	 * Avoid using the UART before base address initialized, or
	 * if CONFIG_EARLY_UART_NS16550 is not enabled.
	 */
	if (!ns16550_uart_initialized)
		return -1;

	/* If RX FIFO is empty, return -1 immediately */
	if (!(NS16550_REG_READ(NS16550_LSR_OFFSET) & NS16550_LSR_RX_EMPTY))
		return -1;

	/* Reset DLAB and read from RBR */
	NS16550_REG_WRITE(NS16550_LCR_OFFSET,
			  NS16550_REG_READ(NS16550_LCR_OFFSET) &
			  ~(NS16550_LCR_DLAB));
	return (int)(NS16550_REG_READ(NS16550_RBR_OFFSET) & 0xff);
}

int ukplat_coutk(const char *buf, unsigned int len)
{
	for (unsigned int i = 0; i < len; i++)
		ns16550_putc(buf[i]);
	return len;
}

int ukplat_cink(char *buf, unsigned int maxlen)
{
	int ret;
	unsigned int num = 0;

	while (num < maxlen && (ret = ns16550_getc()) >= 0) {
		*(buf++) = (char) ret;
		num++;
	}

	return (int) num;
}
