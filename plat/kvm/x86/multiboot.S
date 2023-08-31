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
#include <uk/reloc.h>

#include <kvm-x86/multiboot.h>

/**
 * Stack and entry function to use during CPU initialization
 */
.section .bss
.space 4096
lcpu_bootstack:

.section .rodata
lcpu_boot_startup_args:
	ur_data	quad, multiboot_entry, 8
	ur_data	quad, lcpu_bootstack, 8

/**
 * 32-bit multiboot entry function
 *
 * EAX contains magic value, EBX contains 32-bit physical address of multiboot
 * information. Flat 4GiB CS and DS segments, with ES, FS, GS, and SS set to
 * DS. A20 gate enabled. Protected mode enabled, paging disabled. Interrupts
 * disabled.
 */
.code32
.section .text.32.boot
ENTRY(_multiboot_entry)
	/* only multiboot is supported for now */
	cmpl	$MULTIBOOT_BOOTLOADER_MAGIC, %eax
	jne	no_multiboot

	/* Hardcoding for now I guess... */
	movl    $0x00100000, %edi
	movl    $0x00000000, %esi
	movl    $0x00100000, %edx
	do_uk_reloc32   0

	/* startup args for boot CPU */
	ur_mov  lcpu_boot_startup_args, %edi, 4, _phys
	movl	%ebx, %esi			/* multiboot info */

	ur_mov	lcpu_start32, %ebx, 4, _phys
	jmp     *%ebx

no_multiboot:
	cli
1:
	hlt
	jmp	1b
END(_multiboot_entry)
