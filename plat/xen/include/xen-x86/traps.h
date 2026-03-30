/* SPDX-License-Identifier: MIT */
/*
 ****************************************************************************
 * (C) 2005 - Grzegorz Milos - Intel Reseach Cambridge
 ****************************************************************************
 *
 *        File: traps.h
 *      Author: Grzegorz Milos (gm281@cam.ac.uk)
 *
 *        Date: Jun 2005
 *
 * Environment: Xen Minimal OS
 * Description: Deals with traps
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _TRAPS_H_
#define _TRAPS_H_

#if !__ASSEMBLY__
#include <stdint.h>
#endif /* !__ASSEMBLY__ */

#include <uk/arch.h>

#include <xen/xen.h>

/* Xen-specific traps; using the UK_ARCH_ prefix to be compatible with macros */
#define UK_ARCH_X86_64_TRAPNUM_COPROC_SEG_OVERRUN 9
#define UK_ARCH_X86_64_TRAPNUM_SPURIOUS_INT       15
#define UK_ARCH_X86_64_TRAPNUM_XEN_CALLBACK       32

#define ASM_TRAP_SYM(trapname)   asm_trap_##trapname

#if !__ASSEMBLY__

#define DECLARE_ASM_TRAP(trapname) \
	void ASM_TRAP_SYM(trapname)(void)

/* Assembler stubs for x86 traps supported in Xen */
DECLARE_ASM_TRAP(DIVIDE_ERROR);
DECLARE_ASM_TRAP(DEBUG);
/* no NMI */
DECLARE_ASM_TRAP(INT3);
DECLARE_ASM_TRAP(OVERFLOW);
DECLARE_ASM_TRAP(BOUNDS);
DECLARE_ASM_TRAP(INVALID_OP);
DECLARE_ASM_TRAP(NO_DEVICE);
/* no double-fault */
DECLARE_ASM_TRAP(INVALID_TSS);
DECLARE_ASM_TRAP(NO_SEGMENT);
DECLARE_ASM_TRAP(STACK_ERROR);
DECLARE_ASM_TRAP(GP_FAULT);
DECLARE_ASM_TRAP(PAGE_FAULT);
DECLARE_ASM_TRAP(COPROC_ERROR);
DECLARE_ASM_TRAP(ALIGNMENT_CHECK);
/* no machine check */
DECLARE_ASM_TRAP(SIMD_ERROR);
/* no virtualization exception */
DECLARE_ASM_TRAP(SECURITY_ERROR);

/* Assembler stubs for Xen-specific traps */
DECLARE_ASM_TRAP(COPROC_SEG_OVERRUN);
DECLARE_ASM_TRAP(SPURIOUS_INT);
DECLARE_ASM_TRAP(HYPERVISOR_CALLBACK);

/* Trap init routine; to be called at startup */
void xen_traps_init(void);

/* Failsafe trap handler that pops the stack and returns */
void asm_failsafe_callback(void);

#endif /* !__ASSEMBLY__ */

#define __KERNEL_CS     FLAT_KERNEL_CS
#define __KERNEL_DS     FLAT_KERNEL_DS
#define __KERNEL_SS     FLAT_KERNEL_SS

#endif /* _TRAPS_H_ */
