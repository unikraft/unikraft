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

#ifndef __UK_ARCH_APIC_H__
#define __UK_ARCH_APIC_H__

/* APIC MSR registers */
#define APIC_MSR_BASE			0x01b
#define APIC_MSR_TSC_DEADLINE		0x6e0

/* The following MSRs are only accessible in x2APIC mode */
#define APIC_MSR_ID			0x802
#define APIC_MSR_VER			0x803
#define APIC_MSR_TPR			0x808
#define APIC_MSR_PPR			0x80a
#define APIC_MSR_EOI			0x80b
#define APIC_MSR_LDR			0x80d
#define APIC_MSR_SVR			0x80f
#define APIC_MSR_ISR(x)			(0x810 + (x))
#define APIC_MSR_TMR(x)			(0x818 + (x))
#define APIC_MSR_IRR(x)			(0x820 + (x))
#define APIC_MSR_ESR			0x828
#define APIC_MSR_LVT_CMCI		0x82f
#define APIC_MSR_ICR			0x830
#define APIC_MSR_LVT_TIMER		0x832
#define APIC_MSR_LVT_THERMAL		0x833
#define APIC_MSR_LVT_PERF		0x834
#define APIC_MSR_LVT_LINT0		0x835
#define APIC_MSR_LVT_LINT1		0x836
#define APIC_MSR_LVT_ERROR		0x837
#define APIC_MSR_TIMER_IC		0x838
#define APIC_MSR_TIMER_CC		0x839
#define APIC_MSR_TIMER_DCR		0x83e
#define APIC_MSR_SELF_IPI		0x83f

/* APIC BASE register */
#define APIC_BASE_BSP			(1 << 8)
#define APIC_BASE_EXTD			(1 << 10)
#define APIC_BASE_EN			(1 << 11)
#define APIC_BASE_ADDR_SHIFT		12
#define APIC_BASE_ADDR_MASK		0x0000000ffffff000UL

/* APIC spurious interrupt vector register (SVR) */
#define APIC_SVR_EN			(1 << 8)
#define APIC_SVR_VECTOR_MASK		0x00000000000000ffUL
#define APIC_SVR_EOI_BROADCAST		(1 << 12)

/* APIC error status registers (ESR) */
#define APIC_ESR_SEND_CHECKSUM		(1 << 0) /* only Pentium and P6 */
#define APIC_ESR_RECV_CHECKSUM		(1 << 1) /* only Pentium and P6 */
#define APIC_ESR_SEND_ACCEPT		(1 << 2) /* only Pentium and P6 */
#define APIC_ESR_RECV_ACCEPT		(1 << 3) /* only Pentium and P6 */
#define APIC_ESR_REDIRECTABLE_IPI	(1 << 4)
#define APIC_ESR_SEND_ILLEGAL_VECTOR	(1 << 5)
#define APIC_ESR_RECV_ILLEGAL_VECTOR	(1 << 6)
#define APIC_ESR_ILLEGAL_REGISTER	(1 << 7)

/* APIC interrupt command register (ICR) */
#define APIC_ICR_VECTOR_MASK		0x000000ff

#define APIC_ICR_DMODE_FIXED		(0 << 8)
#define APIC_ICR_DMODE_SMI		(2 << 8)
#define APIC_ICR_DMODE_NMI		(4 << 8)
#define APIC_ICR_DMODE_INIT		(5 << 8)
#define APIC_ICR_DMODE_SUP		(6 << 8)

#define APIC_ICR_DESTMODE_PHYSICAL	0
#define APIC_ICR_DESTMODE_LOGICAL	(1 << 11)
#define APIC_ICR_LEVEL_DEASSERT		0
#define APIC_ICR_LEVEL_ASSERT		(1 << 14)
#define APIC_ICR_TRIGGER_EDGE		0
#define APIC_ICR_TRIGGER_LEVEL		(1 << 15)

#define APIC_ICR_DSTSH_NO		(0 << 18)
#define APIC_ICR_DSTSH_SELF		(1 << 18)
#define APIC_ICR_DSTSH_ALL_INCL_SELF	(2 << 18)
#define APIC_ICR_DSTSH_ALL_EXCL_SELF	(3 << 18)

/* LVT registers */
#define APIC_LVT_MASK_MASK		(0x1 << 16)
#define APIC_LVT_VECTOR_MASK		(0xff)

/* APIC Timer modes */
#define APIC_TIMER_MODE_MASK		(0x3 << 17)
#define APIC_TIMER_MODE_ONE_SHOT	(0x0 << 17)
#define APIC_TIMER_MODE_PERIODIC	(0x1 << 17)
#define APIC_TIMER_MODE_TSC_DEADLINE	(0x2 << 17)

#endif /* __UK_ARCH_APIC_H__ */
