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
#include <cpu.h>
#include <irq.h>
#include <uk/print.h>
#include <uk/plat/bootstrap.h>

static void cpu_halt(void) __noreturn;

/* TODO: implement CPU reset */
void ukplat_terminate(enum ukplat_gstate request __unused)
{
	uk_pr_info("Unikraft halted\n");

	/* Try to make system off */
	system_off();

	/*
	 * If we got here, there is no way to initiate "shutdown" on virtio
	 * without ACPI, so just halt.
	 */
	cpu_halt();
}

static void cpu_halt(void)
{
	__CPU_HALT();
}

int ukplat_suspend(void)
{
	return -EBUSY;
}
