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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
/* Ported from Mini-OS */

#ifndef __UKARCH_TRAPS_X86_64_H__
#define __UKARCH_TRAPS_X86_64_H__

#include <x86/regs.h>

#define TRAP_divide_error        0
#define TRAP_debug               1
#define TRAP_nmi                 2
#define TRAP_int3                3
#define TRAP_overflow            4
#define TRAP_bounds              5
#define TRAP_invalid_op          6
#define TRAP_no_device           7
#define TRAP_double_fault        8
#define TRAP_invalid_tss         10
#define TRAP_no_segment          11
#define TRAP_stack_error         12
#define TRAP_gp_fault            13
#define TRAP_page_fault          14
#define TRAP_coproc_error        16
#define TRAP_alignment_check     17
#define TRAP_machine_check       18
#define TRAP_simd_error          19
#define TRAP_virt_error          20
#define TRAP_security_error      30

#define ASM_TRAP_SYM(trapname)   asm_trap_##trapname

#ifndef __ASSEMBLY__

#define DECLARE_ASM_TRAP(trapname) \
	void ASM_TRAP_SYM(trapname)(void)

/*
 * These are assembler stubs in entry.S.
 * They are the actual entry points for virtual exceptions.
 */
DECLARE_ASM_TRAP(divide_error);
DECLARE_ASM_TRAP(debug);
DECLARE_ASM_TRAP(nmi);
DECLARE_ASM_TRAP(int3);
DECLARE_ASM_TRAP(overflow);
DECLARE_ASM_TRAP(bounds);
DECLARE_ASM_TRAP(invalid_op);
DECLARE_ASM_TRAP(no_device);
DECLARE_ASM_TRAP(double_fault);
DECLARE_ASM_TRAP(invalid_tss);
DECLARE_ASM_TRAP(no_segment);
DECLARE_ASM_TRAP(stack_error);
DECLARE_ASM_TRAP(gp_fault);
DECLARE_ASM_TRAP(page_fault);
DECLARE_ASM_TRAP(coproc_error);
DECLARE_ASM_TRAP(alignment_check);
DECLARE_ASM_TRAP(machine_check);
DECLARE_ASM_TRAP(simd_error);
DECLARE_ASM_TRAP(virt_error);


void do_unhandled_trap(int trapnr, char *str, struct __regs *regs,
		unsigned long error_code);

#define DECLARE_TRAP(name, str) \
void do_##name(struct __regs *regs) \
{ \
	do_unhandled_trap(TRAP_##name, str, regs, 0); \
}

#define DECLARE_TRAP_EC(name, str) \
void do_##name(struct __regs *regs, unsigned long error_code) \
{ \
	do_unhandled_trap(TRAP_##name, str, regs, error_code); \
}


void traps_init(void);
void traps_fini(void);

#endif

#endif /* __UKARCH_TRAPS_X86_64_H__ */
