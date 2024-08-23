/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdio.h>
#include <string.h>
#include <kvm/efi.h>
#include <sys/types.h>
#include <uk/console.h>
#include <uk/tty/gop.h>
#include "format.h"

/* Framebuffer metadata */
static __u32 *fb;
static uk_efi_uintn_t fb_size;
static __u32 fb_real_width;
static __u32 fb_height;
static __u32 fb_width;

/* Terminal metadata */
static __u32 row_count;
static __u32 col_count;
static __u32 cursor_row;
static __u32 cursor_col;

/* Font metadata */
extern __u8 char_width;
extern __u8 char_height;
extern __u8 font[256][16]; /* Default font */

/* Formatting */
extern __u8 format_fg;
extern __u8 format_bg;

#define TAB_ALIGNMENT 8
#define GOP_COLOR(r, g, b) ((r << 16) | (g << 8) | (b << 0))
#define GOP_BLACK	GOP_COLOR(  0,   0,   0)
#define GOP_RED		GOP_COLOR(192,   0,   0)
#define GOP_GREEN	GOP_COLOR(  0, 192,   0)
#define GOP_YELLOW	GOP_COLOR(192, 192,   0)
#define GOP_BLUE	GOP_COLOR(  0,   0, 192)
#define GOP_MAGENTA	GOP_COLOR(192,   0, 192)
#define GOP_CYAN	GOP_COLOR(  0, 192, 192)
#define GOP_WHITE	GOP_COLOR(192, 192, 192)

static __u32 gop_colors[] = {
	GOP_BLACK, GOP_RED, GOP_GREEN, GOP_YELLOW,
	GOP_BLUE, GOP_MAGENTA, GOP_CYAN, GOP_WHITE
};

#define GOP_FG_COLOR(format_fg) (gop_colors[format_fg - 30])
#define GOP_BG_COLOR(format_bg) (gop_colors[format_bg - 40])

static void gop_console_clear(void)
{
	for (size_t y = 0; y < fb_height; y++) {
		for (size_t x = 0; x < fb_width; x++) {
			const size_t index = x + y * fb_real_width;
			fb[index] = GOP_BLACK;
		}
	}
}

/* Draw formatted character at given row and col */
static void gop_putc_at(char c, __u32 col, __u32 row)
{
	__u32 fg_color = GOP_FG_COLOR(format_fg);
	__u32 bg_color = GOP_BG_COLOR(format_bg);
	for (size_t y = 0; y < char_height; y++) {
		const __u8 slice = font[(__u8)c][y];
		for (size_t x = 0; x < char_width; x++) {
			const size_t index = ((x + col * char_width)
				+ (y + row * char_height) * fb_real_width);
			if ((slice >> (char_width - x - 1)) & 1) {
				fb[index] = fg_color;
			} else {
				fb[index] = bg_color;
			}
		}
	}
}

/* Enter a new line, scroll if necessary */
static void gop_newline() {
	if (cursor_row == row_count - 1) {
		/* scroll content up one line */
		memmove(fb, &fb[char_height * fb_real_width],
			fb_size - char_height * fb_real_width);
		/* clean last row */
		for (__u32 col = 0; col < col_count - 2; col++)
			gop_putc_at(' ', col, row_count - 1);
	} else {
		cursor_row++;
	}
}

static void gop_putc(char c)
{
	switch (c) {
	case '\a':
		break;
	case '\b':
		if (cursor_col > 0) {
			cursor_col--;
		} else if (cursor_row > 0) {
			cursor_col = col_count - 1;
			cursor_row--;
		}
		break;
	case '\n':
		gop_newline();
		__fallthrough;
	case '\r':
		cursor_col = 0;
		break;
	case '\t':
		cursor_col += TAB_ALIGNMENT - (cursor_col % TAB_ALIGNMENT);
		if (cursor_col >= col_count) {
			cursor_col = 0;
			gop_newline();
		}
		break;
	default:
		if (!format_char(c))
			return;
		gop_putc_at(c, cursor_col, cursor_row);
		if (++cursor_col == col_count) {
			cursor_col = 0;
			gop_newline();
		}
		break;
	}
}

/* Print string of size len from buf */
static __ssz gop_console_out(struct uk_console *dev __unused,
			     const char *buf, __sz len)
{
	for (__sz i = 0; i < len; i++)
		gop_putc(buf[i]);
	return len;
}

/* Placeholder function. Input not implemented for GOP console */
static __ssz gop_console_in(struct uk_console *dev __unused,
			    char *buf __unused, __sz maxlen __unused)
{
	return 0;
}

static struct uk_console_ops gop_ops = {
	.out = gop_console_out,
	.in = gop_console_in
};

static struct uk_console gop_dev = UK_CONSOLE("GOP", &gop_ops,
					      UK_CONSOLE_FLAG_STDOUT);

uk_efi_status_t gop_init(struct uk_efi_boot_services *uk_efi_bs)
{
	struct uk_efi_graphics_output_proto *gop;
	struct uk_efi_graphics_output_mode_information *gop_info;
	uk_efi_uintn_t gop_info_sz;
	uk_efi_status_t status;

	status = uk_efi_bs->locate_protocol(UK_EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
					    NULL, (void **)&gop);
	if (status != UK_EFI_SUCCESS)
		return status;

	gop_info = NULL;
	status = gop->query_mode(gop, gop->mode->mode, &gop_info_sz, &gop_info);
	if (status != UK_EFI_SUCCESS)
		return status;

	/* Initialize framebuffer and metadata */
	fb = (__u32 *)gop->mode->frame_buffer_base;
	fb_size = gop->mode->frame_buffer_size;
	fb_width = gop->mode->info->horizontal_resolution;
	fb_height = gop->mode->info->vertical_resolution;
	fb_real_width = gop->mode->info->pixels_per_scanline;

	/* Initialize terminal metadata */
	row_count = fb_height / char_height;
	col_count = fb_width / char_width;
	cursor_row = 0;
	cursor_col = 0;

	gop_console_clear();
	uk_console_register(&gop_dev);

	return UK_EFI_SUCCESS;
}
