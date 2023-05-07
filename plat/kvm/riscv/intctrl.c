/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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
#include <riscv/cpu_defs.h>
#include <riscv/cpu.h>
#include <riscv/plic.h>
#include <riscv/sbi.h>
#include <kvm/config.h>
#include <uk/assert.h>

void intctrl_clear_irq(unsigned int irq)
{
	/*
	 * The RISC-V PLIC spec specifies that global interrupt source 0 doesn't
	 * exist. We use IRQ 0 as an internal convention for timer interrupts.
	 * Those are manipulated through Control Status Registers, not the PLIC,
	 * hence timer interrupts are not treated as external interrupts.
	 */
	if (irq)
		plic_enable_irq(irq), plic_set_priority(irq, 1);
	else
		/*
		 * Sets the enable supervisor timer interrupt bit.
		 * A timer interrupt actually fires only when a timer event has
		 * been scheduled via SBI, which in turn uses machine mode
		 * specific CSRs (such as mtimecmp) to program a timer alarm.
		 */
		_csr_set(CSR_SIE, SIP_STIP);
}

void intctrl_mask_irq(unsigned int irq)
{
	if (irq)
		plic_disable_irq(irq);
	else
		_csr_clear(CSR_SIE, SIP_STIP);
}

void intctrl_ack_irq(unsigned int irq)
{
	if (irq) {
		plic_complete(irq);
	} else {
		/*
		 * From the RISC-V SBI spec: "If the supervisor wishes to clear
		 * the timer interrupt without scheduling the next timer event,
		 * it can request a timer interrupt infinitely far into the
		 * future (i.e., (uint64_t)-1)."
		 *
		 * Essentialy this is used to clear the timer interrupt pending
		 * bit to mark that the interrupt has been acknowledged.
		 */
		sbi_set_timer((__u64)-1);
	}
}

void intctrl_init(void)
{
	int rc;

	rc = init_plic(_libkvmplat_cfg.dtb);
	if (rc < 0)
		UK_CRASH("Interrupt controller not found, crashing...\n");

	_csr_set(CSR_SIE, SIP_SEIP); // Enable external interrupts
}
