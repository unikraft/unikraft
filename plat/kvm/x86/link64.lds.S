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

#include <uk/arch/limits.h> /* for __PAGE_SIZE */
#include <uk/plat/common/common.lds.h>

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

	EXCEPTION_SECTIONS

	CTORTAB_SECTION

	INITTAB_SECTION

	/* Read-only data */
	. = ALIGN(__PAGE_SIZE);
	_rodata = .;
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
	}
	_erodata = .;

	/* Constructor tables (read-only) */
	. = ALIGN(0x8);
	_ctors = .;
	.preinit_array : {
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	}

	. = ALIGN(0x8);
	.init_array : {
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
	}
	_ectors = .;

	TLS_SECTIONS

	/* Read-write data (initialized) */
	. = ALIGN(__PAGE_SIZE);
	_data = .;
	.data :
	{
		*(.data)
		*(.data.*)
	}
	_edata = .;

	/* Read-write data (uninitialized) */
	. = ALIGN(__PAGE_SIZE);
	__bss_start = .;
	.bss :
	{
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(__PAGE_SIZE);
	}

	/* We keep the interrupt stack on a different section
	 * given that it may have a big alignment and it would
	 * change the entire binary layout
	 */
	.intrstack :
	{
		*(.intrstack)
		. = ALIGN(__PAGE_SIZE);
	}

	_end = .;

	.comment       0 : { *(.comment) }
	DEBUG_SYMBOLS

	/DISCARD/ : { *(.note.gnu.build-id) }
}
