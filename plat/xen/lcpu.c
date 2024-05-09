/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <stdint.h>
#if defined(__X86_32__) || defined(__x86_64__)
#include <xen-x86/irq.h>
#include <x86/cpu.h>
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/os.h>
#include <arm/cpu.h>
#include <uk/plat/common/irq.h>
#else
#error "Unsupported architecture"
#endif
#include <uk/plat/lcpu.h>
#include <uk/plat/time.h>

void ukplat_lcpu_enable_irq(void)
{
	local_irq_enable();
}

void ukplat_lcpu_disable_irq(void)
{
	local_irq_disable();
}

void ukplat_lcpu_halt_irq(void)
{
	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	ukplat_lcpu_enable_irq();
	halt();
	ukplat_lcpu_disable_irq();
}

unsigned long ukplat_lcpu_save_irqf(void)
{
	unsigned long flags;

	local_irq_save(flags);

	return flags;
}

void ukplat_lcpu_restore_irqf(unsigned long flags)
{
	local_irq_restore(flags);
}

int ukplat_lcpu_irqs_disabled(void)
{
	return irqs_disabled();
}
