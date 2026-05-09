/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/asm.h>
#include <uk/lcpu.h>

/* Boot startup args offsets (struct lcpu_sargs equivalent) */
#define LCPU_SARGS_ENTRY_OFFSET		0x00
#define LCPU_SARGS_STACKP_OFFSET	0x08

/* Hyperlight scratch memory layout constants */
#define SCRATCH_SIZE_GVA    0xFFFFFFFFFFFFEFF8
#define ALLOCATOR_GVA       0xFFFFFFFFFFFFEFF0
#define HL_MAX_GPA          0x0000000FFFFFFFFF
#define HL_MAX_GVA          0xFFFFFFFFFFFFEFFF
#define PTE_ADDR_MASK       0x000FFFFFFFFFF000
#define PTE_COW_BIT         (1 << 9)
#define PAGE_SIZE_4K        4096

/**
 * Stack to use during CPU initialization
 */
.section .bss
.space 4096
lcpu_bootstack:

/**
 * Storage for startup arguments
 */
lcpu_boot_startup_args:
	.quad	0	/* entry */
	.quad	0	/* stackp */

/**
 * Storage for Hyperlight entry arguments
 */
.section .data
.globl hyperlight_entry_args
hyperlight_entry_args:
	.quad	0	/* peb_address (RDI) */
	.quad	0	/* seed (RSI) */
	.quad	0	/* page_size (RDX) */
	.quad	0	/* max_log_level (RCX) */

/**
 * 64-bit Hyperlight entry function
 *
 * Hyperlight sets up the guest with:
 *   - RDI: PEB address
 *   - RSI: Seed
 *   - RDX: Page size
 *   - RCX: Max log level
 *   - RSP: Stack pointer (on scratch stack, always writable)
 *   - Paging enabled with identity mapping
 *   - Long mode (64-bit) enabled
 *   - Interrupts disabled
 *
 * When running without the unikernel_guest feature, Hyperlight maps
 * writable guest pages (.data, .bss) as Copy-on-Write: the write bit
 * is cleared and PTE bit 9 (AVL_COW) is set.  Any write triggers a #PF.
 *
 * To handle this, we set up a minimal GDT and IDT on the scratch stack
 * (always writable) with an assembly-level #PF handler that resolves
 * CoW faults on demand.  This runs until traps_table_init() installs
 * the proper IDT later in the boot sequence.
 */
.code64
.section .text.boot
ENTRY(_hyperlight_entry)


	/* ---- Save entry args in callee-saved registers ----------- */
	movq	%rdi, %rbx	/* peb_address */
	movq	%rsi, %r15	/* seed */
	movq	%rdx, %r14	/* page_size */
	movq	%rcx, %r13	/* max_log_level */

	/* ---- Check if CoW is active (scratch_size > 0) ----------- */
	movabs	$SCRATCH_SIZE_GVA, %rax
	movq	(%rax), %r8		/* r8 = scratch_size */
	testq	%r8, %r8
	jz	.Lcow_setup_done	/* No scratch -> no CoW -> skip */

	/*
	 * ---- Set up a minimal GDT on the scratch stack -----------
	 *
	 * Hyperlight starts the guest with no GDT.  We need one so
	 * that the IDT entries can reference a valid code segment.
	 *
	 * GDT layout (3 entries = 24 bytes):
	 *   [0] null descriptor
	 *   [1] 64-bit code segment  (selector 0x08)
	 *   [2] 64-bit data segment  (selector 0x10)
	 *
	 * We also reserve 240 bytes for the IDT (entries 0..14).
	 * Total reserved: 24 (GDT) + 240 (IDT) = 264, rounded to 272
	 * for 16-byte stack alignment.
	 */
	subq	$272, %rsp
	/* Stack layout:
	 *   RSP +   0 .. 239 : IDT (15 entries x 16 bytes)
	 *   RSP + 240 .. 263 : GDT (3 entries x 8 bytes)
	 *   RSP + 264 .. 271 : alignment padding
	 */

	/* Write GDT entries */
	leaq	240(%rsp), %rax
	movq	$0, (%rax)			/* [0] null */
	movabs	$0x00af9b000000ffff, %rcx
	movq	%rcx, 8(%rax)			/* [1] code64 */
	movabs	$0x00cf93000000ffff, %rcx
	movq	%rcx, 16(%rax)			/* [2] data64 */

	/* Load the GDT (GDTR descriptor on scratch stack, temporary) */
	subq	$16, %rsp
	movw	$23, (%rsp)			/* limit = 24 - 1 */
	leaq	(16 + 240)(%rsp), %rax		/* GDT base address */
	movq	%rax, 2(%rsp)
	lgdt	(%rsp)
	addq	$16, %rsp

	/* Reload CS via far return */
	pushq	$0x08
	leaq	.Lgdt_loaded(%rip), %rax
	pushq	%rax
	lretq
.Lgdt_loaded:

	/* Set remaining segment registers */
	movl	$0x10, %eax
	movl	%eax, %ss
	movl	%eax, %ds
	movl	%eax, %es
	xorl	%eax, %eax
	movl	%eax, %fs
	movl	%eax, %gs

	/*
	 * ---- Set up a minimal IDT with a #PF handler -------------
	 *
	 * We only populate entry #14 (page fault).  All other entries
	 * are zeroed (not-present), so any non-#PF exception during
	 * early boot still results in a triple fault - the same
	 * behaviour as running without an IDT.
	 */

	/* Zero the IDT area (RSP+0 .. RSP+239) */
	movq	%rsp, %rdi
	xorq	%rax, %rax
	movq	$30, %rcx		/* 240 / 8 = 30 qwords */
	cld
	rep stosq

	/* Fill IDT entry #14 at RSP + 14x16 = RSP + 224 */
	leaq	_cow_asm_pf_handler(%rip), %rax
	movw	%ax, 224(%rsp)		/* offset[15:0]  */
	movw	$0x08, 226(%rsp)	/* selector = code64 */
	movb	$0, 228(%rsp)		/* IST = 0 */
	movb	$0x8E, 229(%rsp)	/* P=1 DPL=0 type=0xE (int gate) */
	shrq	$16, %rax
	movw	%ax, 230(%rsp)		/* offset[31:16] */
	shrq	$16, %rax
	movl	%eax, 232(%rsp)		/* offset[63:32] */
	/* bytes 236..239 already 0 (reserved) */

	/* Load the IDT */
	subq	$16, %rsp
	movw	$239, (%rsp)		/* limit = 15x16 - 1 */
	leaq	16(%rsp), %rax		/* IDT base */
	movq	%rax, 2(%rsp)
	lidt	(%rsp)
	addq	$16, %rsp

	/*
	 * ---- Pre-fault the boot stack ----------------------------
	 *
	 * lcpu_start64 will switch RSP to lcpu_bootstack (.bss).
	 * The 4096-byte stack spans up to two pages (lcpu_bootstack is
	 * not guaranteed to be page-aligned).  Pre-fault both pages now
	 * while RSP is still on the scratch stack, before the early asm
	 * #PF handler switches to the new stack.
	 */
	leaq	lcpu_bootstack(%rip), %rax
	movq	$0, -8(%rax)
	movq	$0, -0x1008(%rax)	/* second page: lcpu_bootstack - 4096 - 8 */

.Lcow_setup_done:

	/* ---- Restore entry args ---------------------------------- */
	movq	%rbx, %rdi
	movq	%r15, %rsi
	movq	%r14, %rdx
	movq	%r13, %rcx

	/* ---- Original entry sequence ----------------------------- */
	/* Save the Hyperlight entry arguments to .data */
	leaq	hyperlight_entry_args(%rip), %rax
	movq	%rdi, 0(%rax)	/* peb_address */
	movq	%rsi, 8(%rax)	/* seed */
	movq	%rdx, 16(%rax)	/* page_size */
	movq	%rcx, 24(%rax)	/* max_log_level */


#if CONFIG_LIBUKRELOC
	/* Save arguments before calling do_uk_reloc */
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx

	/* Perform relocations if PIE is enabled */
	xorl	%esi, %esi
	xorl	%edi, %edi
	call	do_uk_reloc


	/* Restore arguments */
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
#endif /* CONFIG_LIBUKRELOC */

	/* Setup boot CPU startup args */
	leaq	lcpu_boot_startup_args(%rip), %r8
	leaq	hyperlight_entry(%rip), %rax
	movq	%rax, LCPU_SARGS_ENTRY_OFFSET(%r8)
	leaq	lcpu_bootstack(%rip), %rax
	movq	%rax, LCPU_SARGS_STACKP_OFFSET(%r8)

	/* Pass startup args pointer to lcpu_start64 */
	movq	%r8, %rdi


	/* Jump to common CPU startup code
	 * Arguments in:
	 *   RDI: pointer to lcpu_sargs (boot_startup_args)
	 *   RSI: pointer to boot params (hyperlight_entry_args)
	 */

	leaq	hyperlight_entry_args(%rip), %rsi
	jmp	lcpu_start64
END(_hyperlight_entry)

/**
 * Assembly-level page fault handler for early-boot CoW resolution.
 *
 * Called via the minimal IDT before the proper Unikraft trap handlers
 * are installed.  Handles Copy-on-Write faults by:
 *   1. Walking the 4-level page table to find the faulting PTE
 *   2. Checking that PTE bit 9 (AVL_COW) is set
 *   3. Allocating a fresh page from the Hyperlight scratch bump allocator
 *   4. Copying the page content from the old read-only mapping
 *   5. Updating the PTE with the new physical address + RW
 *   6. Flushing the TLB entry
 *
 * Non-CoW faults halt the CPU (triple-fault equivalent).
 *
 * Stack on entry (pushed by CPU):
 *   [error_code]  <- RSP
 *   [RIP] [CS] [RFLAGS] [RSP_old] [SS]
 */
.global _cow_asm_pf_handler
_cow_asm_pf_handler:
	/* Save all GP registers we will use */
	pushq	%rax
	pushq	%rcx
	pushq	%rdx
	pushq	%rbx
	pushq	%rsi
	pushq	%rdi
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11

	/*
	 * Stack layout (10 pushes = 80 bytes):
	 *   RSP +  0: r11   RSP + 8: r10  RSP +16: r9   RSP +24: r8
	 *   RSP +32: rdi   RSP +40: rsi  RSP +48: rbx  RSP +56: rdx
	 *   RSP +64: rcx   RSP +72: rax
	 *   RSP +80: error_code (CPU)
	 *   RSP +88: RIP   RSP +96: CS   RSP+104: RFLAGS
	 *   RSP+112: RSP_old  RSP+120: SS
	 */

	/* Fault address from CR2 (page-align it) */
	movq	%cr2, %r11
	andq	$~0xFFF, %r11

	/* Error code sanity checks */
	movq	80(%rsp), %rax
	testq	$1, %rax		/* present? */
	jz	.Lpf_unhandled
	testq	$2, %rax		/* write? */
	jz	.Lpf_unhandled
	testq	$4, %rax		/* user mode? */
	jnz	.Lpf_unhandled
	testq	$8, %rax		/* reserved bit? */
	jnz	.Lpf_unhandled
	testq	$16, %rax		/* instruction fetch? */
	jnz	.Lpf_unhandled

	/* ---- Compute scratch memory bases ----------------------- */
	movabs	$SCRATCH_SIZE_GVA, %rax
	movq	(%rax), %r8		/* scratch_size */
	testq	%r8, %r8
	jz	.Lpf_unhandled

	movabs	$HL_MAX_GPA, %r9
	subq	%r8, %r9
	incq	%r9			/* r9  = scratch_base_gpa */

	movabs	$HL_MAX_GVA, %r10
	subq	%r8, %r10
	incq	%r10			/* r10 = scratch_base_gva */

	/* ---- Walk 4-level page table for r11 -------------------- */
	movq	%cr3, %rax
	andq	$~0xFFF, %rax		/* PML4 base (phys) */

	/* Canonical 48-bit VA for index computation */
	movq	%r11, %rcx
	movabs	$0x0000FFFFFFFFFFFF, %rdx
	andq	%rdx, %rcx

	/* PML4 */
	movq	%rcx, %rdx
	shrq	$39, %rdx
	andq	$0x1FF, %rdx
	leaq	(%rax, %rdx, 8), %rax
	movq	%rax, %rdi
	subq	%r9, %rdi
	addq	%r10, %rdi
	movq	(%rdi), %rax
	testq	$1, %rax
	jz	.Lpf_unhandled
	movabs	$PTE_ADDR_MASK, %rdi
	andq	%rdi, %rax

	/* PDPT */
	movq	%rcx, %rdx
	shrq	$30, %rdx
	andq	$0x1FF, %rdx
	leaq	(%rax, %rdx, 8), %rax
	movq	%rax, %rdi
	subq	%r9, %rdi
	addq	%r10, %rdi
	movq	(%rdi), %rax
	testq	$1, %rax
	jz	.Lpf_unhandled
	movabs	$PTE_ADDR_MASK, %rdi
	andq	%rdi, %rax

	/* PD */
	movq	%rcx, %rdx
	shrq	$21, %rdx
	andq	$0x1FF, %rdx
	leaq	(%rax, %rdx, 8), %rax
	movq	%rax, %rdi
	subq	%r9, %rdi
	addq	%r10, %rdi
	movq	(%rdi), %rax
	testq	$1, %rax
	jz	.Lpf_unhandled
	movabs	$PTE_ADDR_MASK, %rdi
	andq	%rdi, %rax

	/* PT (leaf) */
	movq	%rcx, %rdx
	shrq	$12, %rdx
	andq	$0x1FF, %rdx
	leaq	(%rax, %rdx, 8), %rax
	movq	%rax, %rdi
	subq	%r9, %rdi
	addq	%r10, %rdi		/* rdi = PTE virt addr */
	movq	(%rdi), %rax		/* rax = PTE value */

	/* Is this a CoW page? */
	testq	$PTE_COW_BIT, %rax
	jz	.Lpf_unhandled

	/* ---- Resolve the CoW fault ------------------------------ */
	/*
	 * Save PTE location and old value in callee-saved registers
	 * that we already pushed onto the stack at handler entry.
	 */
	movq	%rdi, %rbx		/* PTE virt addr */
	movq	%rax, %rsi		/* old PTE value */

	/* Bump-allocate one page from scratch */
	movabs	$ALLOCATOR_GVA, %rdi
	movq	$PAGE_SIZE_4K, %rax
	lock xaddq %rax, (%rdi)	/* rax = new page GPA */
	movq	%rax, %rdx		/* save GPA for PTE */

	/* Convert GPA -> GVA via scratch mapping */
	subq	%r9, %rax
	addq	%r10, %rax		/* rax = new page GVA */

	/* Copy 4 KiB: dst = new page, src = old mapping of r11 */
	movq	%rax, %rdi		/* dst */
	pushq	%rsi			/* save old PTE (rsi needed for src) */
	movq	%r11, %rsi		/* src (page-aligned fault addr) */
	movq	$512, %rcx		/* 4096 / 8 */
	cld
	rep movsq
	popq	%rsi			/* restore old PTE */

	/* Build new PTE: keep flags, replace address, set RW, clear CoW */
	movabs	$(PTE_ADDR_MASK | PTE_COW_BIT), %rcx
	notq	%rcx
	andq	%rcx, %rsi		/* flags only */
	orq	%rdx, %rsi		/* + new physical address */
	orq	$2, %rsi		/* + PTE_RW */

	/* Write the updated PTE */
	movq	%rsi, (%rbx)

	/* Flush stale TLB entry */
	invlpg	(%r11)

	/* ---- Return from interrupt ------------------------------ */
.Lpf_return:
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdi
	popq	%rsi
	popq	%rbx
	popq	%rdx
	popq	%rcx
	popq	%rax
	addq	$8, %rsp		/* discard error code */
	iretq

.Lpf_unhandled:
	/* Debug: print CR2 (faulting address) and error code before halt */
	pushq	%rax
	pushq	%rdx

	/* Print "PF:" prefix */
	movw	$103, %dx
	movb	$'P', %al
	outb	%al, %dx
	movb	$'F', %al
	outb	%al, %dx
	movb	$':', %al
	outb	%al, %dx

	/* Print CR2 in hex (16 nibbles) */
	movq	%cr2, %rax
	movq	$60, %rcx
.Lpf_cr2_loop:
	movq	%rax, %r8
	shrq	%cl, %r8
	andq	$0xf, %r8
	cmpq	$10, %r8
	jb	.Lpf_cr2_dig
	addq	$('a' - 10), %r8
	jmp	.Lpf_cr2_out
.Lpf_cr2_dig:
	addq	$'0', %r8
.Lpf_cr2_out:
	movq	%r8, %rax
	outb	%al, %dx
	movq	%cr2, %rax       /* reload CR2 (clobbered RAX) */
	subq	$4, %rcx
	jge	.Lpf_cr2_loop

	/* Print " E:" and error code (from stack) */
	movb	$' ', %al
	outb	%al, %dx
	movb	$'E', %al
	outb	%al, %dx
	movb	$':', %al
	outb	%al, %dx

	/* Error code is at RSP+16 (2 pushes) + 80 (10 saved regs) = RSP+96
	 * Actually we pushed 2 more regs, so error code is at RSP+16+80 = RSP+96 */
	movq	96(%rsp), %rax
	movq	$28, %rcx
.Lpf_err_loop:
	movq	%rax, %r8
	shrq	%cl, %r8
	andq	$0xf, %r8
	cmpq	$10, %r8
	jb	.Lpf_err_dig
	addq	$('a' - 10), %r8
	jmp	.Lpf_err_out
.Lpf_err_dig:
	addq	$'0', %r8
.Lpf_err_out:
	movq	%r8, %rax
	outb	%al, %dx
	movq	96(%rsp), %rax   /* reload error code */
	subq	$4, %rcx
	jge	.Lpf_err_loop

	/* Print " R:" and faulting RIP
	 * RIP is at RSP + 16 (our pushes) + 80 (saved regs) + 8 (error) = RSP+104
	 */
	movb	$' ', %al
	outb	%al, %dx
	movb	$'R', %al
	outb	%al, %dx
	movb	$':', %al
	outb	%al, %dx

	movq	104(%rsp), %rax
	movq	$60, %rcx
.Lpf_rip_loop:
	movq	%rax, %r8
	shrq	%cl, %r8
	andq	$0xf, %r8
	cmpq	$10, %r8
	jb	.Lpf_rip_dig
	addq	$('a' - 10), %r8
	jmp	.Lpf_rip_out
.Lpf_rip_dig:
	addq	$'0', %r8
.Lpf_rip_out:
	movq	%r8, %rax
	outb	%al, %dx
	movq	104(%rsp), %rax
	subq	$4, %rcx
	jge	.Lpf_rip_loop

	movb	$'\n', %al
	outb	%al, %dx

	popq	%rdx
	popq	%rax

	/* Halt via port 108 */
	movw	$108, %dx
	xorl	%eax, %eax
	outl	%eax, %dx
1:	pause
	jmp	1b
