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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Taken from Mini-OS */

#include <uk/arch/types.h>
#include <uk/arch/limits.h>
#include <uk/config.h>
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

#define ENTRY(X) .globl X ; X :

#ifdef CONFIG_PARAVIRT
ELFNOTE(Xen, XEN_ELFNOTE_GUEST_OS, .asciz "Unikraft")
ELFNOTE(Xen, XEN_ELFNOTE_LOADER, .asciz "generic")
ELFNOTE(Xen, XEN_ELFNOTE_HYPERCALL_PAGE, _WORD hypercall_page)
ELFNOTE(Xen, XEN_ELFNOTE_XEN_VERSION, .asciz "xen-3.0")
.text

.globl _libxenplat_start

.section .text.boot, "ax"
_libxenplat_start:
#else

#include "entry_hvm.S"

#endif
        cld
        movq stack_start(%rip),%rsp
        andq $(~(__STACK_SIZE-1)), %rsp
        movq %rsi,%rdi
        call _libxenplat_x86entry

stack_start:
        .quad _libxenplat_bootstack + (2*__STACK_SIZE)

.globl _libxenplat_shared_info, hypercall_page
        /* Unpleasant -- the PTE that maps this page is actually overwritten */
        /* to map the real shared-info page! :-)                             */
        .align __PAGE_SIZE
_libxenplat_shared_info:
        .fill __PAGE_SIZE,1,0

hypercall_page:
        .fill __PAGE_SIZE,1,0


#define XEN_GET_VCPU_INFO(reg)	movq HYPERVISOR_shared_info,reg
#define XEN_LOCKED_BLOCK_EVENTS(reg)	movb $1,evtchn_upcall_mask(reg)
#define XEN_LOCKED_UNBLOCK_EVENTS(reg)	movb $0,evtchn_upcall_mask(reg)
#define XEN_TEST_PENDING(reg)	testb $0xFF,evtchn_upcall_pending(reg)

/* Offsets into shared_info_t. */
#define evtchn_upcall_pending		/* 0 */
#define evtchn_upcall_mask		1

NMI_MASK = 0x80000000
KERNEL_CS_MASK = 0xfc

#define RAX       80
#define RDI      112
#define ORIG_RAX 120       /* + error_code */
#define RIP      128
#define CS       136
#define RFLAGS   144
#define RSP      152


/* Macros */
.macro SAVE_PARAVIRT
#ifdef CONFIG_PARAVIRT
	pop %rcx
	pop %r11 /* rsp points to the error code */
#endif
.endm

.macro zeroentry sym
	SAVE_PARAVIRT
	pushq $0	/* push error code/oldrax */
	pushq %rax	/* push real oldrax to the rdi slot */
	leaq  \sym(%rip),%rax
	jmp error_entry
.endm

.macro errorentry sym
	SAVE_PARAVIRT
	pushq %rax
	leaq  \sym(%rip),%rax
	jmp error_entry
.endm

.macro RESTORE_ALL
	movq (%rsp),%r15
	movq 1*8(%rsp),%r14
	movq 2*8(%rsp),%r13
	movq 3*8(%rsp),%r12
	movq 4*8(%rsp),%rbp
	movq 5*8(%rsp),%rbx
	movq 6*8(%rsp),%r11
	movq 7*8(%rsp),%r10
	movq 8*8(%rsp),%r9
	movq 9*8(%rsp),%r8
	movq 10*8(%rsp),%rax
	movq 11*8(%rsp),%rcx
	movq 12*8(%rsp),%rdx
	movq 13*8(%rsp),%rsi
	movq 14*8(%rsp),%rdi
	addq $15*8+8,%rsp
.endm

.macro SAVE_ALL
	/* rdi slot contains rax, oldrax contains error code */
	cld
	subq $14*8,%rsp
	movq %rsi,13*8(%rsp)
	movq 14*8(%rsp),%rsi	/* load rax from rdi slot */
	movq %rdx,12*8(%rsp)
	movq %rcx,11*8(%rsp)
	movq %rsi,10*8(%rsp)	/* store rax */
	movq %r8, 9*8(%rsp)
	movq %r9, 8*8(%rsp)
	movq %r10,7*8(%rsp)
	movq %r11,6*8(%rsp)
	movq %rbx,5*8(%rsp)
	movq %rbp,4*8(%rsp)
	movq %r12,3*8(%rsp)
	movq %r13,2*8(%rsp)
	movq %r14,1*8(%rsp)
	movq %r15,(%rsp)
	movq %rdi, RDI(%rsp)	/* put rdi into the slot */
.endm

.macro HYPERVISOR_IRET
#ifdef CONFIG_PARAVIRT
	testl $NMI_MASK,2*8(%rsp)
	jnz   2f

	/* Direct iret to kernel space. Correct CS and SS. */
	orb   $3,1*8(%rsp)
	orb   $3,4*8(%rsp)
#endif
	iretq

#ifdef CONFIG_PARAVIRT
2:	/* Slow iret via hypervisor. */
	andl  $~NMI_MASK, 16(%rsp)
	pushq $0
	jmp  hypercall_page + (__HYPERVISOR_iret * 32)
#endif
.endm


/*
 * Exception entry point. This expects an error code/orig_rax on the stack
 * and the exception handler in %rax.
 */
error_entry:
	SAVE_ALL

	movq %rsp,%rdi
	movq ORIG_RAX(%rsp),%rsi	# get error code
	movq $-1,ORIG_RAX(%rsp)
	call *%rax
	jmp error_exit


#ifdef CONFIG_PARAVIRT
/*
 * Xen event (virtual interrupt) entry point.
 */
ENTRY(hypervisor_callback)
	zeroentry hypervisor_callback2

hypervisor_callback2:
	movq %rdi, %rsp

	/* check against event re-entrant */
	movq RIP(%rsp),%rax
	cmpq $scrit,%rax
	jb 11f
	cmpq $ecrit,%rax
	jb  critical_region_fixup

11:	movq %gs:8,%rax
	incl %gs:0
	cmovzq %rax,%rsp
	pushq %rdi
	call do_hypervisor_callback
	popq %rsp
	decl %gs:0

error_exit:
	movl RFLAGS(%rsp), %eax
	shr $9, %eax			# EAX[0] == IRET_RFLAGS.IF
	XEN_GET_VCPU_INFO(%rsi)
	andb evtchn_upcall_mask(%rsi),%al
	andb $1,%al			# EAX[0] == IRET_RFLAGS.IF & event_mask
	jnz restore_all_enable_events	#        != 0 => enable event delivery

	RESTORE_ALL
	HYPERVISOR_IRET

restore_all_enable_events:
	RESTORE_ALL
	pushq %rax                      # save rax; it will be clobbered later
	RSP_OFFSET=8                    # record the stack frame layout changes
	XEN_GET_VCPU_INFO(%rax)         # safe to use rax since it is saved
	XEN_LOCKED_UNBLOCK_EVENTS(%rax)

scrit:	/**** START OF CRITICAL REGION ****/
	XEN_TEST_PENDING(%rax)
	jz 12f
	XEN_LOCKED_BLOCK_EVENTS(%rax)   # if pending, mask events and handle
	                                # by jumping to hypervisor_prologue
12:	popq %rax                       # all registers restored from this point

restore_end:
	jnz hypervisor_prologue         # safe to jump out of critical region
	                                # because events are masked if ZF = 0
	HYPERVISOR_IRET
ecrit:  /**** END OF CRITICAL REGION ****/

# Set up the stack as Xen does before calling event callback
hypervisor_prologue:
	pushq %r11
	pushq %rcx
	jmp hypervisor_callback

# [How we do the fixup]. We want to merge the current stack frame with the
# just-interrupted frame. How we do this depends on where in the critical
# region the interrupted handler was executing, and so if rax has been
# restored. We determine by comparing interrupted rip with "restore_end".
# We always copy all registers below RIP from the current stack frame
# to the end of the previous activation frame so that we can continue
# as if we've never even reached 11 running in the old activation frame.

critical_region_fixup:
	# Set up source and destination region pointers
	leaq RIP(%rsp),%rsi   # esi points at end of src region
	# Acquire interrupted rsp which was saved-on-stack. This points to
	# the end of dst region. Note that it is not necessarily current rsp
	# plus 0xb0, because the second interrupt might align the stack frame.
	movq RSP(%rsp),%rdi   # edi points at end of dst region

	cmpq $restore_end,%rax
	jae  13f

	# If interrupted rip is before restore_end
	# then rax hasn't been restored yet
	movq (%rdi),%rax
	movq %rax, RAX(%rsp)  # save rax
	addq $RSP_OFFSET,%rdi

	# Set up the copy
13:	movq $RIP,%rcx
	shr  $3,%rcx          # convert bytes into count of 64-bit entities
15:	subq $8,%rsi          # pre-decrementing copy loop
	subq $8,%rdi
	movq (%rsi),%rax
	movq %rax,(%rdi)
	loop 15b
16:	movq %rdi,%rsp        # final rdi is top of merged stack
	andb $KERNEL_CS_MASK,CS(%rsp)      # CS might have changed
	jmp  11b

#else
error_exit:
	RESTORE_ALL
	HYPERVISOR_IRET

/*
 * Xen event (virtual interrupt) entry point.
 */
ENTRY(hypervisor_callback)
	zeroentry do_hypervisor_callback


#endif

ENTRY(failsafe_callback)
#ifdef CONFIG_PARAVIRT
        popq  %rcx
        popq  %r11
#endif
        iretq


ENTRY(coprocessor_error)
        zeroentry do_coprocessor_error


ENTRY(simd_coprocessor_error)
        zeroentry do_simd_coprocessor_error


ENTRY(device_not_available)
        zeroentry do_device_not_available


ENTRY(debug)
        zeroentry do_debug


ENTRY(int3)
        zeroentry do_int3

ENTRY(overflow)
        zeroentry do_overflow


ENTRY(bounds)
        zeroentry do_bounds


ENTRY(invalid_op)
        zeroentry do_invalid_op


ENTRY(coprocessor_segment_overrun)
        zeroentry do_coprocessor_segment_overrun


ENTRY(invalid_TSS)
        errorentry do_invalid_TSS


ENTRY(segment_not_present)
        errorentry do_segment_not_present


/* runs on exception stack */
ENTRY(stack_segment)
        errorentry do_stack_segment


ENTRY(general_protection)
        errorentry do_general_protection


ENTRY(alignment_check)
        errorentry do_alignment_check


ENTRY(divide_error)
        zeroentry do_divide_error


ENTRY(spurious_interrupt_bug)
        zeroentry do_spurious_interrupt_bug


ENTRY(page_fault)
        errorentry do_page_fault

#if HAVE_SCHED
ENTRY(thread_starter)
        popq %rdi
        popq %rbx
        pushq $0
        xorq %rbp,%rbp
        call *%rbx
        call exit_thread

ENTRY(__arch_switch_threads)
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	movq %rsp, (%rdi)		/* save ESP */
	movq (%rsi), %rsp		/* restore ESP */
	movq $1f, 8(%rdi)		/* save EIP */
	pushq 8(%rsi)			/* restore EIP */
	ret
1:
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	ret
#endif

#ifndef CONFIG_PARAVIRT
.data
.globl page_table_base
        .align __PAGE_SIZE
page_table_virt_l2:
        PTE(page_table_virt_l1 + L2_PROT)
        .align __PAGE_SIZE, 0
page_table_virt_l3:
        PTE(page_table_virt_l2 + L3_PROT)
        .align __PAGE_SIZE, 0
page_table_l3:
        PTE(page_table_l2 + L3_PROT)
        .align __PAGE_SIZE, 0
page_table_base:
        PTE(page_table_l3 + L4_PROT)
        PTE(page_table_virt_l3 + L4_PROT)
        .align __PAGE_SIZE, 0
#endif
