/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd., All rights reserved.
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
#include <uk/config.h>
#include <uk/plat/common/cpu.h>
#if !CONFIG_ARCH_ARM_32
/* TODO: Not yet supported for Arm32 */
#include <uk/plat/common/irq.h>
#include <arm/cpu_defs.h>
#endif
#include <uk/assert.h>

/*
 * Halts the CPU until the next external interrupt is fired. For Arm,
 * we can use WFI to implement this feature.
 */
void halt(void)
{
	__asm__ __volatile__("wfi");
}

#if !CONFIG_ARCH_ARM_32
/*
 * TODO: Port the following functionality to Arm32
 */
/* Systems support PSCI >= 0.2 can do system reset from PSCI */
void reset(void)
{
	/*
	 * NO PSCI or invalid PSCI method, we can't do reset, just
	 * halt the CPU.
	 */
	if (!smcc_psci_call) {
		uk_pr_crit("Couldn't reset system, HALT!\n");
		__CPU_HALT();
	}

	smcc_psci_call(PSCI_FNID_SYSTEM_RESET, 0, 0, 0);
}

/* Systems support PSCI >= 0.2 can do system off from PSCI */
void system_off(void)
{
	/*
	 * NO PSCI or invalid PSCI method, we can't do shutdown, just
	 * halt the CPU.
	 */
	if (!smcc_psci_call) {
		uk_pr_crit("Couldn't shutdown system, HALT!\n");
		__CPU_HALT();
	}

	smcc_psci_call(PSCI_FNID_SYSTEM_OFF, 0, 0, 0);
}
#endif /* !CONFIG_ARCH_ARM_32 */
