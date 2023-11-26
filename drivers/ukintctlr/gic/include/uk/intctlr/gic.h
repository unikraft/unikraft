/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, OpenSynergy GmbH. All rights reserved.
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
#ifndef __UK_INTCTLR_GIC_H__
#define __UK_INTCTLR_GIC_H__

#include <stdint.h>
#include <uk/config.h>
#include <uk/intctlr.h>
#include <uk/plat/spinlock.h>

/* Shared Peripheral Interrupt (SPI) definitions */
#define GIC_SPI_TYPE 0
#define GIC_SPI_BASE 32

/* Private Peripheral Interrupt (PPI) definitions */
#define GIC_PPI_TYPE 1
#define GIC_PPI_BASE 16

/** Distributor register address */
#define GIC_DIST_REG(gdev, r) ((void *)(gdev.dist_mem_addr + (r)))

/** IRQ type mask */
#define IRQ_TYPE_MASK 0x0000000f

/* Distributor lock functions */
#ifdef CONFIG_HAVE_SMP
#define dist_lock(gdev) ukarch_spin_lock(gdev.dist_lock)
#define dist_unlock(gdev) ukarch_spin_unlock(gdev.dist_lock)
#else /* CONFIG_HAVE_SMP */
#define dist_lock(gdev) {}
#define dist_unlock(gdev) {}
#endif /* !CONFIG_HAVE_SMP */

/** GIC hardware version */
typedef enum _GIC_HW_VER {
	/** GIC version 2 */
	GIC_V2 = 2,
	/** GIC version 3 */
	GIC_V3
} GIC_HW_VER;

/** GIC driver operations */
struct _gic_operations {
	/** Initialize GIC controller */
	int (*initialize)(void);
	/** Acknowledging IRQ */
	uint32_t (*ack_irq)(void);
	/** Finish interrupt handling */
	void (*eoi_irq)(uint32_t irq);
	/** Enable IRQ */
	void (*enable_irq)(uint32_t irq);
	/** Disable IRQ */
	void (*disable_irq)(uint32_t irq);
	/** Set IRQ trigger type */
	void (*set_irq_trigger)(uint32_t irq,
				enum uk_intctlr_irq_trigger trigger);
	/** Set priority for IRQ */
	void (*set_irq_prio)(uint32_t irq, uint8_t priority);
	/** Set IRQ affinity (or "target" for GICv2) */
	void (*set_irq_affinity)(uint32_t irq, uint32_t affinity);
	/** Handle IRQ */
	void (*handle_irq)(struct __regs *regs);
	/** Send a SGI to the specifiec core */
	void (*gic_sgi_gen)(uint8_t sgintid, uint32_t cpuid);
};

/** GIC controller structure */
struct _gic_dev {
	/** GIC hardware version */
	GIC_HW_VER version;
	/** Indicates if GIC is present */
	uint8_t is_present;
	/** Probe status */
	uint8_t is_probed;
	/** GIC status */
	uint8_t is_initialized;
	/** Distributor base address */
	uint64_t dist_mem_addr;
	/** Distributor memory size */
	uint64_t dist_mem_size;
	union {
		/**
		 * CPU Interface base address. This field is used only by
		 * GICv2 driver since in versions above the CPU interface
		 * is accessed through system registers (not memory mapped)
		 */
		uint64_t cpuif_mem_addr;
		/** Redistributor's base address (GICv3) */
		uint64_t rdist_mem_addr;
	};
	union {
		/**
		 * CPU Interface memory size. This field is used only by
		 * GICv2 driver since in versions above the CPU interface
		 * is accessed through system registers (not memory mapped)
		 */
		uint64_t cpuif_mem_size;
		/** Redistributor's memory size (GICv3) */
		uint64_t rdist_mem_size;
	};
#ifdef CONFIG_HAVE_SMP
	/** Pointer to the lock for distributor access */
	__spinlock * dist_lock;
#endif /* CONFIG_HAVE_SMP */

	/** Driver operations */
	struct _gic_operations ops;
};

/**
 * Fetch data from an existing MADT's GICD table.
 *
 * @param _gic_dev The driver whose memory base and size to fill in
 *
 * @return 0 on success, < 0 otherwise
 */
#if defined(CONFIG_UKPLAT_ACPI)
int acpi_get_gicd(struct _gic_dev *g);
#endif /* CONFIG_UKPLAT_ACPI */

#endif /* __UK_INTCTLR_GIC_H__ */
