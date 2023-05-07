/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2022, University of Bucharest. All rights reserved..
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
#ifndef __PLAT_CMN_RISCV64_IRQ_H__
#define __PLAT_CMN_RISCV64_IRQ_H__

#include <riscv/cpu.h>
#include <riscv/cpu_defs.h>

#define __set_sie() _csr_set(CSR_SSTATUS, SSTATUS_SIE)

#define __clear_sie() _csr_clear(CSR_SSTATUS, SSTATUS_SIE)

#define __save_flags(x)                                                        \
	({                                                                     \
		unsigned long __f;                                             \
		__f = _csr_read(CSR_SSTATUS);                                  \
		x = (__f & SSTATUS_SIE) ? 1 : 0;                               \
	})

#define __restore_flags(x)                                                     \
	({                                                                     \
		if (x)                                                         \
			__set_sie();                                           \
		else                                                           \
			__clear_sie();                                         \
	})

#define __save_and_clear_sie(x)                                                \
	({                                                                     \
		__save_flags(x);                                               \
		__clear_sie();                                                 \
	})

static inline int irqs_disabled(void)
{
	int flag;

	__save_flags(flag);
	return !flag;
}

#define local_irq_save(x) __save_and_clear_sie(x)
#define local_irq_restore(x) __restore_flags(x)
#define local_save_flags(x) __save_flags(x)
#define local_irq_disable() __clear_sie()
#define local_irq_enable() __set_sie()

#define __MAX_IRQ 1024

#endif /* __PLAT_CMN_RISCV64_IRQ_H__ */
