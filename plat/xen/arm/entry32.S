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

@ Offset of the kernel within the RAM. This is a Linux/zImage convention which we
@ rely on for now.
#define ZIMAGE_KERNEL_OFFSET 0x8000

.section .text.zimageboot

.globl _libxenplat_start
_libxenplat_start:
	@ zImage header
.rept   8
        mov     r0, r0
.endr
        b       reset
        .word   0x016f2818			@ Magic numbers to help the loader
        .word   0				@ zImage start address (0 = relocatable)
        .word   _edata - _libxenplat_start	@ zImage end address (excludes bss section)
	@ end of zImage header

@ Called at boot time. Sets up MMU, exception vectors and stack, and then calls C _libxenplat_armentry() function.
@ => r2 -> DTB
@ <= never returns
@ Note: this boot code needs to be within the first (1MB - ZIMAGE_KERNEL_OFFSET) of _libxenplat_start.
reset:
	@ Problem: the C code wants to be at a known address (_libxenplat_start), but Xen might
	@ load us anywhere. We initialise the MMU (mapping virtual to physical @ addresses)
	@ so everything ends up where the code expects it to be.
	@
	@ We calculate the offet between where the linker thought _libxenplat_start would be and where
	@ it actually is and initialise the page tables to have that offset for every page.
	@
	@ When we turn on the MMU, we're still executing at the old address. We don't want
	@ the code to disappear from under us. So we have to do the mapping in stages:
	@
	@ 1. set up a mapping to our current page from both its current and desired addresses
	@ 2. enable the MMU
	@ 3. jump to the new address
	@ 4. remap all the other pages with the calculated offset

	adr	r1, _libxenplat_start	@ r1 = physical address of _libxenplat_start
	ldr	r3, =_libxenplat_start	@ r3 = (desired) virtual address of _libxenplat_start
	sub 	r9, r1, r3		@ r9 = (physical - virtual) offset

	ldr	r7, =_page_dir		@ r7 = (desired) virtual addr of translation table
	add	r1, r7, r9		@ r1 = physical addr of translation table

	@ Tell the system where our page table is located.
	@ This is the 16 KB top-level translation table, in which
	@ each word maps one 1MB virtual section to a physical section.
	@ Note: We leave TTBCR as 0, meaning that only TTBR0 is used and
	@ we use the short-descriptor format (32-bit physical addresses).
	orr	r0, r1, #0b0001011	@ Sharable, Inner/Outer Write-Back Write-Allocate Cacheable
	mcr	p15, 0, r0, c2, c0, 0	@ set TTBR0

	@ Set access permission for domains.
	@ Domains are deprecated, but we have to configure them anyway.
	@ We mark every page as being domain 0 and set domain 0 to "client mode"
	@ (client mode = use access flags in page table).
	mov	r0, #1			@ 1 = client
	mcr	p15, 0, r0, c3, c0, 0	@ DACR

	@ Template (flags) for a 1 MB page-table entry.
	@ TEX[2:0] C B = 001 1 1 (outer and inner write-back, write-allocate)
	ldr	r8, =(0x2 +  		/* Section entry */ \
		      0xc +  		/* C B */ \
		      (3 << 10) + 	/* Read/write */ \
		      (1 << 12) +	/* TEX */ \
		      (1 << 16) +	/* Sharable */ \
		      (1<<19))		/* Non-secure */
	@ r8 = template page table entry

	@ Add an entry for the current physical section, at the old and new
	@ addresses. It's OK if they're the same.
	mov	r0, pc, lsr#20
	mov	r0, r0, lsl#20		@ r0 = physical address of this code's section start
	orr	r3, r0, r8		@ r3 = table entry for this section
	ldr	r4, =_libxenplat_start		@ r4 = desired virtual address of this section
	str	r3, [r1, r4, lsr#18] 	@ map desired virtual section to this code
	str	r3, [r1, r0, lsr#18]	@ map current section to this code too

	@ Invalidate TLB
	dsb				@ Caching is off, but must still prevent reordering
	mcr	p15, 0, r1, c8, c7, 0	@ TLBIALL

	@ Enable MMU / SCTLR
	mrc	p15, 0, r1, c1, c0, 0	@ SCTLR
	orr	r1, r1, #3 << 11	@ enable icache, branch prediction
	orr	r1, r1, #4 + 1		@ enable dcache, MMU
	mcr	p15, 0, r1, c1, c0, 0	@ SCTLR
	isb

	ldr	r1, =stage2		@ Virtual address of stage2
	bx	r1


.section .text
@ Called once the MMU is enabled. The boot code and the page table are mapped,
@ but nothing else is yet.
@
@ => r2 -> dtb (physical)
@    r7 = virtual address of page table
@    r8 = section entry template (flags)
@    r9 = desired physical - virtual offset
@    pc -> somewhere in newly-mapped virtual code section
stage2:
	@ Invalidate TLB
	mcr	p15, 0, r1, c8, c7, 0	@ TLBIALL
	isb

	@ The new mapping has now taken effect:
	@ r7 -> page_dir

	@ Fill in the whole top-level translation table (at page_dir).
	@ Populate the whole pagedir with 1MB section descriptors.

	mov	r1, r7			@ r1 -> first section entry
	add	r3, r1, #4*4*1024	@ limit (4 GB address space, 4 byte entries)
	orr	r0, r8, r9		@ r0 = entry mapping section zero to start of physical RAM
1:
	str	r0, [r1],#4		@ write the section entry
	add	r0, r0, #1 << 20 	@ next physical page (wraps)
	cmp	r1, r3
	bne	1b

	@ Invalidate TLB
	dsb
	mcr	p15, 0, r1, c8, c7, 0	@ TLBIALL
	isb

	@ Set VBAR -> exception_vector_table
	@ SCTLR.V = 0
	adr	r0, exception_vector_table
	mcr	p15, 0, r0, c12, c0, 0

	@ Enable hardware floating point:
	@ 1. Access to CP10 and CP11 must be enabled in the Coprocessor Access
	@    Control Register (CP15.CACR):
	mrc	p15, 0, r1, c1, c0, 2		@ CACR
	orr	r1, r1, #(3 << 20) + (3 << 22)	@ full access for CP10 & CP11
	mcr	p15, 0, r1, c1, c0, 2
	@ 2. The EN bit in the FPEXC register must be set:
	vmrs	r0, FPEXC
	orr	r0, r0, #1<<30		@ EN (enable)
	vmsr	FPEXC, r0

	@ Initialise 16 KB stack
	ldr	sp, =_boot_stack_end

	sub	r0, r2, r9		@ r0 -> device tree (virtual address)
	mov	r1, r9			@ r1 = _libxenplat_paddr_offset

	b	_libxenplat_armentry

.pushsection .bss
@ Note: calling _libxenplat_armentry zeroes out this region.
.align 12
.globl shared_info
shared_info:
	.fill (1024), 4, 0x0

.align 3
.globl irqstack
.globl irqstack_end
irqstack:
	.fill (1024), 4, 0x0
irqstack_end:

fault_dump:
	.fill 18, 4, 0x0		@ On fault, we save the registers + CPSR + handler address

.popsection

fault:
	cpsid	aif			@ Disable interrupts

	ldr	r13, =fault_dump
	stmia	r13, {r0-r12}		@ Dump the non-banked registers directly (well, unless from FIQ mode)
	str	r14, [r13, #15 << 2]	@ Our r14 is the faulting r15
	mov	r0, r13

	@ Save the caller's CPSR (our SPSR) too.
	mrs	r1, SPSR
	str	r1, [r13, #16 << 2]

	@ Switch to the mode we came from to get r13 and r14.
	@ If coming from user mode, use System mode instead so we're still
	@ privileged.
	and	r1, r1, #0x1f		@ r1 = SPSR mode
	cmp	r1, #0x10		@ If from User mode
	moveq	r1, #0x1f		@ Then use System mode instead

	mrs	r3, CPSR		@ r3 = our CPSR
	bic	r2, r3, #0x1f
	orr	r2, r2, r1
	msr	CPSR, r2		@ Change to mode r1

	@ Save old mode's r13, r14
	str	r13, [r0, #13 << 2]
	str	r14, [r0, #14 << 2]

	msr	CPSR, r3		@ Back to fault mode

	ldr	r1, [r0, #17 << 2]
	sub	r1, r1, #12		@ Fix to point at start of handler
	str	r1, [r0, #17 << 2]

	@ Call C code to format the register dump.
	@ Clobbers the stack, but we're not going to return anyway.
	ldr	sp, =_boot_stack_end
	b	_libxenplat_armpanic

@ We want to store a unique value to identify this handler, without corrupting
@ any of the registers. So, we store r15 (which will point just after the branch).
@ Later, we subtract 12 so the user gets pointed at the start of the exception
@ handler.
#define FAULT(name)			\
.globl fault_##name;			\
fault_##name:				\
	ldr	r13, =fault_dump;	\
	str	r15, [r13, #17 << 2];	\
	b	fault

FAULT(reset)
FAULT(undefined_instruction)
FAULT(svc)
FAULT(prefetch_call)
FAULT(prefetch_abort)
FAULT(data_abort)

@ exception base address
.align 5
.globl exception_vector_table
@ Note: remember to call CLREX if returning from an exception:
@ "The architecture enables the local monitor to treat any exclusive store as
@  matching a previous LDREX address. For this reason, use of the CLREX
@  instruction to clear an existing tag is required on context switches."
@ -- ARM Cortex-A Series Programmer’s Guide (Version: 4.0)
exception_vector_table:
	b	fault_reset
	b	fault_undefined_instruction
	b	fault_svc
	b	fault_prefetch_call
	b	fault_prefetch_abort
	b	fault_data_abort
	b	irq_handler @ IRQ
	.word 0xe7f000f0    @ abort on FIQ

@ Call fault_undefined_instruction in "Undefined mode"
bug:
	.word	0xe7f000f0    	@ und/udf - a "Permanently Undefined" instruction

irq_handler:
	ldr	sp, =irqstack_end
	push	{r0 - r12, r14}

	ldr	r0, IRQ_handler
	cmp	r0, #0
	beq	bug
	blx	r0		@ call handler

	@ Return from IRQ
	pop	{r0 - r12, r14}
	clrex
	subs	pc, lr, #4

.globl IRQ_handler
IRQ_handler:
	.long	0x0

@ This is called if you try to divide by zero. For now, we make a supervisor call,
@ which will make us halt.
.globl raise
raise:
	svc	0

/*
.globl __arch_switch_threads
@ => r0 = &prev->sp
@    r1 = &next->sp
@ <= returns to next thread's saved return address
__arch_switch_threads:
	push	{r4-r11}	@ Store callee-saved registers to old thread's stack
	stmia	r0, {sp, lr}	@ Store current sp and ip to prev's struct thread

	ldmia	r1, {sp, lr}	@ Load new sp, ip from next's struct thread
	pop	{r4-r11}	@ Load callee-saved registers from new thread's stack

	bx	lr

.globl arm_start_thread
arm_start_thread:
	pop	{r0, r1}
	@ r0 = user data
	@ r1 -> thread's main function
	ldr	lr, =exit_thread
	bx	r1
*/
