/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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

#ifndef __PLAT_COMMON_ARM64_CPU_H__
#define __PLAT_COMMON_ARM64_CPU_H__

#include <inttypes.h>
#include <uk/essentials.h>
#include <uk/plat/common/sw_ctx.h>
#include <uk/alloc.h>
#include <uk/assert.h>

/*
 * we should use inline assembly with volatile constraint to access mmio
 * device memory to avoid compiler use load/store instructions of writeback
 * addressing mode which will cause crash when running in hyper mode
 * unless they will be decoded by hypervisor.
 */
static inline uint8_t ioreg_read8(const volatile uint8_t *address)
{
	uint8_t value;

	asm volatile ("ldrb %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline uint16_t ioreg_read16(const volatile uint16_t *address)
{
	uint16_t value;

	asm volatile ("ldrh %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline uint32_t ioreg_read32(const volatile uint32_t *address)
{
	uint32_t value;

	asm volatile ("ldr %w0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline uint64_t ioreg_read64(const volatile uint64_t *address)
{
	uint64_t value;

	asm volatile ("ldr %0, [%1]" : "=r"(value) : "r"(address));
	return value;
}

static inline void ioreg_write8(const volatile uint8_t *address, uint8_t value)
{
	asm volatile ("strb %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write16(const volatile uint16_t *address,
				 uint16_t value)
{
	asm volatile ("strh %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write32(const volatile uint32_t *address,
				 uint32_t value)
{
	asm volatile ("str %w0, [%1]" : : "rZ"(value), "r"(address));
}

static inline void ioreg_write64(const volatile uint64_t *address,
				 uint64_t value)
{
	asm volatile ("str %0, [%1]" : : "rZ"(value), "r"(address));
}

static inline void _init_cpufeatures(void)
{
}

/* Define compatibility IO macros */
#define outb(addr, v)   UK_BUG()
#define outw(addr, v)   UK_BUG()
#define inb(addr)       UK_BUG()

/* Macros to access system registers */
#define SYSREG_READ(reg) \
({	uint64_t val; \
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg) \
			: "=r" (val)); \
	val; \
})

#define SYSREG_WRITE(reg, val) \
	__asm__ __volatile__("msr " __STRINGIFY(reg) ", %0" \
			: : "r" ((uint64_t)(val)))

#define SYSREG_READ32(reg) \
({	uint32_t val; \
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg) \
			: "=r" (val)); \
	val; \
})

#define SYSREG_WRITE32(reg, val) \
	__asm__ __volatile__("msr " __STRINGIFY(reg) ", %0" \
			: : "r" ((uint32_t)(val)))

#define SYSREG_READ64(reg) SYSREG_READ(reg)
#define SYSREG_WRITE64(reg, val) SYSREG_WRITE(reg, val)

/*
 * PSCI conduit method to call functions, based on the SMC Calling
 * Convention.
 */
typedef int (*smcc_psci_callfn_t)(uint32_t, uint64_t, uint64_t, uint64_t);
extern smcc_psci_callfn_t smcc_psci_call;
int32_t smcc_psci_hvc_call(uint32_t, uint64_t, uint64_t, uint64_t);
int32_t smcc_psci_smc_call(uint32_t, uint64_t, uint64_t, uint64_t);

/* CPU native APIs */
void halt(void);
void reset(void);
void system_off(void);

#ifdef CONFIG_FPSIMD
struct fpsimd_state {
	__u64		regs[32 * 2];
	__u32		fpsr;
	__u32		fpcr;
};

extern void fpsimd_save_state(uintptr_t ptr);
extern void fpsimd_restore_state(uintptr_t ptr);

static inline void save_extregs(struct sw_ctx *ctx)
{
	fpsimd_save_state(ctx->extregs);
}

static inline void restore_extregs(struct sw_ctx *ctx)
{
	fpsimd_restore_state(ctx->extregs);
}

static inline struct sw_ctx *arch_alloc_sw_ctx(struct uk_alloc *allocator)
{
	struct sw_ctx *ctx;

	ctx = (struct sw_ctx *)uk_malloc(allocator,
			sizeof(struct sw_ctx) + sizeof(struct fpsimd_state));
	if (ctx)
		ctx->extregs = (uintptr_t)((void *)ctx + sizeof(struct sw_ctx));

	uk_pr_debug("Allocating %lu + %lu bytes for sw ctx at %p, extregs at %p\n",
			sizeof(struct sw_ctx), sizeof(struct fpsimd_state),
			ctx, (void *)ctx->extregs);

	return ctx;
}

static inline void arch_init_extregs(struct sw_ctx *ctx __unused)
{
}

#else /* !CONFIG_FPSIMD */
static inline void save_extregs(struct sw_ctx *ctx __unused)
{
}

static inline void restore_extregs(struct sw_ctx *ctx __unused)
{
}

static inline struct sw_ctx *arch_alloc_sw_ctx(struct uk_alloc *allocator)
{
	struct sw_ctx *ctx;

	ctx = (struct sw_ctx *)uk_malloc(allocator, sizeof(struct sw_ctx));
	uk_pr_debug("Allocating %lu bytes for sw ctx at %p\n",
		sizeof(struct sw_ctx), ctx);

	return ctx;
}

static inline void arch_init_extregs(struct sw_ctx *ctx)
{
	ctx->extregs = (uintptr_t)ctx + sizeof(struct sw_ctx);
}

#endif /* CONFIG_FPSIMD */
#endif /* __PLAT_COMMON_ARM64_CPU_H__ */
