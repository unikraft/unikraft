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

#include <inttypes.h>

static inline uint8_t ioreg_read8(const volatile uint8_t *addr)
{
	return *addr;
}

static inline void ioreg_write8(volatile uint8_t *addr, uint8_t value)
{
	*addr = value;
}

static inline uint16_t ioreg_read16(const volatile uint16_t *addr)
{
	return *addr;
}

static inline void ioreg_write16(volatile uint16_t *addr, uint16_t value)
{
	*addr = value;
}

static inline uint32_t ioreg_read32(const volatile uint32_t *addr)
{
	return *addr;
}

static inline void ioreg_write32(volatile uint32_t *addr, uint32_t value)
{
	*addr = value;
}

static inline uint64_t ioreg_read64(const volatile uint64_t *addr)
{
	return *addr;
}

static inline void ioreg_write64(volatile uint64_t *addr, uint64_t value)
{
	*addr = value;
}

/* Define compatibility IO macros */
#define outb(addr, v)   UK_BUG()
#define outw(addr, v)   UK_BUG()
#define inb(addr)       UK_BUG()

/* Macros to access system registers */
#define SYSREG_READ(reg) \
({	uint64_t val; \
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg) \
			: "=&r" (val)); \
	val; \
})

#define SYSREG_WRITE(reg, val) \
	__asm__ __volatile__("msr " __STRINGIFY(reg) ", %0" \
			: : "r" ((uint64_t)(val)))

#define SYSREG_READ32(reg) \
({	uint32_t val; \
	__asm__ __volatile__("mrs %0, " __STRINGIFY(reg) \
			: "=&r" (val)); \
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
