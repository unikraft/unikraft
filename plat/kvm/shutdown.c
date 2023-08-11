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
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/irq.h>
#include <uk/print.h>
#include <uk/plat/bootstrap.h>
#include <kvm/efi.h>
#include <uk/plat/common/bootinfo.h>

static void cpu_halt(void) __noreturn;

#ifdef CONFIG_KVM_BOOT_PROTO_EFI_STUB
static void uk_efi_rs_reset_system(enum uk_efi_reset_type reset_type)
{
	const char reset_data[] = "UK EFI SYSTEM RESET";
	struct uk_efi_runtime_services *rs;
	struct ukplat_bootinfo *bi;

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi || !bi->efi_st))
		return;

	rs = ((struct uk_efi_sys_tbl *)bi->efi_st)->runtime_services;
	if (unlikely(!rs))
		return;

	rs->reset_system(reset_type, UK_EFI_SUCCESS,
			 sizeof(reset_data), (void *)reset_data);
}
#else
static void uk_efi_rs_reset_system(enum uk_efi_reset_type reset_type __unused)
{ }
#endif

void ukplat_terminate(enum ukplat_gstate request)
{
	uk_pr_info("Unikraft halted\n");

	switch (request) {
	case UKPLAT_RESTART:
		uk_efi_rs_reset_system(UK_EFI_RESET_COLD);

		break;
	default:
		uk_efi_rs_reset_system(UK_EFI_RESET_SHUTDOWN);
	}

	/* Try to make system off */
	system_off(request);

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
