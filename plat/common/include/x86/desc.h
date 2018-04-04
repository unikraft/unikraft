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
/*
 * Adapted from Mini-OS: include/x86/desc.h
 */

#ifndef __PLAT_CMN_X86_DESC_H__
#define __PLAT_CMN_X86_DESC_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>


/* Protected mode lgdt/lidt table pointer. */
struct desc_table_ptr32 {
	__u16 limit;
	__u32 base;
} __packed;

/* Long mode lgdt/lidt table pointer. */
struct desc_table_ptr64 {
	__u16 limit;
	__u64 base;
} __packed;


/* 8 byte user segment descriptor (GDT/LDT entries with .s = 1) */
struct seg_desc32 {
	union {
		/* Raw backing integers. */
		struct {
			__u32 lo, hi;
		};
		/* Common named fields. */
		struct {
			__u64 limit_lo:16;
			__u64 base_lo:24;
			__u64 type:4;
			__u64 s:1;
			__u64 dpl:2;
			__u64 p:1;
			__u64 limit_hi:4;
			__u64 avl:1;
			__u64 l:1;
			__u64 d:1;
			__u64 gran:1;
			__u64 base_hi:8;
		};
		/* Code segment specific field names. */
		struct {
			__u64 limit_lo:16;
			__u64 base_lo:24;
			__u64 a:1;
			__u64 r:1;
			__u64 c:1;
			__u64 x:1;
			__u64 s:1;
			__u64 dpl:2;
			__u64 p:1;
			__u64 limit_hi:4;
			__u64 avl:1;
			__u64 l:1;
			__u64 d:1;
			__u64 gran:1;
			__u64 base_hi:8;
		} code;
		/* Data segment specific field names. */
		struct {
			__u64 limit_lo:16;
			__u64 base_lo:24;
			__u64 a:1;
			__u64 w:1;
			__u64 e:1;
			__u64 x:1;
			__u64 s:1;
			__u64 dpl:2;
			__u64 p:1;
			__u64 limit_hi:4;
			__u64 avl:1;
			__u64 reserved:1;
			__u64 b:1;
			__u64 gran:1;
			__u64 base_hi:8;
		} data;

		__u64 raw;
	};
} __packed;

struct seg_desc64 {
	union {
		struct {
			__u64 lo, hi;
		};
		struct {
			__u64 limit_lo:16;
			__u64 base_lo:24;
			__u64 type:4;
			__u64 zero:1;
			__u64 dpl:2;
			__u64 p:1;
			__u64 limit_hi:4;
			__u64 avl:1;
			__u64 unused:2;
			__u64 gran:1;
			__u64 base_hi:40;
			__u64 reserved:8;
			__u64 zero1:5;
			__u64 reserved1:19;
		} __packed;
	};
} __packed;


/* 8-byte gate - Protected mode IDT entry, GDT task/call gate. */
struct seg_gate_desc32 {
	union {
		struct {
			__u32 lo, hi;
		};
		struct {
			__u32 offset_lo:16;
			__u32 selector:16;
			__u32 reserved:8;
			__u32 type:4;
			__u32 s:1;
			__u32 dpl:2;
			__u32 p:1;
			__u32 offset_hi:16;
		};
	};
} __packed;

/* 16-byte gate - Long mode IDT entry. */
struct seg_gate_desc64 {
	union {
		struct {
			__u64 lo, hi;
		};
		struct {
			__u64 offset_lo:16;
			__u64 selector:16;
			__u64 ist:3;
			__u64 reserved:5;
			__u64 type:4;
			__u64 s: 1;
			__u64 dpl: 2;
			__u64 p: 1;
			__u64 offset_hi:48;
			__u64 reserved1:32;
		} __packed;
	};
} __packed;


struct tss64 {
	__u32 reserved;
	__u64 rsp[3];
	__u64 reserved2;
	__u64 ist[7];	   /* 1-based structure */
	__u64 reserved3;
	__u16 reserved4;
	__u16 iomap_base;
} __packed;

#endif /* __PLAT_CMN_X86_DESC_H__ */
