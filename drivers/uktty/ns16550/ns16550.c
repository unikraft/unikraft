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
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/init.h>
#include <uk/plat/common/bootinfo.h>

#if CONFIG_PAGING
#include <uk/bus/platform.h>
#include <uk/errptr.h>
#endif /* CONFIG_PAGING */

#define NS16550_THR_OFFSET	0x00U
#define NS16550_RBR_OFFSET	0x00U
#define NS16550_IER_OFFSET	0x01U
#define NS16550_IIR_OFFSET	0x02U
#define NS16550_FCR_OFFSET	0x02U
#define NS16550_LCR_OFFSET	0x03U
#define NS16550_MCR_OFFSET	0x04U
#define NS16550_LSR_OFFSET	0x05U
#define NS16550_MSR_OFFSET	0x06U

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
#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE_BASE
static __u8 ns16550_uart_initialized = 1;
static __u64 ns16550_uart_base = CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE_BASE;
#else /* !CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE_BASE */
static __u8 ns16550_uart_initialized;
static __u64 ns16550_uart_base;
#endif /* !CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE_BASE */

/* The register shift. Default is 0 (device-tree spec v0.4 Sect. 4.2.2) */
static __u32 ns16550_reg_shift = CONFIG_LIBUKTTY_NS16550_REG_SHIFT;

/* The register width. Default is 1 (8-bit register width) */
static __u32 ns16550_reg_width = CONFIG_LIBUKTTY_NS16550_REG_WIDTH;

/* Macros to access ns16550 registers with base address and reg shift */
#define NS16550_REG(r) (ns16550_uart_base + (r << ns16550_reg_shift))

static __u32 ns16550_reg_read(__u32 reg)
{
	__u32 ret;

	switch (ns16550_reg_width) {
	case 1:
		ret = ioreg_read8((__u8 *)NS16550_REG(reg)) & 0xff;
		break;
	case 2:
		ret = ioreg_read16((__u16 *)NS16550_REG(reg)) & 0xffff;
		break;
	case 4:
		ret = ioreg_read32((__u32 *)NS16550_REG(reg));
		break;
	default:
		UK_CRASH("Invalid register width: %d\n", ns16550_reg_width);
	}
	return ret;
}

static void ns16550_reg_write(__u32 reg, __u32 value)
{
	switch (ns16550_reg_width) {
	case 1:
		ioreg_write8((__u8 *)NS16550_REG(reg),
			     (__u8)(value & 0xff));
		break;
	case 2:
		ioreg_write16((__u16 *)NS16550_REG(reg),
			      (__u16)(value & 0xffff));
		break;
	case 4:
		ioreg_write32((__u32 *)NS16550_REG(reg), value);
		break;
	default:
		UK_CRASH("Invalid register width: %d\n", ns16550_reg_width);
	}
}

static int init_ns16550(void)
{
	/* Disable all interrupts */
	ns16550_reg_write(NS16550_IER_OFFSET,
			  ns16550_reg_read(NS16550_FCR_OFFSET) &
					 ~(NS16550_IIR_NO_INT));

	/* Disable FIFOs */
	ns16550_reg_write(NS16550_FCR_OFFSET,
			  ns16550_reg_read(NS16550_FCR_OFFSET) &
					 ~(NS16550_FCR_FIFO_EN));
	return 0;
}

static int ns16550_init(struct uk_init_ctx *ictx __unused)
{
	int offset, len, naddr, nsize;
	struct ukplat_bootinfo *bi;
	const __u64 *regs;
	const void *dtb;
	__u64 reg_base;
	__u64 reg_size;
	int rc;

	bi = ukplat_bootinfo_get();
	UK_ASSERT(bi);

	dtb = (void *)bi->dtb;
	UK_ASSERT(dtb);

	uk_pr_debug("Probing ns16550\n");

	if (unlikely((offset = fdt_node_offset_by_compatible(dtb, -1, "ns16550")) < 0 &&
		     (offset = fdt_node_offset_by_compatible(dtb, -1, "ns16550a")) < 0)) {
		uk_pr_err("ns16550 not found in fdt\n");
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

#if CONFIG_PAGING
	/* Map device region */
	ns16550_uart_base = uk_bus_pf_devmap(reg_base, reg_size);
	if (unlikely(PTRISERR(ns16550_uart_base))) {
		uk_pr_err("Could not map ns16550\n");
		return PTR2ERR(ns16550_uart_base);
	}
#else /* !CONFIG_PAGING */
	ns16550_uart_base = reg_base;
#endif /* !CONFIG_PAGING */

	regs = fdt_getprop(dtb, offset, "reg-shift", &len);
	if (regs)
		ns16550_reg_shift = fdt32_to_cpu(regs[0]);

	regs = fdt_getprop(dtb, offset, "reg-io-width", &len);
	if (regs)
		ns16550_reg_width = fdt32_to_cpu(regs[0]);

	uk_pr_debug("ns16550 @ 0x%lx - 0x%lx\n", reg_base, reg_base + reg_size);

	rc = init_ns16550();
	if (unlikely(rc)) {
		uk_pr_err("Could not initialize ns16550\n");
		return rc;
	}

	ns16550_uart_initialized = 1;
	uk_pr_info("tty: ns16550\n");

	return 0;
}

static void _putc(char a)
{
	/* Wait until TX FIFO becomes empty */
	while (!(ns16550_reg_read(NS16550_LSR_OFFSET) & NS16550_LSR_TX_EMPTY))
		;

	/* Reset DLAB and write to THR */
	ns16550_reg_write(NS16550_LCR_OFFSET,
			  ns16550_reg_read(NS16550_LCR_OFFSET) &
			  ~(NS16550_LCR_DLAB));
	ns16550_reg_write(NS16550_THR_OFFSET, a & 0xff);
}

static void ns16550_putc(char a)
{
	/*
	 * Avoid using the UART before base address initialized, or
	 * if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE_BASE is not enabled.
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
	 * if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE_BASE is not enabled.
	 */
	if (!ns16550_uart_initialized)
		return -1;

	/* If RX FIFO is empty, return -1 immediately */
	if (!(ns16550_reg_read(NS16550_LSR_OFFSET) & NS16550_LSR_RX_EMPTY))
		return -1;

	/* Reset DLAB and read from RBR */
	ns16550_reg_write(NS16550_LCR_OFFSET,
			  ns16550_reg_read(NS16550_LCR_OFFSET) &
			  ~(NS16550_LCR_DLAB));
	return (int)(ns16550_reg_read(NS16550_RBR_OFFSET) & 0xff);
}

int ukplat_coutk(const char *buf, unsigned int len)
{
	for (unsigned int i = 0; i < len; i++)
		ns16550_putc(buf[i]);
	return len;
}

int ukplat_coutd(const char *str, unsigned int len)
{
	return ukplat_coutk(str, len);
}

int ukplat_cink(char *buf, unsigned int maxlen)
{
	int ret;
	unsigned int num = 0;

	while (num < maxlen && (ret = ns16550_getc()) >= 0) {
		*(buf++) = (char)ret;
		num++;
	}

	return (int)num;
}

uk_plat_initcall_prio(ns16550_init, 0, UK_PRIO_EARLIEST);
