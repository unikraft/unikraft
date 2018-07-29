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
#include <kvm-x86/vga_console.h>

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

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;

static void clear_terminal(void)
{
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;

			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void _libkvmplat_init_vga_console(void)
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t *) 0xb8000;
	clear_terminal();
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

static void vga_newline(void)
{
	if (terminal_row == VGA_HEIGHT - 1)
		vga_scroll();
	else
		terminal_row++;
}

void _libkvmplat_vga_putc(char c)
{
	if (terminal_column == 0 && terminal_row == 0)
		clear_terminal();

	switch (c) {
	case '\a':
		break; //ascii bel (0x07) - ignore
	case '\b':
		if (terminal_column > 0) {
			terminal_column--;
		} else if (terminal_row > 0) {
			terminal_column = VGA_WIDTH - 1;
			terminal_row--;
		}
		break;
	case '\n':
		_libkvmplat_vga_putc('\r');
		vga_newline();
		break;
	case '\r':
		terminal_column = 0;
		break;
	case '\t':
		do {
			terminal_column++;
		} while (terminal_column % TAB_ALIGNMENT != 0
				&& terminal_column != VGA_WIDTH);

		if (terminal_column == VGA_WIDTH) {
			terminal_column = 0;
			vga_newline();
		}
		break;
	default:
		terminal_putentryat(c, terminal_color,
				terminal_column, terminal_row);
		if (++terminal_column == VGA_WIDTH) {
			terminal_column = 0;
			vga_newline();
		}
		break;
	}
}
