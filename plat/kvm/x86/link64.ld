/* SPDX-License-Identifier: ISC */
/*
 * Author(s): Dan Williams <djwillia@us.ibm.com>
 *            Martin Lucina <martin.lucina@docker.com>
 *            Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2016, IBM
 *           (c) 2016-2017 Docker, Inc.
 *           (c) 2017-2018, NEC Europe Ltd.
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

ENTRY(_libkvmplat_entry)
SECTIONS
{
	. = 0x100000;

	/* Code */
	_text = .;
	.text :
	{
		/* prevent linker gc from removing multiboot header */
		KEEP (*(.data.boot))
		*(.text.boot)

		*(.text)
		*(.text.*)
	}
	_etext = .;

	/* Read-only data */
	. = ALIGN(0x1000);
	_rodata = .;
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
	}
	_erodata = .;

	/* Constructor tables (read-only) */
	_ctors = .;
	.preinit_array : {
		. = ALIGN(0x8);
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	}

	.init_array : {
		. = ALIGN(0x8);
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
	}
	_ectors = .;

	/* Read-write data (initialized) */
	. = ALIGN(0x1000);
	_data = .;
	.data :
	{
		*(.data)
		*(.data.*)
	}
	_edata = .;

	/* Read-write data (uninitialized) */
	. = ALIGN(0x1000);
	__bss_start = .;
	.bss :
	{
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(0x1000);
	}

	_end = .;
}
