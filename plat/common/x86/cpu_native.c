/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <x86/cpu.h>

void halt(void)
{
	__asm__ __volatile__ ("hlt" : : : "memory");
}

unsigned long read_cr2(void)
{
	unsigned long cr2;

	__asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));

	return cr2;
}

void system_off(void)
{
	/*
	 * Perform an ACPI shutdown by writing (SLP_TYPa | SLP_EN) to PM1a_CNT.
	 * Generally speaking, we'd have to jump through a lot of hoops to
	 * collect those values, however, for QEMU, those are static. Should be
	 * harmless if we're not running on QEMU, especially considering we're
	 * already shutting down, so who cares if we crash.
	 */
	outw(0x604, 0x2000);

	/*
	 * If that didn't work for whatever reason, try poking the QEMU
	 * "isa-debug-exit" device to "shutdown". Should be harmless if it is
	 * not present. This is used to enable automated tests on virtio.  Note
	 * that the actual QEMU exit() status will be 83 ('S', 41 << 1 | 1).
	 */
	outw(0x501, 41);
}
