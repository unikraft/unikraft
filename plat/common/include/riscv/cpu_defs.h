/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __PLAT_CMN_RISCV_CPU_DEFS_H__
#define __PLAT_CMN_RISCV_CPU_DEFS_H__

#include <uk/asm.h>

#define _UL(x) (_AC(x, UL))

#ifdef __ASSEMBLER__
#define __ASM_STR(x) x
#else
#define __ASM_STR(x) #x
#endif

#define SATP64_MODE _UL(0xF000000000000000)
#define SATP64_ASID _UL(0x0FFFF00000000000)
#define SATP64_PPN _UL(0x00000FFFFFFFFFFF)
#define SATP_MODE_OFF _UL(0)
#define SATP_MODE_SV39 _UL(8)

#define SATP64_MODE_SHIFT 60
#define SATP64_ASID_SHIFT 44

/* SSTATUS Register Fields */
#define SSTATUS_SIE _UL(0x00000002)
#define SSTATUS_SPIE_SHIFT 5
#define SSTATUS_SPIE (_UL(1) << SSTATUS_SPIE_SHIFT)
#define SSTATUS_SPP_SHIFT 8
#define SSTATUS_SPP (_UL(1) << SSTATUS_SPP_SHIFT)
#define SSTATUS_FS _UL(0x00006000)
#define SSTATUS_XS _UL(0x00018000)
#define SSTATUS_VS _UL(0x01800000)
#define SSTATUS_SUM _UL(0x00040000)
#define SSTATUS_MXR _UL(0x00080000)
#define SSTATUS32_SD _UL(0x80000000)
#define SSTATUS64_UXL _ULL(0x0000000300000000)
#define SSTATUS64_SD _ULL(0x8000000000000000)

/* Supervisor Trap Setup */
#define CSR_SSTATUS 0x100
#define CSR_SEDELEG 0x102
#define CSR_SIDELEG 0x103
#define CSR_SIE 0x104
#define CSR_STVEC 0x105
#define CSR_SCOUNTEREN 0x106

/* Supervisor Trap Handling */
#define CSR_SSCRATCH 0x140
#define CSR_SEPC 0x141
#define CSR_SCAUSE 0x142
#define CSR_STVAL 0x143
#define CSR_SIP 0x144

/* Supervisor Protection and Translation */
#define CSR_SATP 0x180

/* Time counter */
#define CSR_TIME 0xc01

/* Trap handling modes */
#define STVEC_MODE_DIRECT 0x0
#define STVEC_MODE_VECTORED 0x1

/* IRQs */
#define IRQ_S_SOFT 1
#define IRQ_S_TIMER 5
#define IRQ_S_EXT 9

#define SIP_SSIP (_UL(1) << IRQ_S_SOFT)
#define SIP_STIP (_UL(1) << IRQ_S_TIMER)
#define SIP_SEIP (_UL(1) << IRQ_S_EXT)

/* Trap/Exception Causes */

#define CAUSE_MISALIGNED_FETCH 0x0
#define CAUSE_FETCH_ACCESS 0x1
#define CAUSE_ILLEGAL_INSTRUCTION 0x2
#define CAUSE_BREAKPOINT 0x3
#define CAUSE_MISALIGNED_LOAD 0x4
#define CAUSE_LOAD_ACCESS 0x5
#define CAUSE_MISALIGNED_STORE 0x6
#define CAUSE_STORE_ACCESS 0x7
#define CAUSE_USER_ECALL 0x8
#define CAUSE_SUPERVISOR_ECALL 0x9
#define CAUSE_VIRTUAL_SUPERVISOR_ECALL 0xa
#define CAUSE_MACHINE_ECALL 0xb
#define CAUSE_FETCH_PAGE_FAULT 0xc
#define CAUSE_LOAD_PAGE_FAULT 0xd
#define CAUSE_STORE_PAGE_FAULT 0xf
#define CAUSE_FETCH_GUEST_PAGE_FAULT 0x14
#define CAUSE_LOAD_GUEST_PAGE_FAULT 0x15
#define CAUSE_VIRTUAL_INST_FAULT 0x16
#define CAUSE_STORE_GUEST_PAGE_FAULT 0x17
#define CAUSE_INTERRUPT (_UL(1) << (__riscv_xlen - 1))
#define CAUSE_SUPERVISOR_SOFT (CAUSE_INTERRUPT | IRQ_S_SOFT)
#define CAUSE_SUPERVISOR_TIMER (CAUSE_INTERRUPT | IRQ_S_TIMER)
#define CAUSE_SUPERVISOR_EXT (CAUSE_INTERRUPT | IRQ_S_EXT)

#endif
