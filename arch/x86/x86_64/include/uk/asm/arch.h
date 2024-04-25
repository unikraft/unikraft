/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_LCPU_H__
#error Do not include this header directly
#endif

#include <uk/asm/apic.h>
#include <uk/asm/fsgsbase.h>
#include <uk/asm/pic.h>

/* EFLAGS register */
#define X86_EFLAGS_CF           (1 <<  0)   /* carry flag                */
#define X86_EFLAGS_PF           (1 <<  2)   /* parity flag               */
#define X86_EFLAGS_AF           (1 <<  4)   /* auxiliary flag            */
#define X86_EFLAGS_ZF           (1 <<  6)   /* zero flag                 */
#define X86_EFLAGS_SF           (1 <<  7)   /* sign flag                 */
#define X86_EFLAGS_TF           (1 <<  8)   /* trap flag                 */
#define X86_EFLAGS_IF           (1 <<  9)   /* interrupt flag            */
#define X86_EFLAGS_DF           (1 << 10)   /* direction flag            */
#define X86_EFLAGS_OF           (1 << 11)   /* overflow flag             */
#define X86_EFLAGS_NT           (1 << 14)   /* nested task flag          */
#define X86_EFLAGS_RF           (1 << 16)   /* resume flag               */
#define X86_EFLAGS_VM           (1 << 17)   /* virtual 8086 mode flag    */
#define X86_EFLAGS_AC           (1 << 18)   /* alignment check flag      */
#define X86_EFLAGS_VIF          (1 << 19)   /* virtual interrupt flag    */
#define X86_EFLAGS_VIP          (1 << 20)   /* virtual interrupt pending */
#define X86_EFLAGS_ID           (1 << 21)   /* ID flag                   */

/*
 * Basic CPU control in CR0
 */
#define X86_CR0_PE              (1 << 0)    /* Protection Enable */
#define X86_CR0_MP              (1 << 1)    /* Monitor Coprocessor */
#define X86_CR0_EM              (1 << 2)    /* Emulation */
#define X86_CR0_TS              (1 << 3)    /* Task Switched */
#define X86_CR0_NE              (1 << 5)    /* Numeric Exception */
#define X86_CR0_WP              (1 << 16)   /* Write Protect */
#define X86_CR0_PG              (1 << 31)   /* Paging */

/*
 * Intel CPU features in CR4
 */
#define X86_CR4_PAE             (1 << 5)    /* enable PAE */
#define X86_CR4_OSFXSR          (1 << 9)    /* OS support for FXSAVE/FXRSTOR */
#define X86_CR4_OSXMMEXCPT      (1 << 10)   /* OS support for FP exceptions */
#define X86_CR4_FSGSBASE        (1 << 16)   /* enable FSGSBASE*/
#define X86_CR4_OSXSAVE         (1 << 18)   /* enable XSAVE, extended states */
#define X86_CR4_PKE             (1 << 22)   /* enable protection keys */

/*
 * Intel CPU features in EFER
 */
#define X86_EFER_LME            (1 << 8)    /* Long mode enable (R/W) */

/*
 * Extended Control Register 0 (XCR0)
 */
#define X86_XCR0_X87            (1 << 0)
#define X86_XCR0_SSE            (1 << 1)
#define X86_XCR0_AVX            (1 << 2)
#define X86_XCR0_PKRU		(1 << 9)

/*
 * Model-specific register addresses
 */
/* FS and GS Registers in 64-bit Mode */
#define X86_MSR_FS_BASE         0xc0000100
#define X86_MSR_GS_BASE         0xc0000101
/* Used in conjunction with swapgs instruction */
#define X86_MSR_KERNEL_GS_BASE	0xc0000102
/* extended feature register */
#define X86_MSR_EFER		0xc0000080
/* legacy mode SYSCALL target */
#define X86_MSR_STAR		0xc0000081
/* long mode SYSCALL target */
#define X86_MSR_LSTAR		0xc0000082
/* compat mode SYSCALL target */
#define X86_MSR_CSTAR		0xc0000083
/* EFLAGS mask for syscall */
#define X86_MSR_SYSCALL_MASK	0xc0000084
/* page attribute table configuration */
#define X86_MSR_PAT		0x277

/* MSR EFER bits */
#define X86_EFER_SCE		(1 << 0)
#define X86_EFER_LME		(1 << 8)
#define X86_EFER_LMA		(1 << 10)
#define X86_EFER_NXE		(1 << 11)
#define X86_EFER_SVME		(1 << 12)
#define X86_EFER_LMSLE		(1 << 13)
#define X86_EFER_FFXSR		(1 << 14)
#define X86_EFER_TCE		(1 << 15)
