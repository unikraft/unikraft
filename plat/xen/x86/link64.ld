/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Taken from Mini-OS */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(_libxenplat_start)
SECTIONS
{
	. = 0x0;
	_text = .;			/* Text and read-only data */
	.text : {
		*(.text.boot)
		*(.text)
		*(.gnu.warning)
	} = 0x2000

	_etext = .;			/* End of text section */

	_rodata = .;
	.rodata : {
		*(.rodata)
		*(.rodata.*)
	}
	. = ALIGN(4096);
	_erodata = .;

	_data = .;
	.data : {			/* Data */
		*(.data)
	}

	_edata = .;			/* End of data section */

	__bss_start = .;		/* BSS */
	.bss : {
		*(.bss)
		. = ALIGN(4096);
	}
	_end = . ;

	/* Sections to be discarded */
	/* TODO revisit when we have a resolution for discarded sections
	/DISCARD/ : {
		*(.text.exit)
		*(.data.exit)
		*(.exitcall.exit)
	}
	*/
}
