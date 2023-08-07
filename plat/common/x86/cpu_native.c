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

#ifdef CONFIG_KVM_VMM_QEMU

/* The port used by QEMU by default for the isa-debug-exit device */
#define QEMU_ISA_DEBUG_EXIT_PORT	0x501
/* This corresponds to an 83 (41 << 1 | 1) return value from QEMU */
#define QEMU_ISA_DEBUG_EXIT_NO_CRASH	41
/* This corresponds to an 85 (42 << 1 | 1) return value from QEMU */
#define QEMU_ISA_DEBUG_EXIT_CRASH	42

/* The port used by QEMU by default for the pvpanic device */
#define QEMU_PVPANIC_EXIT_PORT		0x505
/* This corresponds to a GUEST_PANICKED event for QEMU */
#define QEMU_PVPANIC_GUEST_PANICKED	(1 << 0)
/* This corresponds to a GUEST_CRASHLOADED event for QEMU */
#define QEMU_PVPANIC_GUEST_CRASHLOADED	(1 << 1)

/**
 * Trigger an exit() in QEMU with the code `value << 1 | 1`.
 * @param value the value used in the calculation of the exit code
 */
static void qemu_debug_exit(int value)
{
	outw(QEMU_ISA_DEBUG_EXIT_PORT, value);
}
#endif /* CONFIG_KVM_VMM_QEMU */

void system_off(enum ukplat_gstate request __maybe_unused)
{
#ifdef CONFIG_KVM_VMM_FIRECRACKER
	/* Trigger the reset line via the PS/2 controller. On firecracker
	 * this will shutdown the VM.
	 */
	outb(0x64, 0xFE);
#endif /* CONFIG_KVM_VMM_FIRECRACKER */

#ifdef CONFIG_KVM_VMM_QEMU
	/* If we are crashing, then try to exit QEMU with the isa-debug-exit
	 * device.
	 * Should be harmless if it is not present. This is used to enable
	 * automated tests on virtio.
	 * Also send a panic request to the pvpanic device.
	 * Should be also harmless and helps with automated tests.
	 */
	if (request == UKPLAT_CRASH) {
		qemu_debug_exit(QEMU_ISA_DEBUG_EXIT_CRASH);
		outb(QEMU_PVPANIC_EXIT_PORT, QEMU_PVPANIC_GUEST_PANICKED);
	}
#endif /* CONFIG_KVM_VMM_QEMU */

	/*
	 * Perform an ACPI shutdown by writing (SLP_TYPa | SLP_EN) to PM1a_CNT.
	 * Generally speaking, we'd have to jump through a lot of hoops to
	 * collect those values, however, for QEMU, those are static. Should be
	 * harmless if we're not running on QEMU, especially considering we're
	 * already shutting down, so who cares if we crash.
	 */
	outw(0x604, 0x2000);

#ifdef CONFIG_KVM_VMM_QEMU
	/*
	 * If that didn't work for whatever reason, try poking the QEMU
	 * "isa-debug-exit" device to "shutdown".
	 */
	qemu_debug_exit(QEMU_ISA_DEBUG_EXIT_NO_CRASH);
#endif /* CONFIG_KVM_VMM_QEMU */
}
