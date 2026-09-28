/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/asm.h>
#include <uk/arch/arm64.h>
#include <uk/asm/lcpu.h>

/* Hyperlight layout (from hyperlight/mem.h and hyperlight/outb.h) */
#define HL_SCRATCH_SIZE_GVA	0x0000FFFFFFFFDFF8
#define HL_SCRATCH_ALLOC_GVA	0x0000FFFFFFFFDFF0
#define HL_MAX_GPA		0x0000000FFFFFBFFF
#define HL_MAX_GVA		0x0000FFFFFFFFDFFF
#define HL_IO_PAGE_GVA		0x0000FFFFFFFFE000
#define HL_HALT_GVA		(HL_IO_PAGE_GVA + 8 * 108)
#define HL_ABORT_GVA		(HL_IO_PAGE_GVA + 8 * 102)
#define HL_DEBUG_PRINT_GVA	(HL_IO_PAGE_GVA + 8 * 103)

#define PTE_ADDR_MASK		0x0000FFFFFFFFF000	/* bits 47:12 */
#define PTE_AP_RO		(1 << 7)		/* AP[2] */
#define PTE_COW_BIT		55			/* software CoW marker */
#define HL_PAGE_SIZE		4096

/*
 * MAIR_EL1 as Unikraft's arm64 code expects it (see uk/asm/lcpu.h), but
 * with attribute 0 normal write-back rather than Device-nGnRnE: every
 * page Hyperlight maps uses attribute 0, and would otherwise be device
 * memory.  Nothing on this platform maps a device.
 */
#define HL_MAIR_EL1						\
	((MAIR_INIT_ATTR & ~0xff) | MAIR_NORMAL_WB)

/*
 * Boot stack — 4 KiB in .bss (CoW pages, must be pre-faulted
 * before use).
 */
.section .bss
.balign 16
.space 4096
lcpu_bootstack:

/*
 * TPIDR_EL0 across a halt: the TLS pointer of the thread the VM halted
 * on.  Hyperlight keeps only a few system registers of an arm64 guest, not
 * this one, and a restored guest, or one HVF moved to a new vCPU between
 * calls, would come back with it zeroed.  Saved by every halt, reloaded
 * by every dispatch entry, and so part of a snapshot's memory.
 */
.section .bss
.balign 8
.globl hl_halt_tpidr_el0
hl_halt_tpidr_el0:
	.quad	0

/*
 * Hyperlight entry arguments — saved to .data so that C code can
 * access them.  Writing to .data triggers CoW faults, so the CoW
 * handler must be installed before we reach this point.
 */
.section .data
.balign 8
.globl hyperlight_entry_args
hyperlight_entry_args:
	.quad	0	/* peb_address   (from x0) */
	.quad	0	/* seed          (from x1) */
	.quad	0	/* page_size     (from x2) */
	.quad	0	/* max_log_level (from x3) */

/*
 * _hyperlight_entry — entry point for the Hyperlight platform.
 *
 * Hyperlight starts the guest at EL1 with the MMU on and:
 *   x0 = PEB address    (shared memory layout)
 *   x1 = random seed
 *   x2 = host page size
 *   x3 = max log level
 *   SP_EL0 = scratch stack (always writable), SPSel = 0
 *   TTBR0_EL1 = host-provided page tables, TTBR1_EL1 walks enabled
 *   VBAR_EL1 = SP_EL1 = 0, IRQ and FIQ masked
 *
 * Writable guest pages (.data, .bss) are marked Copy-on-Write by the
 * host: read-only, with descriptor bit 55 set.  Any write to these pages
 * faults.  Before touching any writable memory outside the scratch
 * region we need vectors to take the fault and a stack to take it on:
 * the early vectors below run the CoW handler on the stack in use, so
 * we move to EL1h on the scratch stack first.
 */
.section .text.boot
ENTRY(_hyperlight_entry)
	/* ---- Save entry args in callee-saved registers --------- */
	mov	x19, x0		/* peb_address */
	mov	x20, x1		/* seed */
	mov	x21, x2		/* page_size */
	mov	x22, x3		/* max_log_level */

	/* ---- CPU state Hyperlight leaves to the guest ----------- */
	/* Exceptions on the stack in use: EL1h on the scratch stack */
	mov	x9, sp
	msr	spsel, #1
	mov	sp, x9

	adr	x9, hl_early_vectors
	msr	vbar_el1, x9

	/* Nothing lives in the upper half: stop TTBR1 walks */
	mrs	x9, tcr_el1
	orr	x9, x9, #UK_ARCH_ARM64_TCR_EL1_EPD1_BIT
	msr	tcr_el1, x9

	ldr	x9, =HL_MAIR_EL1
	msr	mair_el1, x9

	msr	contextidr_el1, xzr
	isb

	/* Per-CPU data of the only CPU */
	adrp	x9, _uk_pcpuvar_base
	add	x9, x9, #:lo12:_uk_pcpuvar_base
	msr	tpidr_el1, x9

	/*
	 * ---- Pre-fault the boot stack ----------------------------
	 *
	 * The boot stack is 4096 bytes and may span two CoW pages.
	 * Trigger CoW resolution for both pages now, while SP is
	 * still on the scratch stack.
	 */
	adrp	x9, lcpu_bootstack
	add	x9, x9, #:lo12:lcpu_bootstack
	str	xzr, [x9, #-8]
	sub	x10, x9, #0x1000
	str	xzr, [x10]

	/* Save to .data (triggers CoW faults — handler is ready) */
	adrp	x10, hyperlight_entry_args
	add	x10, x10, #:lo12:hyperlight_entry_args
	stp	x19, x20, [x10]
	stp	x21, x22, [x10, #16]

	/* Switch to the boot stack and enter C */
	mov	sp, x9
	adrp	x0, uk_lcpus
	add	x0, x0, #:lo12:uk_lcpus
	mov	x1, x10
	mov	x29, xzr
	mov	x30, xzr
	b	hyperlight_entry
END(_hyperlight_entry)

/*
 * hyperlight_dispatch_function — entry point for guest function calls.
 *
 * The host starts every call here at EL1t on the stack the last halt left
 * in SP_EL0, with all general-purpose and FP registers zeroed.  After a
 * snapshot restore it starts one instruction in instead: the page tables
 * were moved, and the TLB has to go.
 *
 * Every entry moves to EL1h on the scratch exception stack
 * (g_exn_stack_top, queried from the host at init) and sets the per-CPU
 * pointer: under HVF the vCPU running this call may not be the one that
 * ran the last (see hyperlight_out32()).  After a restore it also:
 *
 *   1. flushes the TLB,
 *   2. runs on the early vectors, whose CoW handler takes faults on the
 *      scratch stack, while it pre-faults the native PAL's exception
 *      stacks (restore made them copy-on-write again),
 *   3. goes back to the native vectors,
 *   4. carries the monotonic clock on from the snapshot, and
 *   5. re-initialises the paging frame allocator, whose state lived in
 *      scratch.
 */
.text
.balign 4
ENTRY(hyperlight_dispatch_function)
	b	1f			/* +0: normal dispatch */

	/* +4: first call after a snapshot restore */
	tlbi	vmalle1
	dsb	ish
	isb
	bl	hl_dispatch_cpu_state

	adr	x9, hl_early_vectors
	msr	vbar_el1, x9
	isb
	bl	hyperlight_dispatch_prefault
	adrp	x9, vector_table
	add	x9, x9, #:lo12:vector_table
	msr	vbar_el1, x9
	isb

	bl	hyperlight_time_restore
	bl	hyperlight_paging_reinit
	b	2f

1:	bl	hl_dispatch_cpu_state
2:	bl	hyperlight_dispatch_inner

	/* Halt — port 108 = dispatch complete; x0 = where to come back. */
	mrs	x9, tpidr_el0
	adrp	x10, hl_halt_tpidr_el0
	str	x9, [x10, #:lo12:hl_halt_tpidr_el0]
	mov	x9, sp
	msr	sp_el0, x9
	msr	spsel, #0
	adr	x0, hyperlight_dispatch_function
	ldr	x9, =HL_HALT_GVA
	str	x0, [x9]
3:	b	3b
END(hyperlight_dispatch_function)

/*
 * Stack and thread pointers for a dispatch: SPSel 1, SP_EL1 on the
 * scratch exception stack, TPIDR_EL1 at CPU 0's per-CPU data, TPIDR_EL0
 * back to what it was when the VM halted.  Leaf routine; returns on the
 * new stack.
 */
hl_dispatch_cpu_state:
	msr	spsel, #1
	adrp	x9, g_exn_stack_top
	ldr	x9, [x9, #:lo12:g_exn_stack_top]
	mov	sp, x9
	adrp	x9, _uk_pcpuvar_base
	add	x9, x9, #:lo12:_uk_pcpuvar_base
	msr	tpidr_el1, x9
	adrp	x9, hl_halt_tpidr_el0
	ldr	x9, [x9, #:lo12:hl_halt_tpidr_el0]
	msr	tpidr_el0, x9
	isb
	ret

/*
 * Early vectors.
 *
 * Active from _hyperlight_entry's VBAR write until the native PAL's
 * vectors take over (setup.c), and again while a dispatch after a snapshot restore
 * pre-faults the native exception stacks.  Only a synchronous exception
 * at EL1 is expected, and only a CoW write fault is handled: anything
 * else is reported through DebugPrint and aborts the guest.
 */
.macro hl_early_vector label
.balign 0x80
	b	\label
.endm

.balign 0x800
hl_early_vectors:
	hl_early_vector	hl_early_sync	/* Synchronous EL1t */
	hl_early_vector	hl_early_bad	/* IRQ EL1t */
	hl_early_vector	hl_early_bad	/* FIQ EL1t */
	hl_early_vector	hl_early_bad	/* SError EL1t */
	hl_early_vector	hl_early_sync	/* Synchronous EL1h */
	hl_early_vector	hl_early_bad	/* IRQ EL1h */
	hl_early_vector	hl_early_bad	/* FIQ EL1h */
	hl_early_vector	hl_early_bad	/* SError EL1h */
	hl_early_vector	hl_early_bad	/* Synchronous EL0 (AArch64) */
	hl_early_vector	hl_early_bad	/* IRQ EL0 (AArch64) */
	hl_early_vector	hl_early_bad	/* FIQ EL0 (AArch64) */
	hl_early_vector	hl_early_bad	/* SError EL0 (AArch64) */
	hl_early_vector	hl_early_bad	/* Synchronous EL0 (AArch32) */
	hl_early_vector	hl_early_bad	/* IRQ EL0 (AArch32) */
	hl_early_vector	hl_early_bad	/* FIQ EL0 (AArch32) */
	hl_early_vector	hl_early_bad	/* SError EL0 (AArch32) */

/*
 * The CoW write fault handler.  Self-contained: uses only the stack in
 * use (scratch or an already faulted-in one) and no BSS, so it is safe
 * while BSS pages are still CoW.
 *
 *   1. Check the syndrome: a data abort from EL1, a level-3 permission
 *      fault, caused by a write that is not a cache maintenance op
 *   2. Walk the 4-level table from TTBR0_EL1; the tables are in the
 *      scratch region, reached through the scratch GPA->GVA offset
 *   3. Check descriptor bit 55 (the CoW marker)
 *   4. Bump-allocate a fresh page from scratch
 *   5. Copy the page to it
 *   6. Break-before-make: invalidate the old descriptor and its TLB
 *      entry, then write the new one, writable and without the marker
 */
hl_early_sync:
	sub	sp, sp, #96
	stp	x0, x1, [sp, #0]
	stp	x2, x3, [sp, #16]
	stp	x4, x5, [sp, #32]
	stp	x6, x7, [sp, #48]
	stp	x8, x9, [sp, #64]
	stp	x10, x11, [sp, #80]

	/* ---- Is this a CoW write fault? ------------------------- */
	mrs	x0, esr_el1
	ubfx	x1, x0, #UK_ARCH_ARM64_ESR_EC_SHIFT, #6
	cmp	x1, #UK_ARCH_ARM64_ESR_EL1_EC_MMU_DABRT_EL1
	b.ne	hl_early_unhandled
	and	x1, x0, #UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_MASK
	cmp	x1, #UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_PERM_L3
	b.ne	hl_early_unhandled
	tbz	x0, #6, hl_early_unhandled	/* ISS.WnR: a write */
	tbnz	x0, #8, hl_early_unhandled	/* ISS.CM: cache maintenance */

	/* ---- Scratch GPA -> GVA offset -------------------------- */
	ldr	x1, =HL_SCRATCH_SIZE_GVA
	ldr	x1, [x1]			/* x1 = scratch size */
	cbz	x1, hl_early_unhandled
	ldr	x2, =HL_MAX_GPA
	sub	x2, x2, x1
	add	x2, x2, #1			/* x2 = scratch base GPA */
	ldr	x3, =HL_MAX_GVA
	sub	x3, x3, x1
	add	x3, x3, #1			/* x3 = scratch base GVA */
	sub	x3, x3, x2			/* x3 = GVA - GPA */

	/* ---- Walk levels 0-2 ------------------------------------ */
	ldr	x8, =PTE_ADDR_MASK
	mrs	x4, far_el1
	bic	x4, x4, #(HL_PAGE_SIZE - 1)	/* x4 = faulting page */
	mrs	x5, ttbr0_el1
	and	x5, x5, x8			/* x5 = table GPA */
	mov	x6, #39				/* x6 = index shift */
1:	lsr	x7, x4, x6
	and	x7, x7, #0x1ff
	add	x7, x5, x7, lsl #3
	ldr	x7, [x7, x3]
	and	x1, x7, #3
	cmp	x1, #3				/* a table descriptor */
	b.ne	hl_early_unhandled
	and	x5, x7, x8
	sub	x6, x6, #9
	cmp	x6, #12
	b.gt	1b

	/* ---- Level 3: the page descriptor ----------------------- */
	ubfx	x7, x4, #12, #9
	add	x5, x5, x7, lsl #3
	add	x5, x5, x3			/* x5 = descriptor GVA */
	ldr	x6, [x5]			/* x6 = descriptor */
	tbz	x6, #PTE_COW_BIT, hl_early_unhandled

	/* ---- Bump-allocate one page from scratch ---------------- */
	ldr	x0, =HL_SCRATCH_ALLOC_GVA
2:	ldxr	x7, [x0]			/* x7 = new page GPA */
	add	x1, x7, #HL_PAGE_SIZE
	stxr	w2, x1, [x0]
	cbnz	w2, 2b
	dmb	ish

	/* ---- Copy the page -------------------------------------- */
	add	x0, x7, x3			/* x0 = new page GVA */
	mov	x1, x4
	mov	x2, #(HL_PAGE_SIZE / 16)
3:	ldp	x9, x10, [x1], #16
	stp	x9, x10, [x0], #16
	subs	x2, x2, #1
	b.ne	3b

	/* ---- Remap: break, then make ---------------------------- */
	mov	x1, #PTE_AP_RO
	orr	x1, x1, x8
	orr	x1, x1, #(1 << PTE_COW_BIT)
	bic	x6, x6, x1			/* flags, writable, no marker */
	orr	x6, x6, x7			/* new output address */
	str	xzr, [x5]
	dsb	ishst
	lsr	x1, x4, #12
	tlbi	vaae1is, x1
	dsb	ish
	str	x6, [x5]
	dsb	ishst
	isb

	ldp	x10, x11, [sp, #80]
	ldp	x8, x9, [sp, #64]
	ldp	x6, x7, [sp, #48]
	ldp	x4, x5, [sp, #32]
	ldp	x2, x3, [sp, #16]
	ldp	x0, x1, [sp, #0]
	add	sp, sp, #96
	eret

/*
 * Print one register in hex through DebugPrint, one exit per character.
 * Clobbers x0-x3, x9.
 */
.macro hl_early_print_hex reg
	mov	x0, \reg
	ldr	x9, =HL_DEBUG_PRINT_GVA
	mov	x1, #60
4:	lsr	x2, x0, x1
	and	x2, x2, #0xf
	cmp	x2, #10
	add	x3, x2, #'0'
	add	x2, x2, #('a' - 10)
	csel	x2, x3, x2, lo
	str	x2, [x9]
	subs	x1, x1, #4
	b.ge	4b
.endm

.macro hl_early_print_char c
	mov	x2, #\c
	str	x2, [x9]
.endm

/*
 * Anything but a CoW fault: print FAR, ESR and ELR, then abort.  Port
 * 102 carries up to three bytes of message per exit and ends with 0xFF;
 * a single 0xFF terminator is enough to stop the guest.
 */
hl_early_bad:
hl_early_unhandled:
	mrs	x4, far_el1
	mrs	x5, esr_el1
	mrs	x6, elr_el1
	ldr	x9, =HL_DEBUG_PRINT_GVA
	hl_early_print_char 'E'
	hl_early_print_char 'X'
	hl_early_print_char ':'
	hl_early_print_hex x4
	hl_early_print_char ' '
	hl_early_print_char 'E'
	hl_early_print_char ':'
	hl_early_print_hex x5
	hl_early_print_char ' '
	hl_early_print_char 'R'
	hl_early_print_char ':'
	hl_early_print_hex x6
	hl_early_print_char 10

	ldr	x9, =HL_ABORT_GVA
	mov	x0, #0xff01
	str	x0, [x9]
5:	b	5b
