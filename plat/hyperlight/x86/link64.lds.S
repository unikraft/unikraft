/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/limits.h> /* for __PAGE_SIZE */
#include <uk/plat/common/common.lds.h>

/*
 * Override UK_SEGMENT_TLS_LOAD to be empty.
 * The tls_load segment creates an empty LOAD segment at the end which
 * confuses Hyperlight's ELF loader (it assumes the last PT_LOAD has
 * the highest address).
 */
#undef UK_SEGMENT_TLS_LOAD
#define UK_SEGMENT_TLS_LOAD

/*
 * Hyperlight loads the guest at address BASE_ADDRESS (0x1000 on Linux,
 * layout.rs) but links the binary from 0x0 so e_entry is an offset.
 * The page tables are set up by Hyperlight before the guest starts.
 * The entry point receives:
 *   - RDI: PEB address
 *   - RSI: Seed
 *   - RDX: Page size
 *   - RCX: Max log level
 *   - RSP: Stack pointer (already set)
 */
#define BASE_ADDR 0x0

PHDRS
{
	text PT_LOAD FLAGS(PHDRS_PF_RX);
	rodata PT_LOAD FLAGS(PHDRS_PF_R);
	data PT_LOAD;
	tls PT_TLS;
	/* Note: tls_load removed - it creates an empty LOAD segment at the end
	 * which confuses Hyperlight's ELF loader that assumes the last PT_LOAD
	 * has the highest address.
	 */
	stack PT_GNU_STACK FLAGS(PHDRS_PF_RW);
}

SECTIONS
{
	. = BASE_ADDR;
	_base_addr = .; /* section-relative: leaq _base_addr(%rip) returns load_bias at runtime */

	/* Code */
	_text = .;
	.text :
	{
		/* Entry point must be at the start */
		*(.text.boot)
		*(.text.boot.*)

		*(.text)
		*(.text.*)
	} :text
	_etext = .;

	/* Read-only data */
	. = ALIGN(__PAGE_SIZE);
	_rodata = .;
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
	} :rodata
	_erodata = .;

	EXCEPTION_SECTIONS

	CTORTAB_SECTION

	INITTAB_SECTION

	EVENTTAB_SECTION

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

	DATA_SECTIONS

	_end = .;

	.comment       0 : { *(.comment) }
	DEBUG_SYMBOLS

	DISCARDS
}
