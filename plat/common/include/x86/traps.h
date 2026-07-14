/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
/* Ported from Mini-OS */

#ifndef __UKARCH_TRAPS_X86_64_H__
#define __UKARCH_TRAPS_X86_64_H__

#include <uk/lcpu.h>

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
#define TRAP_security_error      21

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
DECLARE_ASM_TRAP(security_error);
#endif /* !__ASSEMBLY__ */

#endif /* __UKARCH_TRAPS_X86_64_H__ */
