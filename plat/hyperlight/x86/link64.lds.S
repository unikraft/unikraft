/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/limits.h>
#include <uk/plat/common/common.lds.h>

/*
 * Hyperlight's ELF loader assumes the last PT_LOAD segment has the
 * highest virtual address.  The default UK_SEGMENT_TLS_LOAD creates
 * a separate tls_load PT_LOAD segment at the end that violates this
 * assumption.  Instead, load .tdata via the data PT_LOAD segment —
 * it still gets a PT_TLS header for the TLS template metadata, but
 * the actual content is loaded as part of the data segment.
 */
#undef UK_SEGMENT_TLS_LOAD
#define UK_SEGMENT_TLS_LOAD :data

/*
 * Hyperlight loads guest code at BASE_ADDRESS.  This kernel is a
 * non-PIE ET_EXEC with no relocations, so it must be *linked* at the
 * exact address Hyperlight loads it at — otherwise every absolute
 * reference resolves to the wrong place and the guest faults on the
 * first non-relative jump.
 *
 * BASE_ADDR must therefore track `SandboxMemoryLayout::BASE_ADDRESS`
 * in hyperlight-host (src/mem/layout.rs).
 */
#define BASE_ADDR 0x4000

/*
 * Program headers.  Hyperlight reads these to determine memory
 * protection for each region:
 *   text   — read + execute  (code)
 *   rodata — read only       (constants, exception tables, init tables)
 *   data   — read + write    (.data, .bss — these become CoW pages)
 *   tls    — thread-local storage template
 *   stack  — marks the stack as non-executable
 */
PHDRS
{
	text    PT_LOAD    FLAGS(PHDRS_PF_RX);
	rodata  PT_LOAD    FLAGS(PHDRS_PF_R);
	data    PT_LOAD;
	tls     PT_TLS;
	stack   PT_GNU_STACK FLAGS(PHDRS_PF_RW);
}

SECTIONS
{
	. = BASE_ADDR;
	_base_addr = .;

	/*
	 * Code — .text.boot first so the entry point is at the
	 * lowest address in the text segment.
	 */
	_text = .;
	.text :
	{
		*(.text.boot)
		*(.text.boot.*)
		*(.text)
		*(.text.*)
	} :text
	_etext = .;

	/*
	 * Read-only data.  Page-aligned so Hyperlight can map it
	 * with different permissions than .text.
	 */
	. = ALIGN(__PAGE_SIZE);
	_rodata = .;
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
	} :rodata
	_erodata = .;

	/* Exception handling tables */
	EXCEPTION_SECTIONS

	/* Unikraft constructor/init/event tables */
	CTORTAB_SECTION
	INITTAB_SECTION
	EVENTTAB_SECTION

	/* C++ / static constructors */
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
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)
			SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
	}
	_ectors = .;

	/* Thread-local storage */
	TLS_SECTIONS

	/*
	 * Writable data (.data + .bss).  These pages are marked CoW
	 * by Hyperlight — the first write to any page triggers a
	 * page fault, the CoW handler copies the page to scratch
	 * memory, and remaps it writable.
	 */
	DATA_SECTIONS

	_end = .;

	.comment       0 : { *(.comment) }
	DEBUG_SYMBOLS

	DISCARDS
}
