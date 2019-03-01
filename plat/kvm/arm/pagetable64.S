/* SPDX-License-Identifier: BSD-2-Clause */
/*-
 * This code is based on:
 * https://github.com/freebsd/freebsd/blob/master/sys/arm64/arm64/locore.S
 *
 * Authors: Andrew Turner
 *          Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2012-2014 Andrew Turner. All rights reserved.
 * Copyright (c) 2018 Arm Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */
#include <uk/arch/limits.h>
#include <uk/asm.h>
#include <kvm-arm/mm.h>
#include <arm/cpu_defs.h>
/*
 * The following is the Memory Layout of AArch64 Virtual Machine
 * | 0 - 0x3FFFFFFF |   0x40000000 - 0x7FFFFFFFFF           |  512GB - 1TB   |
 * --------------------------------------------------------------------------
 * |  DEVICES MMIO  | DTB|TEXT|DATA|BSS|PAGETABLE|BOOTSTACK | PCI-e High Mem |
 * --------------------------------------------------------------------------
 */
#define DEVICE_ADDR_START 0
#define DEVICE_ADDR_SIZE  0x40000000
#define RAM_ADDR_START    0x40000000
#define RAM_L2_ENTRIES    255
#define RAM_ADDR_SIZE     (0x40000000 * RAM_L2_ENTRIES)
#define PCIE_ADDR_START   0x8000000000
#define PCIE_L2_ENTRIES   512
#define PCIE_ADDR_SIZE    0x8000000000

/*
 * As we use VA == PA mapping, so the VIRT_BITS must be the same
 * as PA_BITS. We can get PA_BITS from ID_AA64MMFR0_EL1.PARange.
 */
virt_bits:
	.byte 32, 36, 40, 42, 44, 48

/*
 * We will use a direct map for physical address and virtual address
 * (PA = VA 1:1 mapping)
 *  x15 = image size (trashed)
 */
ENTRY(create_pagetables)
	/* Save link address */
	mov x5, x30

	/* Pagetable starts from _end */
	ldr x14, =_end

	/*
	 * We have 2 L1 tables to map 0 ~ 512GB, 512 GB ~ 1TB.
	 * Link these two tables to L0 entries#0, #1
	 */
	mov x6, x14
	mov x8, #0
	add x9, x14, #L1_TABLE_OFFSET
	mov x10, #2
	bl  link_l0_pagetable

	/*
	 * Using 1GiB block to map device address space (0x0 ~ 0x3fffffff)
	 */
	add x6, x14, #L1_TABLE_OFFSET
	ldr x7, =SECT_ATTR_DEVICE_nGnRE
	mov x8, #DEVICE_ADDR_START
	mov x9, x8
	mov x10, #1
	bl  build_l1_block_pagetable

	/*
	 * Using 1GiB block to map RAM address space
	 * (0x40000000 ~ (256GiB -1)). The RAM areas that contain kernel
	 * sections will be update later.
	 */
	add x6, x14, #L1_TABLE_OFFSET
	ldr x7, =SECT_ATTR_NORMAL
	mov x8, #RAM_ADDR_START
	mov x9, x8
	mov x10, #255
	bl  build_l1_block_pagetable

	/*
	 * Using 1GiB block to map high PCIe address space
	 * (0x512GiB ~ (1TiB -1)).
	 */
	add x6, x14, #L1_TABLE_OFFSET
	add x6, x6, #__PAGE_SIZE
	ldr x7, =SECT_ATTR_DEVICE_nGnRE
	mov x8, #PCIE_ADDR_START
	mov x9, x8
	mov x10, #512
	bl  build_l1_block_pagetable

	/*
	 * The text, bss, data and other sections are placed at
	 * 0x40000000 ~ 0x7fffffff. We use 4k granularity to manage
	 * their attributes to improve security.
	 * L1 -> L2 -> L3 (4K)
	 */
	/* 1st: Build a L2 block pagetable */
	add x6, x14, #L2_TABLE_OFFSET
	ldr x7, =SECT_ATTR_NORMAL
	mov x8, #RAM_ADDR_START
	mov x9, x8
	mov x10, 512
	bl  build_l2_block_pagetable

	/* 2nd: Link this L2 block pagetable to L1 entry */
	add x6, x14, #L1_TABLE_OFFSET
	mov x8, #RAM_ADDR_START
	add x9, x14, #L2_TABLE_OFFSET
	bl  link_l1_pagetable

	/* 3rd: Build a L3 pagetable for image occupied memory */
	add x16, x14, #L3_TABLE_OFFSET
	mov x17, #RAM_ADDR_START

	mov x6, x16
	ldr x7, =SECT_ATTR_NORMAL
	mov x8, x17
	mov x9, x17
	lsr x10, x15, L3_SHIFT
	bl  build_l3_pagetable

	/* 4th: Link this L3 pagetable to L2 entry */
	add x6, x14, #L2_TABLE_OFFSET
1:
	mov x8, x17
	mov x9, x16
	bl  link_l2_pagetable

	/* Check whether the image mapping has been done */
	add x16, x16, #__PAGE_SIZE
	add x17, x17, #L2_SIZE
	sub x15, x15, #L2_SIZE
	cbnz x15, 1b

	/* 5th: Update dtb section to readonly */
	ldr x8, =_dtb
	ldr x9, =_text
	sub x10, x9, x8
	add x6, x14, #L3_TABLE_OFFSET
	ldr x7, =SECT_ATTR_NORMAL_RO
	mov x9, x8
	lsr x10, x10, L3_SHIFT
	bl  build_l3_pagetable

	/* 6th: Update text section to Readonly & EXEC*/
	ldr x8, =_text
	ldr x9, =_etext
	sub x10, x9, x8
	add x6, x14, #L3_TABLE_OFFSET
	ldr x7, =SECT_ATTR_NORMAL_EXEC
	mov x9, x8
	lsr x10, x10, L3_SHIFT
	bl  build_l3_pagetable

	/* 7th: Update rodata and Constructor tables sections to Readonly */
	ldr x8, =_rodata
	ldr x9, =_ectors
	sub x10, x9, x8
	add x6, x14, #L3_TABLE_OFFSET
	ldr x7, =SECT_ATTR_NORMAL_RO
	mov x9, x8
	lsr x10, x10, L3_SHIFT
	bl  build_l3_pagetable

	/* Restore the Link register */
	mov x30, x5

	ret
END(create_pagetables)

ENTRY(start_mmu)
	/*
	 * Using dsb here to guarantee the create_pagetables has
	 * been done.
	 */
	dsb sy

	/* Load ttbr0, pagetable starts from _end */
	ldr x27, =_end
	msr ttbr0_el1, x27
	isb

	/* Clear the Monitor Debug System control register */
	msr mdscr_el1, xzr

	/* Invalidate the TLB to avoid stale one */
	tlbi vmalle1
	dsb nsh

	ldr x2, =MAIR_INIT_ATTR
	msr mair_el1, x2

	/* Get VIRT_BITS from id_aa64mmfr0_el1.PARange */
	mrs x3, id_aa64mmfr0_el1
	ldr x5, =virt_bits
	ubfx x4, x3, #0, #4
	ldrb w4, [x5, x4]

	/* Setup TCR_TxSZ(64 - VIRT_BITS) for TCR_INIT_FLAGS */
	mov x5, #64
	sub x5, x5, x4
	mov x4, x5
	lsl x5, x5, #TCR_T1SZ_SHIFT
	orr x5, x4, x5
	ldr x2, =TCR_INIT_FLAGS
	orr x2, x5, x2
	bfi x2, x3, #32, #3
	msr tcr_el1, x2

	/* save lr */
	mov x22, x30

        /*
	 * Invalidate the D-Cache to avoid using invalid data that existed
	 * in D-Cache. Invalidate ranges that may have been modified:
	 * DATA, BSS, PAGETABLE and BOOTSTACK.
	 */
	ldr x0, =_data
	ldr x1, =_end
	ldr x2, =page_table_size
	ldr x2, [x2]
	add x1, x2, x1
	add x1, x1, #__STACK_SIZE
	sub x1, x1, x0
	bl clean_and_invalidate_dcache_range

	/* Setup SCTLR */
	ldr x2, =SCTLR_SET_BITS
	ldr x3, =SCTLR_CLEAR_BITS
	mrs x1, sctlr_el1
	bic x1, x1, x3	/* Clear the required bits */
	orr x1, x1, x2	/* Set the required bits */
	msr sctlr_el1, x1
	isb

	/* restore lr */
	mov x30, x22

	ret
END(start_mmu)

/*
 * Builds an L0 -> L1 table descriptor
 *
 * This is a link for a 512GiB block of memory with up to 1GiB regions mapped
 * within it by build_l1_block_pagetable.
 *
 *  x6  = L0 table
 *  x8  = Virtual Address
 *  x9  = L1 PA (trashed)
 *  x10 = Entry count
 *  x11, x12 and x13 are trashed
 */
link_l0_pagetable:
	/* Find the table index */
	lsr x11, x8, #L0_SHIFT
	and x11, x11, #L0_ADDR_MASK

	/* Build the L0 block entry */
	mov x12, #L0_TABLE

	/* Only use the output address bits */
	lsr x9, x9, #__PAGE_SHIFT
1:	orr x13, x12, x9, lsl #__PAGE_SHIFT

	/* Store the entry */
	str x13, [x6, x11, lsl #3]

	sub x10, x10, #1
	add x11, x11, #1
	add x9, x9, #1
	cbnz x10, 1b

	ret

/*
 * Builds an L1 -> L2 table descriptor
 *
 * This is a link for a 1GiB block of memory with up to 2MiB regions mapped
 * within it by build_l2_block_pagetable.
 *
 *  x6  = L1 table
 *  x8  = Virtual Address
 *  x9  = L2 PA (trashed)
 *  x11, x12 and x13 are trashed
 */
link_l1_pagetable:
	/* Find the table index */
	lsr x11, x8, #L1_SHIFT
	and x11, x11, #Ln_ADDR_MASK

	/* Build the L1 block entry */
	mov x12, #L1_TABLE

	/* Only use the output address bits */
	lsr x9, x9, #__PAGE_SHIFT
	orr x13, x12, x9, lsl #__PAGE_SHIFT

	/* Store the entry */
	str x13, [x6, x11, lsl #3]

	ret

/*
 * Builds count 1 GiB page table entry
 *  x6  = L1 table
 *  x7  = Memory attributes
 *  x8  = VA start
 *  x9  = PA start (trashed)
 *  x10 = Entry count
 *  x11, x12 and x13 are trashed
 */
build_l1_block_pagetable:
	/* Find the table index */
	lsr x11, x8, #L1_SHIFT
	and x11, x11, #Ln_ADDR_MASK

	/* Build the L1 block entry */
	mov x12, x7
#ifdef CONFIG_UKPLAT_LCPU_MULTICORE
	orr x12, x12, ATTR_SH(ATTR_SH_IS)
#endif

	/* Only use the output address bits */
	lsr x9, x9, #L1_SHIFT

	/* Set the physical address for this virtual address */
1:	orr x13, x12, x9, lsl #L1_SHIFT

	/* Store the entry */
	str x13, [x6, x11, lsl #3]

	sub x10, x10, #1
	add x11, x11, #1
	add x9, x9, #1
	cbnz x10, 1b

	ret

/*
 * Builds count 2 MiB page table entry
 *  x6  = L2 table
 *  x7  = Memory attributes (trashed)
 *  x8  = VA start
 *  x9  = PA start (trashed)
 *  x10 = Entry count
 *  x11, x12 and x13 are trashed
 */
build_l2_block_pagetable:
	/* Find the table index */
	lsr x11, x8, #L2_SHIFT
	and x11, x11, #Ln_ADDR_MASK

	/* Build the L2 block entry */
	mov x12, x7
#ifdef CONFIG_UKPLAT_LCPU_MULTICORE
	orr x12, x12, ATTR_SH(ATTR_SH_IS)
#endif

	/* Only use the output address bits */
	lsr x9, x9, #L2_SHIFT

	/* Set the physical address for this virtual address */
1:	orr x13, x12, x9, lsl #L2_SHIFT

	/* Store the entry */
	str x13, [x6, x11, lsl #3]

	sub x10, x10, #1
	add x11, x11, #1
	add x9, x9, #1
	cbnz x10, 1b

	ret

/*
 * Builds an L3 -> L2 table descriptor
 *
 * This is a link for a 2MiB block of memory with up to 4KiB regions mapped
 * within it by build_pte_pagetable.
 *
 *  x6  = L2 table
 *  x8  = Virtual Address
 *  x9  = L3 table PA (trashed)
 *  x11, x12 and x13 are trashed
 */
link_l2_pagetable:
	/* Find the table index */
	lsr x11, x8, #L2_SHIFT
	and x11, x11, #Ln_ADDR_MASK

	/* Build the L2 block entry */
	mov x12, #L2_TABLE

	/* Only use the output address bits */
	lsr x9, x9, #__PAGE_SHIFT
	orr x13, x12, x9, lsl #__PAGE_SHIFT

	/* Store the entry */
	str x13, [x6, x11, lsl #3]

	ret

/*
 * Builds count 4 KiB page table entry
 *  x6  = L3 table
 *  x7  = Memory attributes (trashed)
 *  x8  = VA start
 *  x9  = PA start (trashed)
 *  x10 = Entry count
 *  x11, x12 and x13 are trashed
 */
build_l3_pagetable:
	/* Find the first L3 table to be updated */
	lsr x11, x8, #L2_SHIFT
	and x11, x11, #Ln_ADDR_MASK
	lsl x11, x11, #L3_SHIFT
	add x6, x11, x6

	/* Find the first entry index in L3 table */
	lsr x11, x8, #L3_SHIFT
	and x11, x11, #Ln_ADDR_MASK

	/* Build the L3 entry */
	mov x12, x7
	orr x12, x12, #L3_PAGE
#ifdef CONFIG_UKPLAT_LCPU_MULTICORE
	orr x12, x12, ATTR_SH(ATTR_SH_IS)
#endif

	/* Only use the output address bits */
	lsr x9, x9, #L3_SHIFT

	/* Set the physical address for this virtual address */
1:	orr x13, x12, x9, lsl #L3_SHIFT

	/* Store the entry */
	str x13, [x6, x11, lsl #3]
	add x11, x11, #1

	/* Cross L3 Page? */
	and x11, x11, #Ln_ADDR_MASK
	cbnz x11, 2f

	/* To next L3 table */
	add x6, x6, #__PAGE_SIZE

2:	add x9, x9, #1
	sub x10, x10, #1
	cbnz x10, 1b

	ret
