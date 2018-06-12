/* SPDX-License-Identifier: ISC */
/*
 * Authors: Martin Lucina
 *          Felipe Huici <felipe.huici@neclab.eu>
 *
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017 NEC Europe Ltd., NEC Corporation
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

#include <errno.h>
#include <x86/cpu.h>
#include <uk/print.h>
#include <uk/plat/bootstrap.h>

static void cpu_halt(void) __noreturn;

/* TODO: implement CPU reset */
void ukplat_terminate(enum ukplat_gstate request __unused)
{
	/*
	 * Poke the QEMU "isa-debug-exit" device to "shutdown". Should be
	 * harmless if it is not present. This is used to enable automated
	 * tests on virtio.  Note that the actual QEMU exit() status will
	 * be 83 ('S', 41 << 1 | 1).
	 */
	uk_printk("Unikraft halted\n");
	outw(0x501, 41);

	/*
	 * If we got here, there is no way to initiate "shutdown" on virtio
	 * without ACPI, so just halt.
	 */
	cpu_halt();
}

static void cpu_halt(void)
{
	__asm__ __volatile__("cli; hlt");
	for (;;)
		;
}

int ukplat_suspend(void)
{
	return -EBUSY;
}
