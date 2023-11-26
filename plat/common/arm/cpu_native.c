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
 */
#include <uk/config.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/irq.h>
#include <arm/psci.h>
#include <uk/assert.h>
#include <arm/smccc.h>
#include <uk/plat/common/lcpu.h>


/*
 * Halts the CPU until the next external interrupt is fired. For Arm,
 * we can use WFI to implement this feature.
 */
void halt(void)
{
	__asm__ __volatile__("wfi");
}

/* Systems support PSCI >= 0.2 can do system reset from PSCI */
void reset(void)
{
	struct smccc_args smccc_arguments = {0};

	/*
	 * NO PSCI or invalid PSCI method, we can't do reset, just
	 * halt the CPU.
	 */
	if (!smccc_psci_call) {
		uk_pr_crit("Couldn't reset system, HALT!\n");
		__CPU_HALT();
	}

	smccc_arguments.a0 = PSCI_FNID_SYSTEM_RESET;
	smccc_psci_call(&smccc_arguments);
}

/* Systems support PSCI >= 0.2 can do system off from PSCI */
void system_off(enum ukplat_gstate request __unused)
{
	struct smccc_args smccc_arguments = {0};

	/*
	 * NO PSCI or invalid PSCI method, we can't do shutdown, just
	 * halt the CPU.
	 */
	if (!smccc_psci_call) {
		uk_pr_crit("Couldn't shutdown system, HALT!\n");
		__CPU_HALT();
	}

	smccc_arguments.a0 = PSCI_FNID_SYSTEM_OFF;
	smccc_psci_call(&smccc_arguments);
}

#ifdef CONFIG_HAVE_SMP
/* Powers on a secondary cpu in an SMP system. */
int cpu_on(__lcpuid id, __paddr_t entry, void *arg)
{
	struct smccc_args smccc_arguments = {0};

	/*
	 * Check if a PSCI method is set.
	 */
	UK_ASSERT(smccc_psci_call);

	smccc_arguments.a0 = PSCI_FNID_CPU_ON;
	smccc_arguments.a1 = (__u64) id;
	smccc_arguments.a2 = (__u64) entry;
	smccc_arguments.a3 = (__u64) arg;

	smccc_psci_call(&smccc_arguments);

	return (int) smccc_arguments.a0;
}
#endif /* CONFIG_HAVE_SMP */
