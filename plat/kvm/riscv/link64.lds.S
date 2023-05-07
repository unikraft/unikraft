/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *          Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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
#include <uk/plat/common/common.lds.h>

/* RAM base address for many RISCV64 platforms, including QEMU */
#define RAM_BASE_ADDR 0x80000000

/* Reserve at most 2MB of space for SBI firmware */
#define SBI_RESERVED_SIZE 0x200000

OUTPUT_ARCH(riscv)
ENTRY(_libkvmplat_entry)

PHDRS
{
	text PT_LOAD FLAGS(PHDRS_PF_RX);
	rodata PT_LOAD FLAGS(PHDRS_PF_R);
	data PT_LOAD;
	tls PT_TLS;
	tls_load PT_LOAD;
	stack PT_GNU_STACK FLAGS(PHDRS_PF_RW);
}

SECTIONS {
	. = RAM_BASE_ADDR;
	_start_ram_addr = .;
	/*
	 * TODO: An SBI firmware is loaded at RAM_BASE_ADDR only when executing
	 * in a baremetal/emulated environment. When Unikraft is running as a
	 * KVM guest, the SBI implementation is found at the hypervisor level,
	 * so in that case we should not reserve any space for a firmware.
	 */
	. = RAM_BASE_ADDR + SBI_RESERVED_SIZE;

	/* Code */
	_text = .;
	.text :
	{
		*(.text)
		*(.text.*)
	} :text

	. = ALIGN(__PAGE_SIZE);
	_etext = .;

	/* Read-only data */
	. = ALIGN(__PAGE_SIZE);
	_rodata = .;
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
		*(.srodata)
		*(.srodata.*)
	} :rodata

	EXCEPTION_SECTIONS

	CTORTAB_SECTION

	INITTAB_SECTION

	EVENTTAB_SECTION

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
	. = ALIGN(__PAGE_SIZE);
	_ectors = .;


	TLS_SECTIONS

	DATA_SECTIONS

	_end = .;

	.comment       0 : { *(.comment) }
	DEBUG_SYMBOLS

	/DISCARD/ : { *(.note.gnu.build-id) }
}
