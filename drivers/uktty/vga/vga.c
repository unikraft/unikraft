/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Dafna Hirschfeld <dafna3@gmail.com>
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

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <x86/cpu.h>
#include <x86/irq.h>
#include <uk/console.h>
#include <uk/prio.h>
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>

#include <uk/essentials.h>

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

#define TAB_ALIGNMENT 8
#define VGA_WIDTH     80
#define VGA_HEIGHT    25
#define VGA_FB_BASE   0xb8000
#define X86_VIDEO_MEM_START	0xA0000UL
#define X86_VIDEO_MEM_LEN	0x20000UL


static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t *const terminal_buffer = (uint16_t *)VGA_FB_BASE;
static uint16_t areg;   /* VGA address register */
static uint16_t dreg;   /* VGA data register */

static void clear_terminal(void)
{
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;

			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;

	terminal_buffer[index] = vga_entry(c, color);
}
static void vga_scroll(void)
{
	size_t i;

	for (i = 1; i < VGA_HEIGHT; i++) {
		memcpy(terminal_buffer + ((i - 1) * VGA_WIDTH),
			terminal_buffer + (i * VGA_WIDTH), VGA_WIDTH * 2);
	}
	for (i = 0; i < VGA_WIDTH; i++)
		terminal_buffer[((VGA_HEIGHT - 1) * VGA_WIDTH) + i]
			= vga_entry(' ', terminal_color);
}

static void vga_update_cursor(void)
{
	unsigned long irq_flags;
	uint8_t old;

	local_irq_save(irq_flags);
	old = inb(areg);
	outb(areg, 0x0e); // Cursor Location High
	outb(dreg, ((terminal_row * VGA_WIDTH) + terminal_column) >> 8);
	outb(areg, 0x0f); // Cursor Location Low
	outb(dreg, ((terminal_row * VGA_WIDTH) + terminal_column) & 0xff);
	outb(areg, old);
	local_irq_restore(irq_flags);
}

static void vga_putc(char c)
{
#define NEWLINE()                           \
	do {                                \
		if (row == VGA_HEIGHT - 1)  \
			vga_scroll();       \
		else                        \
			row++;              \
	} while (0)

	unsigned long irq_flags;
	size_t row;
	size_t column;

	/* Make a consistent copy of the global state variables (row, column).
	 * This way, we can work on them consistently in this function and
	 * and prevent race conditions on them that could lead to writing
	 * outside the video memory. This doesn't make the function behave
	 * perfectly on reentrance (lines can still be overwritten by
	 * code paths running through this function concurrently), but at
	 * least we stay inside the video memory.
	 */
	local_irq_save(irq_flags);
	row = terminal_row;
	column = terminal_column;
	local_irq_restore(irq_flags);

	switch (c) {
	case '\a':
		break; //ascii bel (0x07) - ignore
	case '\b':
		if (column > 0) {
			column--;
		} else if (row > 0) {
			column = VGA_WIDTH - 1;
			row--;
		}
		break;
	case '\n':
		NEWLINE();
		__fallthrough;
	case '\r':
		column = 0;
		break;
	case '\t':
		do {
			column++;
		} while (column % TAB_ALIGNMENT != 0
				&& column != VGA_WIDTH);

		if (column == VGA_WIDTH) {
			column = 0;
			NEWLINE();
		}
		break;
	default:
		terminal_putentryat(c, terminal_color,
				column, row);
		if (++column == VGA_WIDTH) {
			column = 0;
			NEWLINE();
		}
		break;
	}

	local_irq_save(irq_flags);
	terminal_row = row;
	terminal_column = column;
	local_irq_restore(irq_flags);

	vga_update_cursor();
}

static __ssz vga_console_out(struct uk_console *dev __unused,
			     const char *buf, __sz len)
{
	for (__sz i = 0; i < len; i++)
		vga_putc(buf[i]);
	return len;
}

static __ssz vga_console_in(struct uk_console *dev __unused,
			    char *buf __unused, __sz maxlen __unused)
{
	return 0;
}

static struct uk_console_ops vga_ops = {
	.out = vga_console_out,
	.in = vga_console_in
};

static struct uk_console vga_dev = UK_CONSOLE("VGA", &vga_ops,
					      UK_CONSOLE_FLAG_STDOUT);

static int vga_init(struct ukplat_bootinfo *bi)
{
	unsigned long irq_flags;
	struct ukplat_memregion_desc mrd = {0};
	int rc;

	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	local_irq_save(irq_flags);
	/* Location of the address and data registers is variable and denoted
	 * by the least significant bit in the Input/Output register located
	 * at 0x3cc. For our emulated color display, they should always be
	 * 0x3d{4,5}, but better safe than sorry, so let's check at init time.
	 */
	if (inb(0x3cc) & 0x1) {
		areg = 0x3d4;
		dreg = 0x3d5;
	} else {
		areg = 0x3b4;
		dreg = 0x3b5;
	}

	/* Initialize cursor appearance. Setting CURSOR_START (0x0a) to 0x0e
	 * and CURSOR_END (0x0b) to 0x0f enables the cursor and produces
	 * a blinking underscore.
	 */
	outb(areg, 0x0a);
	outb(dreg, 0x0e);
	outb(areg, 0x0b);
	outb(dreg, 0x0f);
	local_irq_restore(irq_flags);

	clear_terminal();

	uk_console_register(&vga_dev);

	mrd.pbase = X86_VIDEO_MEM_START;
	mrd.vbase = X86_VIDEO_MEM_START;
	mrd.pg_off = 0;
	mrd.len = X86_VIDEO_MEM_LEN;
	mrd.pg_count = PAGE_COUNT(X86_VIDEO_MEM_LEN);
	mrd.type = UKPLAT_MEMRT_RESERVED;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0)) {
		uk_pr_err("Could not insert mrd (%d)\n", rc);
		return rc;
	}

	return 0;
}

UK_BOOT_EARLYTAB_ENTRY(vga_init, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
