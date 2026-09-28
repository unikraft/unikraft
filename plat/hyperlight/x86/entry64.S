/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/asm.h>
#include <uk/lcpu.h>

/* Boot startup args struct offsets */
#define LCPU_SARGS_ENTRY_OFFSET		0x00
#define LCPU_SARGS_STACKP_OFFSET	0x08

/* Hyperlight scratch region metadata (from hyperlight/mem.h) */
#define SCRATCH_SIZE_GVA	0xFFFFFFFFFFFFEFF8
#define ALLOCATOR_GVA		0xFFFFFFFFFFFFEFF0
#define HL_MAX_GPA		0x0000000FFFFFFFFF
#define HL_MAX_GVA		0xFFFFFFFFFFFFEFFF
#define PTE_ADDR_MASK		0x000FFFFFFFFFF000
#define PTE_COW_BIT		(1 << 9)
#define PAGE_SIZE_4K		4096

/*
 * Boot stack — 4 KiB in .bss (CoW pages, must be pre-faulted
 * before use).
 */
.section .bss
.space 4096
lcpu_bootstack:

/* Boot startup args (entry function + stack pointer) */
lcpu_boot_startup_args:
	.quad	0
	.quad	0

/*
 * Hyperlight entry arguments — saved to .data so that C code can
 * access them.  Writing to .data triggers CoW faults, so the CoW
 * handler must be installed before we reach this point.
 */
.section .data
.globl hyperlight_entry_args
hyperlight_entry_args:
	.quad	0	/* peb_address (from RDI) */
	.quad	0	/* seed        (from RSI) */
	.quad	0	/* page_size   (from RDX) */
	.quad	0	/* max_log_level (from RCX) */

/*
 * _hyperlight_entry — 64-bit entry point for the Hyperlight platform.
 *
 * Hyperlight starts the guest in 64-bit long mode with:
 *   RDI = PEB address    (shared memory layout)
 *   RSI = random seed
 *   RDX = page size
 *   RCX = max log level
 *   RSP = scratch stack   (always writable)
 *   CR3 = host-provided page tables (identity-mapped)
 *   Interrupts disabled
 *
 * Writable guest pages (.data, .bss) are marked Copy-on-Write by
 * the host: PTE write bit cleared, AVL bit 9 set.  Any write to
 * these pages faults.  We must install a page fault handler BEFORE
 * touching any writable memory outside the scratch region.
 *
 * The scratch stack (RSP on entry) is always writable — we use it
 * to build the GDT and IDT.
 */
.code64
.section .text.boot
ENTRY(_hyperlight_entry)

	/* ---- Save entry args in callee-saved registers --------- */
	movq	%rdi, %rbx	/* peb_address */
	movq	%rsi, %r15	/* seed */
	movq	%rdx, %r14	/* page_size */
	movq	%rcx, %r13	/* max_log_level */

	/* ---- Check if CoW is active ----------------------------- */
	movabs	$SCRATCH_SIZE_GVA, %rax
	movq	(%rax), %r8
	testq	%r8, %r8
	jz	.Lcow_setup_done

	/*
	 * ---- Minimal GDT on the scratch stack -------------------
	 *
	 * Hyperlight starts the guest with no GDT.  We need one so
	 * that IDT entries can reference a valid code segment.
	 *
	 * Layout (3 entries = 24 bytes):
	 *   [0] 0x00: null descriptor
	 *   [1] 0x08: 64-bit code segment (selector 0x08)
	 *   [2] 0x10: 64-bit data segment (selector 0x10)
	 *
	 * We also reserve 240 bytes for 15 IDT entries (only #14
	 * will be populated).  Total: 24 + 240 = 264, padded to
	 * 272 for 16-byte stack alignment.
	 */
	subq	$272, %rsp

	/* GDT starts at RSP+240 */
	leaq	240(%rsp), %rax
	movq	$0, (%rax)			/* [0] null */
	movabs	$0x00af9b000000ffff, %rcx	/* [1] code64: L=1, D=0, P=1, DPL=0, type=0xb */
	movq	%rcx, 8(%rax)
	movabs	$0x00cf93000000ffff, %rcx	/* [2] data64: P=1, DPL=0, type=0x3 */
	movq	%rcx, 16(%rax)

	/* Load GDTR */
	subq	$16, %rsp
	movw	$23, (%rsp)			/* limit = 24 - 1 */
	leaq	(16 + 240)(%rsp), %rax
	movq	%rax, 2(%rsp)
	lgdt	(%rsp)
	addq	$16, %rsp

	/* Reload CS via far return */
	pushq	$0x08
	leaq	.Lgdt_loaded(%rip), %rax
	pushq	%rax
	lretq
.Lgdt_loaded:
	movl	$0x10, %eax
	movl	%eax, %ss
	movl	%eax, %ds
	movl	%eax, %es
	xorl	%eax, %eax
	movl	%eax, %fs
	movl	%eax, %gs

	/*
	 * ---- Minimal IDT with #PF handler -----------------------
	 *
	 * Zero all 15 entries (RSP+0..RSP+239), then populate only
	 * entry #14 (page fault) at offset 14*16 = 224.
	 */
	movq	%rsp, %rdi
	xorq	%rax, %rax
	movq	$30, %rcx		/* 240 / 8 */
	cld
	rep stosq

	/* IDT entry #14: _cow_asm_pf_handler */
	leaq	_cow_asm_pf_handler(%rip), %rax
	movw	%ax, 224(%rsp)		/* offset[15:0] */
	movw	$0x08, 226(%rsp)	/* selector = code64 */
	movb	$0, 228(%rsp)		/* IST = 0 (use current stack) */
	movb	$0x8E, 229(%rsp)	/* P=1 DPL=0 type=0xE (interrupt gate) */
	shrq	$16, %rax
	movw	%ax, 230(%rsp)		/* offset[31:16] */
	shrq	$16, %rax
	movl	%eax, 232(%rsp)		/* offset[63:32] */

	/* Load IDTR */
	subq	$16, %rsp
	movw	$239, (%rsp)		/* limit = 15*16 - 1 */
	leaq	16(%rsp), %rax
	movq	%rax, 2(%rsp)
	lidt	(%rsp)
	addq	$16, %rsp

	/*
	 * ---- Pre-fault the boot stack ----------------------------
	 *
	 * lcpu_start64 switches RSP to lcpu_bootstack (.bss).
	 * The stack is 4096 bytes and may span two CoW pages.
	 * Trigger CoW resolution for both pages now, while RSP is
	 * still on the scratch stack.
	 */
	leaq	lcpu_bootstack(%rip), %rax
	movq	$0, -8(%rax)		/* top of stack page */
	movq	$0, -0x1008(%rax)	/* next page down */

.Lcow_setup_done:

	/* ---- Restore entry args -------------------------------- */
	movq	%rbx, %rdi
	movq	%r15, %rsi
	movq	%r14, %rdx
	movq	%r13, %rcx

	/* Save to .data (triggers CoW faults — handler is ready) */
	leaq	hyperlight_entry_args(%rip), %rax
	movq	%rdi, 0(%rax)
	movq	%rsi, 8(%rax)
	movq	%rdx, 16(%rax)
	movq	%rcx, 24(%rax)

#if CONFIG_LIBUKRELOC
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	xorl	%esi, %esi
	xorl	%edi, %edi
	call	do_uk_reloc
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
#endif

	/* Set up boot CPU startup args */
	leaq	lcpu_boot_startup_args(%rip), %r8
	leaq	hyperlight_entry(%rip), %rax
	movq	%rax, LCPU_SARGS_ENTRY_OFFSET(%r8)
	leaq	lcpu_bootstack(%rip), %rax
	movq	%rax, LCPU_SARGS_STACKP_OFFSET(%r8)

	/* Jump to CPU bootstrap (FPU/SSE init, then C entry) */
	movq	%r8, %rdi
	leaq	hyperlight_entry_args(%rip), %rsi
	jmp	lcpu_start64

END(_hyperlight_entry)

/*
 * _cow_asm_pf_handler — early-boot page fault handler for CoW.
 *
 * Runs on the current stack (IST=0) before proper trap handlers are
 * installed.  Handles Copy-on-Write faults by:
 *
 *   1. Checking error code: must be a write to a present, supervisor,
 *      non-instruction-fetch page
 *   2. Walking the 4-level page table (from CR3) to find the leaf PTE
 *      — page tables are in the scratch region, accessed via the
 *      scratch GPA→GVA offset
 *   3. Checking PTE bit 9 (AVL_COW)
 *   4. Bump-allocating a fresh page from scratch (atomic lock xaddq)
 *   5. Copying the original page content to the new page
 *   6. Updating the PTE: new physical address + RW, clear CoW bit
 *   7. Flushing the TLB entry with invlpg
 *
 * Non-CoW faults print diagnostic info via DebugPrint (port 103)
 * and halt the guest.
 *
 * CPU pushes onto the stack before entry:
 *   [SS] [RSP_old] [RFLAGS] [CS] [RIP] [error_code] ← RSP
 */
.global _cow_asm_pf_handler
_cow_asm_pf_handler:
	/* Save registers */
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

	/* r11 = page-aligned faulting address */
	movq	%cr2, %r11
	andq	$~0xFFF, %r11

	/*
	 * Validate error code (at RSP+80 after 10 pushes):
	 *   bit 0 (P)    = 1  page was present
	 *   bit 1 (W)    = 1  access was a write
	 *   bit 2 (U)    = 0  supervisor mode
	 *   bit 3 (RSVD) = 0  no reserved-bit violation
	 *   bit 4 (I/D)  = 0  not an instruction fetch
	 */
	movq	80(%rsp), %rax
	testq	$1, %rax
	jz	.Lpf_unhandled
	testq	$2, %rax
	jz	.Lpf_unhandled
	testq	$4, %rax
	jnz	.Lpf_unhandled
	testq	$8, %rax
	jnz	.Lpf_unhandled
	testq	$16, %rax
	jnz	.Lpf_unhandled

	/* ---- Compute scratch GPA/GVA bases ---------------------- */
	movabs	$SCRATCH_SIZE_GVA, %rax
	movq	(%rax), %r8
	testq	%r8, %r8
	jz	.Lpf_unhandled

	movabs	$HL_MAX_GPA, %r9
	subq	%r8, %r9
	incq	%r9			/* r9  = scratch_base_gpa */

	movabs	$HL_MAX_GVA, %r10
	subq	%r8, %r10
	incq	%r10			/* r10 = scratch_base_gva */

	/*
	 * ---- Walk 4-level page table ----------------------------
	 *
	 * Page tables are in the scratch region (always writable).
	 * To read a PTE at physical address P, we compute:
	 *   virt = P - scratch_base_gpa + scratch_base_gva
	 *
	 * We extract the canonical 48-bit VA from the faulting
	 * address and use bits [47:39], [38:30], [29:21], [20:12]
	 * as indices into PML4, PDPT, PD, and PT respectively.
	 */
	movq	%cr3, %rax
	andq	$~0xFFF, %rax

	movq	%r11, %rcx
	movabs	$0x0000FFFFFFFFFFFF, %rdx
	andq	%rdx, %rcx		/* canonical 48-bit VA */

	/* PML4 [47:39] */
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

	/* PDPT [38:30] */
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

	/* PD [29:21] */
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

	/* PT [20:12] — leaf entry */
	movq	%rcx, %rdx
	shrq	$12, %rdx
	andq	$0x1FF, %rdx
	leaq	(%rax, %rdx, 8), %rax
	movq	%rax, %rdi
	subq	%r9, %rdi
	addq	%r10, %rdi		/* rdi = leaf PTE virtual addr */
	movq	(%rdi), %rax		/* rax = PTE value */

	/* Is this a CoW page? */
	testq	$PTE_COW_BIT, %rax
	jz	.Lpf_unhandled

	/* ---- Resolve CoW fault ---------------------------------- */
	movq	%rdi, %rbx		/* save PTE virtual addr */
	movq	%rax, %rsi		/* save old PTE value */

	/* Bump-allocate one page from scratch */
	movabs	$ALLOCATOR_GVA, %rdi
	movq	$PAGE_SIZE_4K, %rax
	lock xaddq %rax, (%rdi)		/* rax = new page GPA */
	movq	%rax, %rdx		/* save GPA for new PTE */

	/* Convert new page GPA → GVA for the copy */
	subq	%r9, %rax
	addq	%r10, %rax		/* rax = new page GVA */

	/* Copy 4 KiB from old mapping to new page */
	movq	%rax, %rdi		/* dst = new page */
	pushq	%rsi			/* save old PTE */
	movq	%r11, %rsi		/* src = faulting page */
	movq	$512, %rcx		/* 4096 / 8 = 512 qwords */
	cld
	rep movsq
	popq	%rsi			/* restore old PTE */

	/* Build new PTE: old flags + new address + RW - CoW */
	movabs	$(PTE_ADDR_MASK | PTE_COW_BIT), %rcx
	notq	%rcx
	andq	%rcx, %rsi		/* keep flags only */
	orq	%rdx, %rsi		/* new physical address */
	orq	$2, %rsi		/* set PTE_RW */

	/* Write updated PTE and flush TLB */
	movq	%rsi, (%rbx)
	invlpg	(%r11)

	/* ---- Return from fault ---------------------------------- */
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

	/*
	 * Unhandled fault — print CR2, error code, and RIP via
	 * DebugPrint (port 103) for diagnostics, then halt.
	 */
.Lpf_unhandled:
	pushq	%rax
	pushq	%rdx

	/* "PF:" prefix */
	movw	$103, %dx
	movb	$'P', %al
	outb	%al, %dx
	movb	$'F', %al
	outb	%al, %dx
	movb	$':', %al
	outb	%al, %dx

	/* Print CR2 in hex */
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
	movq	%cr2, %rax
	subq	$4, %rcx
	jge	.Lpf_cr2_loop

	/* " E:" + error code */
	movb	$' ', %al
	outb	%al, %dx
	movb	$'E', %al
	outb	%al, %dx
	movb	$':', %al
	outb	%al, %dx

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
	movq	96(%rsp), %rax
	subq	$4, %rcx
	jge	.Lpf_err_loop

	/* " R:" + faulting RIP */
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
