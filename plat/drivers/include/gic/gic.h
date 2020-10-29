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
#ifndef __PLAT_DRV_ARM_GIC_COMMON_H__
#define __PLAT_DRV_ARM_GIC_COMMON_H__

#include <stdint.h>
#include <uk/config.h>
#include <uk/plat/spinlock.h>
#include <uk/plat/common/irq.h>

/* SPI interrupt definitions */
#define GIC_SPI_TYPE 0
#define GIC_SPI_BASE 32

/* PPI interrupt definitions */
#define GIC_PPI_TYPE 1
#define GIC_PPI_BASE 16

/** Max support interrupt number for GIC */
#define GIC_MAX_IRQ  __MAX_IRQ

/** Distributor register address */
#define GIC_DIST_REG(gdev, r) ((void *)(gdev.dist_mem_addr + (r)))

/** IRQ type mask */
#define IRQ_TYPE_MASK 0x0000000f

/* Distributor lock functions */
#ifdef CONFIG_UKPLAT_LCPU_MULTICORE
#define dist_lock(gdev) ukarch_spin_lock(gdev.dist_lock)
#define dist_unlock(gdev) ukarch_spin_unlock(gdev.dist_lock)
#else
#define dist_lock(gdev) {}
#define dist_unlock(gdev) {}
#endif

/** GIC hardware version */
typedef enum _GIC_HW_VER {
	/** GIC version 2 */
	GIC_V2 = 2,
	/** GIC version 3 */
	GIC_V3,
	/** GIC version 4 */
	GIC_V4,
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
	void (*set_irq_type)(uint32_t irq, int trigger);
	/** Set priority for IRQ */
	void (*set_irq_prio)(uint32_t irq, uint8_t priority);
	/** Set IRQ affinity (or "target" for GICv2) */
	void (*set_irq_affinity)(uint32_t irq, uint8_t affinity);
	/** Translate to hwirq according to type e.g. PPI SPI SGI */
	int (*irq_translate)(uint32_t type, uint32_t hw_irq);
	/** Handle IRQ */
	void (*handle_irq)(void);
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
		/** CPU Interface base address. This field is used only by
		 * GICv2 driver since in versions above the CPU Interface
		 * is access through system registers (not memory mapped)
		 */
		uint64_t cpuif_mem_addr;
		/** Redistributor's base address (GICv3) */
		uint64_t rdist_mem_addr;
	};
	union {
		/** CPU Interface memory size. This field is used only by
		 * GICv2 driver since in versions above the CPU Interface
		 * is access through system registers (not memory mapped)
		 */
		uint64_t cpuif_mem_size;
		/** Redistributor's memory size */
		uint64_t rdist_mem_size;
	};
#ifdef UKPLAT_LCPU_MULTICORE
	/** Pointer to the Lock for distributor access */
	spinlock_t *dist_lock;
#endif
	/** Driver operations */
	struct _gic_operations ops;
};

/* Prototypes */
struct _gic_dev *_dtb_init_gic(const void *fdt);
int32_t gic_irq_translate(uint32_t type, uint32_t hw_irq);
#endif /* __PLAT_DRV_ARM_GIC_COMMON_H__ */
