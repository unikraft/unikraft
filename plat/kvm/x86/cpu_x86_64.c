/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
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

#include <string.h>
#include <x86/desc.h>
#include <kvm/setup.h>
#include <kvm-x86/cpu_x86_64_defs.h>
#include <kvm-x86/cpu_x86_64.h>

static struct seg_desc32 cpu_gdt64[GDT_NUM_ENTRIES] ALIGN_64_BIT;

/*
 * The monitor (ukvm) or bootloader + bootstrap (virtio) starts us up with a
 * bootstrap GDT which is "invisible" to the guest, init and switch to our own
 * GDT.
 *
 * This is done primarily since we need to do LTR later in a predictable
 * fashion.
 */
static void gdt_init(void)
{
	volatile struct desc_table_ptr64 gdtptr;

	memset(cpu_gdt64, 0, sizeof(cpu_gdt64));
	cpu_gdt64[GDT_DESC_CODE].raw = GDT_DESC_CODE_VAL;
	cpu_gdt64[GDT_DESC_DATA].raw = GDT_DESC_DATA_VAL;

	gdtptr.limit = sizeof(cpu_gdt64) - 1;
	gdtptr.base = (__u64) &cpu_gdt64;
	__asm__ __volatile__("lgdt (%0)" ::"r"(&gdtptr));
	/*
	 * TODO: Technically we should reload all segment registers here, in
	 * practice this doesn't matter since the bootstrap GDT matches ours,
	 * for now.
	 */
}

void cpu_init(void)
{
	gdt_init();
}

void cpu_halt(void)
{
	__asm__ __volatile__("cli; hlt");
	for (;;)
		;
}
