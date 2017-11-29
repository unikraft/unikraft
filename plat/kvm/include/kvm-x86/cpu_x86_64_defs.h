/* SPDX-License-Identifier: ISC */
/*
 * Authors: Martin Lucina
 *
 * Copyright (c) 2016-2017 Docker, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _BITUL

#define _AC(X, Y)               X
#define _AT(T, X)               X
#else
#define __AC(X, Y)              (X##Y)
#define _AC(X, Y)               __AC(X, Y)
#define _AT(T, X)               ((T)(X))
#endif

#define _BITUL(x)               (_AC(1, UL) << (x))
#define _BITULL(x)              (_AC(1, ULL) << (x))

/*
 * Basic CPU control in CR0
 */
#define X86_CR0_MP_BIT          1 /* Monitor Coprocessor */
#define X86_CR0_MP              _BITUL(X86_CR0_MP_BIT)
#define X86_CR0_EM_BIT          2 /* Emulation */
#define X86_CR0_EM              _BITUL(X86_CR0_EM_BIT)
#define X86_CR0_NE_BIT          5 /* Numeric Exception */
#define X86_CR0_NE              _BITUL(X86_CR0_NE_BIT)
#define X86_CR0_PG_BIT          31 /* Paging */
#define X86_CR0_PG              _BITUL(X86_CR0_PG_BIT)

/*
 * Intel CPU features in CR4
 */
#define X86_CR4_PAE_BIT         5 /* enable physical address extensions */
#define X86_CR4_PAE             _BITUL(X86_CR4_PAE_BIT)
#define X86_CR4_OSFXSR_BIT      9 /* OS support for FXSAVE/FXRSTOR */
#define X86_CR4_OSFXSR          _BITUL(X86_CR4_OSFXSR_BIT)
#define X86_CR4_OSXMMEXCPT_BIT  10 /* OS support for FP exceptions */
#define X86_CR4_OSXMMEXCPT      _BITUL(X86_CR4_OSXMMEXCPT_BIT)

/*
 * Intel CPU features in EFER
 */
#define X86_EFER_LME_BIT        8 /* Long mode enable (R/W) */
#define X86_EFER_LME            _BITUL(X86_EFER_LME_BIT)

/* Needed by mem.c */
#define PAGE_SIZE               4096
//#define PAGE_SHIFT              12
#define PAGE_MASK               ~(0xfff)

/*
 * GDT layout
 *
 * This should be kept consistent with the layout used by the ukvm target (as
 * defined in ukvm/ukvm_cpu_x86_64.h.
 */
#define GDT_DESC_NULL           0
#define GDT_DESC_CODE           1
#define GDT_DESC_CODE32         2 /* Used by boot.S on virtio targets */
#define GDT_DESC_DATA           3
#define GDT_DESC_TSS_LO         4
#define GDT_DESC_TSS_HI         5
#define GDT_DESC_TSS            GDT_DESC_TSS_LO

#define GDT_DESC_OFFSET(n)      ((n) * 0x8)
#define GDT_NUM_ENTRIES         6

#define GDT_DESC_CODE_VAL       0x00af99000000ffff
#define GDT_DESC_DATA_VAL       0x00cf93000000ffff
