/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_X86_TRAPS_H__
#define __HYPERLIGHT_X86_TRAPS_H__

#include <x86/traps.h>

/*
 * Global descriptor table (GDT)
 *
 * For Hyperlight we use a minimal GDT:
 * - Null descriptor (required)
 * - Code segment (0x08)
 * - Data segment (0x10)
 * - TSS segment (0x18) for exception stacks
 */
#define GDT_DESC_NULL		0
#define GDT_DESC_CODE		1
#define GDT_DESC_DATA		2
#define GDT_DESC_TSS_LO		3
#define GDT_DESC_TSS_HI		4
#define GDT_DESC_TSS		GDT_DESC_TSS_LO

#define GDT_DESC_TYPE_LDT	0x2
#define GDT_DESC_TYPE_TSS_AVAIL	0x9
#define GDT_DESC_TYPE_TSS_BUSY	0xb
#define GDT_DESC_TYPE_CALL	0xc
#define GDT_DESC_TYPE_INTR	0xe
#define GDT_DESC_TYPE_TRAP	0xf

#define GDT_DESC_DPL_KERNEL	0
#define GDT_DESC_DPL_USER	3

#define GDT_DESC_OFFSET(n)	((n) * 0x8)
#define GDT_NUM_ENTRIES		5

/* Seg. Limit                       : 0xfffff
 * Base                             : 0x00000000
 * Type                             : 0xa (execute/read/accessed)
 * Code or Data Segment (S)         : 0x1 (true)
 * Descriptor Privilege Level (DPL) : 0x0 (most privileged)
 * Segment Present (P)              : 0x1 (true)
 * Default Operation Size (D)       : 0x1 (32-bit)
 * Granularity (G)                  : 0x1 (4KiB)
 */
#define GDT_DESC_CODE32_VAL	0x00cf9b000000ffff

/* Seg. Limit                       : 0xfffff
 * Base                             : 0x00000000
 * Type                             : 0x3 (read/write/accessed)
 * Code or Data Segment (S)         : 0x1 (true)
 * Descriptor Privilege Level (DPL) : 0x0 (most privileged)
 * Segment Present (P)              : 0x1 (true)
 * Granularity (G)                  : 0x1 (4KiB)
 */
#define GDT_DESC_DATA32_VAL	0x00cf93000000ffff

/* Seg. Limit                       : 0xfffff
 * Base                             : 0x00000000
 * Type                             : 0xb (execute/read/accessed)
 * Code or Data Segment (S)         : 0x1 (true)
 * Descriptor Privilege Level (DPL) : 0x0 (most privileged)
 * Segment Present (P)              : 0x1 (true)
 * 64-bit Code Segment (L)          : 0x1 (true)
 * Granularity (G)                  : 0x1 (4KiB)
 */
#define GDT_DESC_CODE64_VAL	0x00af9b000000ffff
#define GDT_DESC_DATA64_VAL	GDT_DESC_DATA32_VAL

/*
 * Interrupt descriptor table (IDT)
 */
#define IDT_DESC_CODE		GDT_DESC_CODE

#define IDT_DESC_TYPE_INTR	GDT_DESC_TYPE_INTR

#define IDT_DESC_DPL_KERNEL	GDT_DESC_DPL_KERNEL
#define IDT_DESC_DPL_USER	GDT_DESC_DPL_USER

#define IDT_DESC_OFFSET(n)	GDT_DESC_OFFSET(n)
#define IDT_NUM_ENTRIES		256

#endif /* __HYPERLIGHT_X86_TRAPS_H__ */
