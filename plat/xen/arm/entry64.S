/* SPDX-License-Identifier: BSD-2-Clause */
/*
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
 */
/* Taken from Mini-OS */

#include <uk/arch/lcpu.h>
#include <uk/asm.h>
#include <xen/xen.h>
#include <xen/elfnote.h>

#define ELFNOTE(name, type, desc)           \
	.pushsection .note.name               ; \
	.align 4                              ; \
	.long 2f - 1f       /* namesz */      ; \
	.long 4f - 3f       /* descsz */      ; \
	.long type          /* type   */      ; \
1:.asciz #name          /* name   */      ; \
2:.align 4                                ; \
3:desc                  /* desc   */      ; \
4:.align 4                                ; \
	.popsection

ELFNOTE(Xen, XEN_ELFNOTE_GUEST_OS, .asciz "Unikraft")
ELFNOTE(Xen, XEN_ELFNOTE_LOADER, .asciz "generic")
ELFNOTE(Xen, XEN_ELFNOTE_XEN_VERSION, .asciz "xen-3.0")

#include <uk/asm.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/limits.h>
#include <xen-arm/mm.h>

lr      .req    x30             // link register

#define MAIR(attr, mt)  ((attr) << ((mt) * 8))
#define TCR_FLAGS       (TCR_IRGN_WBWA | TCR_ORGN_WBWA | TCR_SHARED)
#define PT_PT           0x3
#define PT_MEM          0x711
/* Number of virtual address bits for 4KB page */
#define VA_BITS         39

#define PRINT(_s)           \
	mov     x17, x0;        \
	mov     x18, x1;        \
	mov     x19, x2;        \
	ldr     x2, =97f;       \
	ldr     x1, =98f;       \
	sub     x1, x1, x2;     \
	sub     x2, x2, x22;    \
	mov     x0, xzr;        \
	mov     x16, #0x12;     \
	hvc     #0xEA1;         \
	b       99f;            \
97: .asciz _s;              \
98: ;                       \
	.align  2;              \
99: mov     x0, x17;        \
	mov     x1, x18;        \
	mov     x2, x19;        \

#define VPRINT(_s)           \
	mov     x17, x0;        \
	mov     x18, x1;        \
	mov     x19, x2;        \
	ldr     x2, =97f;       \
	ldr     x1, =98f;       \
	sub     x1, x1, x2;     \
	mov     x0, xzr;        \
	mov     x16, #0x12;     \
	hvc     #0xEA1;         \
	b       99f;            \
97: .asciz _s;              \
98: ;                       \
	.align  2;              \
99: mov     x0, x17;        \
	mov     x1, x18;        \
	mov     x2, x19;        \

	.data
	.globl shared_info_page, IRQ_handler, stack
	.globl boot_l1_pgtable, boot_l2_pgtable, fixmap_pgtable
	.globl idmap_pgtable

	.align 12 // 4KiB aligned
boot_l1_pgtable:
	.space PAGE_SIZE
boot_l2_pgtable:
	.space PAGE_SIZE
fixmap_pgtable:
	.space PAGE_SIZE
idmap_pgtable:
	.space PAGE_SIZE


shared_info_page:
	.space PAGE_SIZE
IRQ_handler:
	.long 0x0

	.align 12
stack:
	.space __STACK_SIZE
stack_end:

#if PAGE_SIZE == 4096
#define HEADER_PGSIZE	1
#elif PAGE_SIZE == 16384
#define HEADER_PGSIZE	2
#elif PAGE_SIZE == 65536
#define HEADER_PGSIZE	3
#else
#define HEADER_PGSIZE 0
#warning "Can't determine page size for header flags"
#endif

#define HEADER_FLAGS	(HEADER_PGSIZE << 1)

.section .text.zimageboot

ENTRY(_libxenplat_zimageboot)
/*
 * Kernel startup entry point.
 * ---------------------------
 *
 * The kernel image has to contains a 64-bytes header as follows:
 *
 *   u32 code0;                     // Executable code
 *   u32 code1;                     // Executable code
 *   u64 text_offset;               // Image load offset
 *   u64 res0 = 0;                  // reserved
 *   u64 res1 = 0;                  // reserved
 *   u64 res2 = 0;                  // reserved
 *   u64 res3 = 0;                  // reserved
 *   u64 res4 = 0;                  // reserved
 *   u32 magic = 0x644d5241         // Magic number, little endian, "ARM\x64"
 *   u32 res5 = 0;                  // reserved
 */

	b       _libxenplat_start // branch to kernel start, magic
	.long   0                       // reserved
	.quad   0x0                     // Image load offset from start of RAM
	.quad   _edata - _libxenplat_start // Effective size of kernel
	                                   // image, little-endian
	.quad   HEADER_FLAGS            // Informative flags, little-endian
	.quad   0                       // reserved
	.quad   0                       // reserved
	.quad   0                       // reserved
	.byte   0x41                    // Magic number, "ARM\x64"
	.byte   0x52
	.byte   0x4d
	.byte   0x64
	.long   0                       // reserved

ENDPROC(_libxenplat_zimageboot)
/*
 * Primary CPU general-purpose register settings
 * x0 = physical address of device tree blob (dtb) in system RAM.
 * x1 = 0 (reserved for future use)
 * x2 = 0 (reserved for future use)
 * x3 = 0 (reserved for future use)
 */

ENTRY(_libxenplat_start)
	/* Disable all interrupts */
	msr     daifset, #2

	/* Save the FDT pointer */
	mov     x20, x0

	/*
	 * Disable the MMU. We may have entered the kernel with it on and
	 * will need to update the tables later. If this has been set up
	 * with anything other than a VA == PA map then this will fail,
	 * but in this case the code to find where we are running from
	 * would have also failed.
	 */
	dsb     sy
	mrs     x2, sctlr_el1
	bic     x2, x2, #0x1
	msr     sctlr_el1, x2
	isb

	/* Calculate where we are */
	bl      _calc_offset

	PRINT("- Unikraft booting -\n")

	PRINT("- Setup CPU -\n")
	/* Setup CPU for turnning the MMU on. */
	bl      _libxenplat_setup_cpu

	PRINT("- Setup booting pagetable -\n")
	/* Setup the initial page table. */
	bl      _libxenplat_setup_initial_pgtable

	/*
	 * Setup the identity mapping
	 */
	bl      _libxenplat_setup_idmap_pgtable

	/* Load TTBRx */
	msr     ttbr1_el1, x4
	msr     ttbr0_el1, x5
	isb

	/* Load the exception vectors */
	ldr     x2, =vector_table
	msr     vbar_el1, x2

	/* Turning on MMU */
	dsb     sy
	/*
	 * Prepare system control register (SCTRL)
	 *
	 *   0011 0xxx 1101 xx0x xx0x 10xx xxxx xxxx (reserved bits)
	 *   xxxx x100 xxxx 01x1 11x1 xx01 0000 0101 (bits to set)
	 *   ---------------------------------------
	 *   0011 0100 1101 0101 1101 1001 0000 0101 (all together)
	 *
	 *   UCI     [26] Enables EL0 access in AArch64 for DC CVAU, DC CIVAC,
					  DC CVAC and IC IVAU instructions
	 *   EE      [25] Explicit data accesses at EL1 and Stage 1 translation
	                  table walks at EL1 & EL0 are little-endian
	 *   EOE     [24] Explicit data accesses at EL0 are little-endian
	 *   WXN     [19] Regions with write permission are not forced to XN
	 *   nTWE    [18] WFE instructions are executed as normal
	 *   nTWI    [16] WFI instructions are executed as normal
	 *   UCT     [15] Enables EL0 access in AArch64 to the CTR_EL0 register
	 *   DZE     [14] Execution of the DC ZVA instruction is allowed at EL0
	 *   I       [12] Instruction caches enabled at EL0 and EL1
	 *   UMA     [9]  Disable access to the interrupt masks from EL0
	 *   SED     [8]  The SETEND instruction is available
	 *   ITD     [7]  The IT instruction functionality is available
	 *   THEE    [6]  ThumbEE is disabled
	 *   CP15BEN [5]  CP15 barrier operations disabled
	 *   SA0     [4]  Stack Alignment check for EL0 enabled
	 *   SA      [3]  Stack Alignment check enabled
	 *   C       [2]  Data and unified enabled
	 *   A       [1]  Alignment fault checking disabled
	 *   M       [0]  MMU disabled
	 */
	ldr     x1, =0x34d5d905
	msr     sctlr_el1, x1
	isb

	VPRINT("- MMU on -\n")
	ldr     x0, =mmu_on
	br      x0

mmu_on:
	/* Setup stack */
	VPRINT("- Setup stack -\n")
	ldr     x0, =stack
	ldr     x1, =stack_end
	mov     sp, x1

	/* Clear stack */
1:  stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	cmp     x0, x1
	b.lo    1b

	VPRINT("- Jumping to C entry -\n")
	mov     x0, x20                  // x0 <- device tree (physical address)
	mov     x1, x22                  // x1 <- phys_offset

	b       _libxenplat_armentry
ENDPROC(_libxenplat_start)

.section .text

ENTRY(_calc_offset)
	ldr     x22, =_libxenplat_start  // x0 := vaddr(_libxenplat_start)
	adr     x21, _libxenplat_start   // x21 := paddr(_libxenplat_start)
	sub     x22, x22, x21            // x22 := phys-offset (vaddr - paddr)
	ret
ENDPROC(_calc_offset)

ENTRY(_libxenplat_setup_cpu)
	ic      iallu
	tlbi    vmalle1is
	dsb     ish

	/*
	 * Setup memory attribute type tables
	 *
	 * Memory regioin attributes for LPAE:
	 *
	 *   n = AttrIndx[2:0]
	 *                           MAIR
	 *   DEVICE_nGnRnE           00000000 (0x00)
	 *   DEVICE_nGnRE            00000100 (0x04)
	 *   DEVICE_GRE              00001100 (0x0c)
	 *   NORMAL_NC               01000100 (0x44)
	 *   NORMAL_WB               11111111 (0xff)
	 *   NORMAL_WT               10111011 (0xbb)
	 */
	ldr     x0, =(MAIR_DEVICE_nGnRnE | \
	             MAIR_DEVICE_nGnRE | \
	             MAIR_DEVICE_GRE | \
	             MAIR_NORMAL_NC | \
	             MAIR_NORMAL_WB | \
	             MAIR_NORMAL_WT)
	msr     mair_el1, x0

	/*
	 * Setup translation control register (TCR)
	 */
	ldr     x0, =(TCR_TxSZ(VA_BITS) | TCR_ASID16 | TCR_TG1_4K \
	            | TCR_FLAGS )
	mrs     x1, ID_AA64MMFR0_EL1
	bfi     x0, x1, #32, #3
	msr     tcr_el1, x0

	/*
	 * Enable FP/ASIMD
	 */
	mov     x0, #3 << 20
	msr     cpacr_el1, x0

	/*
	 * Reset mdscr_el1
	 */
	msr     mdscr_el1, xzr

	ret
ENDPROC(_libxenplat_setup_cpu)

ENTRY(_libxenplat_setup_initial_pgtable)
	ldr     x4, =boot_l1_pgtable
	ldr     x5, =boot_l2_pgtable

	sub     x4, x4, x22             // x4 := paddr (boot_l1_pgtable)
	sub     x5, x5, x22             // x5 := paddr (boot_l2_pgtable)


	/* Clear level-1 boot page table */
	mov     x0, x4
	add     x1, x0, #PAGE_SIZE
1:  stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	cmp     x0, x1
	b.lo    1b

	/* Clear level-2 boot page table */
	mov     x0, x5
	add     x1, x0, #PAGE_SIZE
1:  stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	cmp     x0, x1
	b.lo    1b

	/* Find the size of the kernel */
	ldr     x0, =_text                  // x0 := vaddr(_text)
	ldr     x1, =_end                   // x1 := vaddr(_end)
	sub     x2, x1, x0
	/* Get the number of l2 pages to allocate, rounded down */
	lsr     x2, x2, #L2_SHIFT
	/* Add 4 MiB for any rounding above and the module data */
	add     x2, x2, #2                  // x2 := total number of entries

	/* Find the table index */
	lsr     x3, x0, #L2_SHIFT           // L2_SHIFT = 21
	and     x3, x3, #Ln_ADDR_MASK       // x3 := index of l2 table


	/* Build the L2 block entries */
	sub     x6, x0, x22                 // x6 := paddr(_text)
	lsr     x6, x6, #L2_SHIFT           // L2_SHIFT = 21
	mov     x7, #PT_MEM
1:  orr     x7, x7, x6, lsl #L2_SHIFT   // x7 := l2 pgtbl entry content

	/* Store the entry */
	str     x7, [x5, x3, lsl #3]

	/* Clear the address bits */
	and     x7, x7, #ATTR_MASK_L

	sub     x2, x2, #1
	add     x3, x3, #1
	add     x6, x6, #1
	cbnz    x2, 1b

	/* Link the l1 -> l2 table */
	/* Find the table index */
	lsr     x3, x0, #L1_SHIFT           // L1_SHIFT = 30
	and     x3, x3, #Ln_ADDR_MASK       // x3 := index of l1 table


	/* Build the L1 page table entry */
	ldr     x7, =PT_PT
	lsr     x9, x5, #12
	orr     x7, x7, x9, lsl #12

	/* Store the entry */
	str     x7, [x4, x3, lsl #3]

	ret
ENDPROC(_libxenplat_setup_initial_pgtable)

ENTRY(_libxenplat_setup_idmap_pgtable)
	ldr     x5, =idmap_pgtable
	sub     x5, x5, x22             // x5 := paddr(idmap_pgtable)

	/* Clear identity mapping page table */
	mov     x0, x5
	add     x1, x0, #PAGE_SIZE
1:  stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	stp     xzr, xzr, [x0], #16
	cmp     x0, x1
	b.lo    1b

	/* Create the VA = PA map */
	ldr     x6, =_text                  // x0 := vaddr(_text)
	sub     x6, x6, x22                 // x0 := paddr(_text)

	/* Find the table index */
	lsr     x0, x6, #L1_SHIFT
	and     x0, x0, #Ln_ADDR_MASK       // x0 := index of l1 table

	/* Build the L1 block entry */
	ldr     x1, =PT_MEM
	lsr     x2, x6, #L1_SHIFT
	orr     x1, x1, x2, lsl #L1_SHIFT

	/* Store the entry */
	str     x1, [x5, x0, lsl #3]

	ret
ENDPROC(_libxenplat_setup_idmap_pgtable)

/*
 * There are no PUSH/POP instruction in ARMv8.
 * Use STR and LDR for stack accesses.
 */
	.macro push, xreg
	str     \xreg, [sp, #-8]!
	.endm

	.macro pop, xreg
	ldr     \xreg, [sp], #8
	.endm

	.macro trap_entry, el
	sub     sp, sp, #32             // room for LR, SP, SPSR, ELR
	push    x29
	push    x28
	push    x27
	push    x26
	push    x25
	push    x24
	push    x23
	push    x22
	push    x21
	push    x20
	push    x19
	push    x18
	push    x17
	push    x16
	push    x15
	push    x14
	push    x13
	push    x12
	push    x11
	push    x10
	push    x9
	push    x8
	push    x7
	push    x6
	push    x5
	push    x4
	push    x3
	push    x2
	push    x1
	push    x0
	.if     \el == 0
	mrs     x21, sp_el0
	.else
	add     x21, sp, #272
	.endif
	mrs     x22, elr_el1
	mrs     x23, spsr_el1
	stp     x30, x21, [sp, #240]
	stp     x22, x23, [sp, #256]
	.endm

	.macro trap_exit, el
	ldp     x21, x22, [sp, #256]    // load ELR, SPSR
	.if     \el == 0
	ldr     x23, [sp, #248]         // load return stack pointer
	.endif
	pop     x0
	pop     x1
	pop     x2
	pop     x3
	pop     x4
	pop     x5
	pop     x6
	pop     x7
	pop     x8
	pop     x9
	msr     elr_el1, x21            // set up the return data
	msr     spsr_el1, x22
	.if     \el == 0
	msr     sp_el0, x23
	.endif
	pop     x10
	pop     x11
	pop     x12
	pop     x13
	pop     x14
	pop     x15
	pop     x16
	pop     x17
	pop     x18
	pop     x19
	pop     x20
	pop     x21
	pop     x22
	pop     x23
	pop     x24
	pop     x25
	pop     x26
	pop     x27
	pop     x28
	pop     x29
	ldr     x30, [sp], #32
	.endm

/*
 * Exception vector entry
 */
	.macro ventry label
	.align  7
	b       \label
	.endm

	.align  11
ENTRY(vector_table)
	/* Current EL with SP0 */
	ventry el1_sync_invalid         // Synchronous EL1t
	ventry el1_irq_invalid          // IRQ EL1t
	ventry el1_fiq_invalid          // FIQ EL1t
	ventry el1_error_invalid        // Error EL1t

	/* Current EL with SPx */
	ventry el1_sync                 // Synchronous EL1h
	ventry el1_irq                  // IRQ EL1h    ventry el1_fiq_invalid
	                                // FIQ EL1h
	ventry el1_error_invalid        // Error EL1h

	/* Lower EL using AArch64 */
	ventry el0_sync                 // Synchronous 64-bit EL0
	ventry el0_irq                  // IRQ 64-bit EL0
	ventry el0_fiq_invalid          // FIQ 64-bit EL0
	ventry el0_error_invalid        // Error 64-bit EL0

	/* Lower EL using AArch32 */
	ventry el0_sync_invalid         // Synchronous 32-bit EL0
	ventry el0_irq_invalid          // IRQ 32-bit EL0
	ventry el0_fiq_invalid          // FIQ 32-bit EL0
	ventry el0_error_invalid        // Error 32-bit EL0
END(vector_table)

/*
 * SYNC & IRQ exception handler.
 */
	.align 6
el1_sync:
	trap_entry 1
	mov     x0, sp
	bl      trap_el1_sync
	trap_exit 1
	eret
ENDPROC(el1_sync)

	.align 6
el1_irq:
	trap_entry 1
	mov     x0, sp
	bl      trap_el1_irq
	trap_exit 1
	eret
ENDPROC(el1_irq)

	.align 6
el0_sync:
	trap_entry 0
	mov     x0, sp
	bl      trap_el1_sync
	trap_exit 0
	eret
ENDPROC(el0_sync)

	.align 6
el0_irq:
	trap_entry 0
	mov     x0, sp
	bl      trap_el1_irq
	trap_exit 0
	eret
ENDPROC(el0_irq)

/*
 * Bad Abort numbers
 */
#define BAD_SYNC    0
#define BAD_IRQ     1
#define BAD_FIQ     2
#define BAD_ERROR   3

	.macro invalid, reason
	mov     x0, sp
	mov     x1, #\reason
	b       invalid_trap_handler
	.endm

el0_sync_invalid:
	invalid BAD_SYNC
ENDPROC(el0_sync_invalid)

el0_irq_invalid:
	invalid BAD_IRQ
ENDPROC(el0_irq_invalid)

el0_fiq_invalid:
	invalid BAD_FIQ
ENDPROC(el0_fiq_invalid)

el0_error_invalid:
	invalid BAD_ERROR
ENDPROC(el0_error_invalid)

el1_sync_invalid:
	invalid BAD_SYNC
ENDPROC(el1_sync_invalid)

el1_irq_invalid:
	invalid BAD_IRQ
ENDPROC(el1_irq_invalid)

el1_fiq_invalid:
	invalid BAD_FIQ
ENDPROC(el1_fiq_invalid)

el1_error_invalid:
	invalid BAD_ERROR
ENDPROC(el1_error_invalid)

ENTRY(thread_starter)
	ret
ENDPROC(thread_starter)

ENTRY(__arch_switch_threads)
	// r0 - prev sp
	// r1 - next sp
	// actually we are not obligued to save/restore x9
	stp     x29, x9, [sp,#-16]!
	stp     x27, x28, [sp,#-16]!
	stp     x25, x26, [sp,#-16]!
	stp     x23, x24, [sp,#-16]!
	stp     x21, x22, [sp,#-16]!
	stp     x19, x20, [sp,#-16]!  // store callee-saved registers
	mov     x9, sp
	stp     x9, lr, [x0]

	ldp      x9, lr, [x1]
	mov      sp, x9
	ldp     x19, x20, [sp], #16  // restore callee-saved registers
	ldp     x21, x22, [sp], #16
	ldp     x23, x24, [sp], #16
	ldp     x25, x26, [sp], #16
	ldp     x27, x28, [sp], #16
	ldp     x29, x9, [sp], #16
	ret
ENDPROC(__arch_switch_threads)

ENTRY(arm_start_thread)
	ldp x0, x1, [sp], #16	// parameter in x0, entry point in x1
	ldr lr, =  uk_sched_thread_exit // exit_thread
	br x1
ENDPROC(arm_start_thread)

/*
* We do not want to unmap anything.
*/
.section .bss
.align	4
.globl	bpt_unmap_mrd
bpt_unmap_mrd:
.space	64
