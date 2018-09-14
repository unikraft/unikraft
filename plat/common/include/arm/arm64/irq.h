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
#ifndef __PLAT_CMN_ARM64_IRQ_H__
#define __PLAT_CMN_ARM64_IRQ_H__

/*
 * SPSR_EL1, Saved Program Status Register
 * When the exception is taken in AArch64:
 * M[3:2] is the exception level
 * M[1]   is unused
 * M[0]   is the SP select:
 *         0: always SP0
 *         1: current ELs SP
 */
#define PSR_M_EL0t	0x00000000
#define PSR_M_EL1t	0x00000004
#define PSR_M_EL1h	0x00000005
#define PSR_M_EL2t	0x00000008
#define PSR_M_EL2h	0x00000009
#define PSR_M_MASK	0x0000000f

#define PSR_AARCH32	0x00000010
#define PSR_F		0x00000040
#define PSR_I		0x00000080
#define PSR_A		0x00000100
#define PSR_D		0x00000200
#define PSR_IL		0x00100000
#define PSR_SS		0x00200000
#define PSR_V		0x10000000
#define PSR_C		0x20000000
#define PSR_Z		0x40000000
#define PSR_N		0x80000000
#define PSR_FLAGS	0xf0000000

#define __disable_irq() \
({ \
	__asm __volatile( "msr daifset, #2" : : : "memory" ); \
})

#define __enable_irq() \
({ \
	__asm __volatile( "msr daifclr, #2" : : : "memory" ); \
})

#define __save_flags(x) \
({ \
	__asm __volatile( "mrs %x0, daif" : "=&r" (x) : : ); \
})

#define __restore_flags(x) \
({ \
	__asm __volatile ( "msr daif, %0" : : "r" (x) : "memory" ); \
})

#define __save_and_disable_irq(x) \
({ \
	__save_flags(x); \
	__disable_irq(); \
})

static inline int irqs_disabled(void)
{
	uint64_t flags;
	__save_flags(flags);
	return !(flags & PSR_I);
}

#define local_irq_save(x)	__save_and_disable_irq(x)
#define local_irq_restore(x)	__restore_flags(x)
#define local_save_flags(x)	__save_flags(x)
#define local_irq_disable()	__disable_irq()
#define local_irq_enable()	__enable_irq()

#endif /* __PLAT_CMN_ARM64_IRQ_H__ */
