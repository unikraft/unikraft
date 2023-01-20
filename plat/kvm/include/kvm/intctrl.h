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
#ifndef __PLAT_KVM_X86_INTCTRL_H
#define __PLAT_KVM_X86_INTCTRL_H

#include <stdint.h>
#include <uk/plat/common/irq.h>

struct _pic_operations {
	/** Initialize PIC controller */
	int (*initialize)(void);
	/** Acknowledging IRQ */
	void (*ack_irq)(uint32_t irq);
	/** Enable IRQ */
	void (*enable_irq)(uint32_t irq);
	/** Disable IRQ */
	void (*disable_irq)(uint32_t irq);
	/** Set IRQ trigger type */
	void (*set_irq_type)(uint32_t irq, enum uk_irq_trigger trigger);
	/** Set priority for IRQ */
	void (*set_irq_prio)(uint32_t irq, uint8_t priority);
	/** Select destination processor */
	void (*set_irq_affinity)(uint32_t irq, uint8_t affinity);
	/** Handle IRQ */
	void (*handle_irq)(void);
	/* Get max IRQs */
	uint32_t (*get_max_irqs)(void);
};

struct _pic_dev {
	/* Probe status */
	uint8_t is_probed;
	/* Interrupt controller status */
	uint8_t is_initialized;
	/* Interrupt controller operations */
	struct _pic_operations ops;
};

/* Initialize the interrupt controller 
 */
void intctrl_init(void);

/* Unmask the interrupt
 *
 * @param irq
 *  The interrupt to be unmasked
 */
void intctrl_clear_irq(uint32_t irq);

/* Mask the interrupt
 *
 * @param irq
 *  The interrupt to be mask
 */
void intctrl_mask_irq(uint32_t irq);

/* Acknowlege the interrupt
 *
 * @param irq
 *  The interrupt to be acknowledge
 */
void intctrl_ack_irq(uint32_t irq);

/* Send interprocessor interrupt
 *
 * @param cpuid
 *  The destination cpuid
 *
 * @sgintid
 *  The softward generated interrupt id
 */
void intctrl_send_ipi(uint8_t sgintid, uint32_t cpuid);

#endif
