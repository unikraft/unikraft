/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Port from Mini-OS: include/x86/os.h
 */
/*
 * Copyright (c) 2009 Citrix Systems, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef __PLAT_CMN_X86_IRQ_H__
#define __PLAT_CMN_X86_IRQ_H__

#include <x86/cpu_defs.h>

#ifdef __X64_32__
#define __SZ  "l"
#define __REG "e"
#else
#define __SZ  "q"
#define __REG "r"
#endif

#define __cli() \
({ \
	asm volatile("cli" : : : "memory"); \
})

#define __sti() \
({ \
	asm volatile("sti" : : : "memory"); \
})

#define __save_flags(x) \
	do { \
		unsigned long __f; \
		asm volatile("pushf" __SZ " ; pop" __SZ " %0" : "=g"(__f)); \
		x = (__f & X86_EFLAGS_IF) ? 1 : 0; \
	} while (0)

#define __restore_flags(x) \
	do { \
		if (x) \
			__sti(); \
		else \
			__cli(); \
	} while (0)

#define __save_and_cli(x) \
	do { \
		__save_flags(x); \
		__cli(); \
	} while (0)

static inline int irqs_disabled(void)
{
	int flag;

	__save_flags(flag);
	return !flag;
}

#define local_irq_save(x)        __save_and_cli(x)
#define local_irq_restore(x)     __restore_flags(x)
#define local_save_flags(x)      __save_flags(x)
#define local_irq_disable()      __cli()
#define local_irq_enable()       __sti()

#define __MAX_IRQ	16

#endif /* __PLAT_CMN_X86_IRQ_H__ */
