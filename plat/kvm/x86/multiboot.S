/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
 *                     All rights reserved.
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

#include <uk/config.h>
#include <uk/asm.h>

#include <kvm-x86/multiboot_defs.h>

#define MULTIBOOT_FLAGS \
	(MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE)

.section .data.boot

.align 4
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC+MULTIBOOT_FLAGS) /* checksum */
	.long multiboot_header				/* header addr */
	.long 0x100000					/* load addr */
	.long __bss_start				/* load end addr */
	.long _end					/* bss end addr */
	.long multiboot_start32				/* entry addr */

/*
 * Stack and entry function to use during CPU initialization
 */
.section .bss
.space 4096
lcpu_bootstack:

.section .rodata
lcpu_boot_startup_args:
	.quad	_libkvmplat_entry
	.quad	lcpu_bootstack

/*
 * 32-bit multiboot entry function
 *
 * EAX contains magic value, EBX contains 32-bit physical address of multiboot
 * information. Flat 4GiB CS and DS segments, with ES, FS, GS, and SS set to
 * DS. A20 gate enabled. Protected mode enabled, paging disabled. Interrupts
 * disabled.
 */
.code32
.section .text.32.boot
ENTRY(multiboot_start32)
	/* only multiboot is supported for now */
	cmpl	$MULTIBOOT_BOOTLOADER_MAGIC, %eax
	jne	no_multiboot

	movl	$lcpu_boot_startup_args, %edi	/* startup args for boot CPU */
	movl	%ebx, %esi			/* multiboot info */

	jmp	lcpu_start32

no_multiboot:
	cli
1:
	hlt
	jmp	1b
END(multiboot_start32)
