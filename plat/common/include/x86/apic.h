/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
 *                     All rights reserved.
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

#ifndef __PLAT_CMN_X86_APIC_H__
#define __PLAT_CMN_X86_APIC_H__

#include <uk/assert.h>
#include <uk/arch/limits.h>
#include <uk/plat/common/lcpu.h>
#include <x86/cpu.h>
#include <x86/apic_defs.h>

#include <errno.h>

static inline int apic_enable(struct lcpu *this_lcpu)
{
	__u32 eax, ebx, ecx, edx;
	__u32 x2apic_support;

	/* Check for x2APIC support */
	cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	x2apic_support = ecx & X86_CPUID1_ECX_x2APIC;

	/*
	 * We expect to be started in xAPIC mode. This means firmware has not
	 * irreversibly disabled the APIC and not yet enabled x2APIC mode.
	 */
	rdmsr(APIC_MSR_BASE, &eax, &edx);
	UK_BUGON(!(eax & APIC_BASE_EN));
	if (x2apic_support)
		UK_BUGON(eax & APIC_BASE_EXTD);

	/* Set APIC software enable flag */
	eax = ioreg_read32(APIC_MMIO_SVR);
	eax |= APIC_SVR_EN;
	ioreg_write32(APIC_MMIO_SVR, eax);

	/*
	 * Enable "virtual wire mode" to receive PIC interrupts on the BSP.
	 * Local interrupts are masked after reset and will remain masked on
	 * AP cores.
	 */
	if (lcpu_is_bsp(this_lcpu)) {
		eax =  APIC_LVT_DELIVERY_MODE_EXTINT;
		ioreg_write32(APIC_MMIO_LINT0, eax);

		eax = APIC_LVT_DELIVERY_MODE_NMI;
		ioreg_write32(APIC_MMIO_LINT1, eax);
	}

#ifdef CONFIG_HAVE_SMP
	/* We currently need x2APIC mode for SMP boot */
	if (!x2apic_support)
		return -ENOTSUP;

	/* Switch to x2APIC mode */
	rdmsr(APIC_MSR_BASE, &eax, &edx);
	eax |= APIC_BASE_EXTD;
	wrmsr(APIC_MSR_BASE, eax, edx);
#endif

	/*
	 * TODO: Configure spurious interrupt vector number
	 * After power-up or reset this is 0xff, which might not be
	 * configured in the trap table
	 */

	return 0;
}

static inline void x2apic_send_ipi(int irqno, int dest)
{
	__u32 eax;

	UK_ASSERT(((32 + irqno) & 0xff) == (32 + irqno));

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_FIXED
	      | (32 + irqno);

	wrmsr(APIC_MSR_ICR, eax, dest);
}

static inline void x2apic_send_self_ipi(int irqno)
{
	__u32 eax;

	UK_ASSERT(((32 + irqno) & 0xff) == (32 + irqno));

	eax = (32 + irqno);

	wrmsr(APIC_MSR_SELF_IPI, eax, 0);
}

static inline void x2apic_send_nmi(int dest)
{
	__u32 eax;

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_NMI;

	wrmsr(APIC_MSR_ICR, eax, dest);
}

static inline void x2apic_send_sipi(__vaddr_t addr, int dest)
{
	__u32 eax;

	UK_ASSERT((addr & (APIC_ICR_VECTOR_MASK << __PAGE_SHIFT)) == addr);

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_SUP
	      | (addr >> __PAGE_SHIFT);

	wrmsr(APIC_MSR_ICR, eax, dest);
}

static inline void x2apic_send_iipi(int dest)
{
	__u32 eax;

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_INIT;

	wrmsr(APIC_MSR_ICR, eax, dest);
}

/* Deassert only supported on Pentium and P6 familiy processors */
#define x2apic_send_iipi_deassert() {}

static inline void x2apic_clear_errors(void)
{
	wrmsr(APIC_MSR_ESR, 0, 0);
}

static inline void x2apic_ack_interrupt(void)
{
	wrmsr(APIC_MSR_EOI, 0, 0);
}

#define apic_send_ipi		x2apic_send_ipi
#define apic_send_nmi		x2apic_send_nmi
#define apic_send_sipi		x2apic_send_sipi
#define apic_send_iipi		x2apic_send_iipi
#define apic_send_iipi_deassert x2apic_send_iipi_deassert
#define apic_clear_errors	x2apic_clear_errors
#define apic_ack_interrupt	x2apic_ack_interrupt

#endif /* __PLAT_CMN_X86_APIC_H__ */
