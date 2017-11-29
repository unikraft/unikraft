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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* Taken from Mini-OS */

OUTPUT_ARCH(arm)
ENTRY(_libxenplat_start)
SECTIONS
{
	/* Note: we currently assume that Xen will load the kernel image
	 * at start-of-RAM + 0x8000. We use this initial 32 KB for the stack
	 * and translation tables.
	*/
	_boot_stack	= 0x400000;	/* 16 KB boot stack */
	_boot_stack_end	= 0x404000;
	_page_dir	= 0x404000;	/* 16 KB translation table */
	.		= 0x408000;
	_text = .;			/* Text and read-only data */
	.text : {
		*(.text.zimageboot)	/* zImage boot header & code
					   must come first */

		*(.text)
		*(.gnu.warning)
	} = 0x9090

	_etext = .;			/* End of text section */

	_rodata = .;
	.rodata : { *(.rodata) *(.rodata.*) }
	. = ALIGN(4096);
	_erodata = .;

	/* newlib initialization functions */
	. = ALIGN(32 / 8);
	PROVIDE (__preinit_array_start = .);
	.preinit_array : { *(.preinit_array) }
	PROVIDE (__preinit_array_end = .);
	PROVIDE (__init_array_start = .);
	.init_array : { *(.init_array) }
	PROVIDE (__init_array_end = .);
	PROVIDE (__fini_array_start = .);
	.fini_array : { *(.fini_array) }
	PROVIDE (__fini_array_end = .);

	.ctors : {
		__CTOR_LIST__ = .;
		*(.ctors)
		CONSTRUCTORS
		LONG(0)
		__CTOR_END__ = .;
	}

	.dtors : {
		__DTOR_LIST__ = .;
		*(.dtors)
		LONG(0)
		__DTOR_END__ = .;
	}

	.data : {			/* Data */
		_data = .;
		*(.data)
	}

	/* Note: linker will insert any extra sections here, just before .bss */

	.bss : {
		_edata = .;		/* End of data included in image */
		/* Nothing after here is included in the zImage's size */

		__bss_start = .;
		*(.bss)
		*(.app.bss)
	}
	_end = . ;

	/* Sections to be discarded */
	/DISCARD/ : {
		*(.text.exit)
		*(.data.exit)
		*(.exitcall.exit)
	}

	/* Stabs debugging sections.  */
	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }
}
