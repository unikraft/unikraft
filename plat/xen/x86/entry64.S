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
#include <x86/cpu_defs.h>
#include <x86/traps.h>
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
	andq $STACK_MASK_TOP, %rsp
	movq %rsi, %r8 /* esi contains pointer to start_info page */
	/* We will work on cr0 and cr4 multiple times.
	 * We put cr0 into rsi and cr4 into rdi, because cpuid and
	 * xgetbv/xsetbv work on eax/ebx/ecx/edx. */
	movq %cr0, %rsi
	movq %cr4, %rdi
	/* FPU and SSE are part of base x86-64, so no need to check for their
	 * availability before enabling and initializing. */
	andl $(~(X86_CR0_EM | X86_CR0_TS)), %esi
	orl $(X86_CR0_MP | X86_CR0_NE), %esi
	movq %rsi, %cr0
	fninit
#if __SSE__
	orl $(X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT), %edi
	movq %rdi, %cr4
	ldmxcsr (mxcsr_ptr)
#endif /* __SSE__ */
	/* Check capabilities subject to availability as indicated by cpuid.
	 * First, start off with "standard features" */
	movl $0x1, %eax
	cpuid
#if __AVX__
	/* ecx and edx now contain capability information, so we can now
	 * enable capabilities based on the indicated features */
	/* OSXSAVE needs to be enabled before AVX */
	testl $(X86_CPUID1_ECX_XSAVE), %ecx
	jz noxsave
	orl $(X86_CR4_OSXSAVE), %edi
	movq %rdi, %cr4
	/* now enable AVX. This needs to be last checking cpuid features from
	 * the eax=1 cpuid call, because it clobbers ecx */
	testl $(X86_CPUID1_ECX_AVX), %ecx
	jz noavx
	xorl %ecx, %ecx
	xgetbv
	orl $(X86_XCR0_SSE | X86_XCR0_AVX), %eax
	xsetbv
noavx:
noxsave:
#endif /* __AVX__ */
	/* Now, check for extended features. */
	movl $0x7, %eax
	movl $0x0, %ecx
	cpuid
	/* ebx, ecx, edx now contain extended capabilities information. */
	/* check for and enable FS/GSBASE */
	testl $(X86_CPUID7_EBX_FSGSBASE), %ebx
	jz nofsgsbase
	orl $(X86_CR4_FSGSBASE), %edi
	movq %rdi, %cr4
nofsgsbase:
	/* Done setting up CPU capabilities, hand over to C entry point. */
	movq %r8, %rdi /* pass pointer to start_info page to C entry */
	call _libxenplat_x86entry

.type mxcsr_ptr, @object
mxcsr_ptr:
	.long 0x1f80			/* Intel SDM power-on default */


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

.macro TRAP_ENTRY trapname, has_ec
ENTRY(ASM_TRAP_SYM(\trapname))
.if \has_ec
	errorentry do_\trapname
.else
	zeroentry  do_\trapname
.endif
.endm

.macro RESTORE_ALL
	movq __REGS_OFFSETOF_R15(%rsp), %r15
	movq __REGS_OFFSETOF_R14(%rsp), %r14
	movq __REGS_OFFSETOF_R13(%rsp), %r13
	movq __REGS_OFFSETOF_R12(%rsp), %r12
	movq __REGS_OFFSETOF_RBP(%rsp), %rbp
	movq __REGS_OFFSETOF_RBX(%rsp), %rbx
	movq __REGS_OFFSETOF_R11(%rsp), %r11
	movq __REGS_OFFSETOF_R10(%rsp), %r10
	movq __REGS_OFFSETOF_R9(%rsp),  %r9
	movq __REGS_OFFSETOF_R8(%rsp),  %r8
	movq __REGS_OFFSETOF_RAX(%rsp), %rax
	movq __REGS_OFFSETOF_RCX(%rsp), %rcx
	movq __REGS_OFFSETOF_RDX(%rsp), %rdx
	movq __REGS_OFFSETOF_RSI(%rsp), %rsi
	movq __REGS_OFFSETOF_RDI(%rsp), %rdi
	addq $__REGS_OFFSETOF_RIP,%rsp
.endm

.macro SAVE_ALL
	/* rdi slot contains rax, oldrax contains error code */
	cld
	subq $__REGS_OFFSETOF_RDI, %rsp
	movq %rsi, __REGS_OFFSETOF_RSI(%rsp)
	movq __REGS_OFFSETOF_RDI(%rsp), %rsi    /* load rax from rdi slot */
	movq %rdx, __REGS_OFFSETOF_RDX(%rsp)
	movq %rcx, __REGS_OFFSETOF_RCX(%rsp)
	movq %rsi, __REGS_OFFSETOF_RAX(%rsp)    /* store rax */
	movq %r8,  __REGS_OFFSETOF_R8(%rsp)
	movq %r9,  __REGS_OFFSETOF_R9(%rsp)
	movq %r10, __REGS_OFFSETOF_R10(%rsp)
	movq %r11, __REGS_OFFSETOF_R11(%rsp)
	movq %rbx, __REGS_OFFSETOF_RBX(%rsp)
	movq %rbp, __REGS_OFFSETOF_RBP(%rsp)
	movq %r12, __REGS_OFFSETOF_R12(%rsp)
	movq %r13, __REGS_OFFSETOF_R13(%rsp)
	movq %r14, __REGS_OFFSETOF_R14(%rsp)
	movq %r15, __REGS_OFFSETOF_R15(%rsp)
	movq %rdi, __REGS_OFFSETOF_RDI(%rsp)    /* put rdi into the slot */
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
	movq __REGS_OFFSETOF_ORIG_RAX(%rsp),%rsi	# get error code
	movq $-1,__REGS_OFFSETOF_ORIG_RAX(%rsp)
	call *%rax
	jmp error_exit


#ifdef CONFIG_PARAVIRT
/*
 * Xen event (virtual interrupt) entry point.
 */
ENTRY(ASM_TRAP_SYM(hypervisor_callback))
	zeroentry hypervisor_callback2

hypervisor_callback2:
	movq %rdi, %rsp

	/* check against event re-entrant */
	movq __REGS_OFFSETOF_RIP(%rsp),%rax
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
	movl __REGS_OFFSETOF_EFLAGS(%rsp), %eax
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
	jmp ASM_TRAP_SYM(hypervisor_callback)

# [How we do the fixup]. We want to merge the current stack frame with the
# just-interrupted frame. How we do this depends on where in the critical
# region the interrupted handler was executing, and so if rax has been
# restored. We determine by comparing interrupted rip with "restore_end".
# We always copy all registers below RIP from the current stack frame
# to the end of the previous activation frame so that we can continue
# as if we've never even reached 11 running in the old activation frame.

critical_region_fixup:
	# Set up source and destination region pointers
	leaq __REGS_OFFSETOF_RIP(%rsp),%rsi   # esi points at end of src region
	# Acquire interrupted rsp which was saved-on-stack. This points to
	# the end of dst region. Note that it is not necessarily current rsp
	# plus 0xb0, because the second interrupt might align the stack frame.
	movq __REGS_OFFSETOF_RSP(%rsp),%rdi   # edi points at end of dst region

	cmpq $restore_end,%rax
	jae  13f

	# If interrupted rip is before restore_end
	# then rax hasn't been restored yet
	movq (%rdi),%rax
	movq %rax, __REGS_OFFSETOF_RAX(%rsp)  # save rax
	addq $RSP_OFFSET,%rdi

	# Set up the copy
13:	movq $__REGS_OFFSETOF_RIP,%rcx
	shr  $3,%rcx          # convert bytes into count of 64-bit entities
15:	subq $8,%rsi          # pre-decrementing copy loop
	subq $8,%rdi
	movq (%rsi),%rax
	movq %rax,(%rdi)
	loop 15b
16:	movq %rdi,%rsp        # final rdi is top of merged stack
	andb $KERNEL_CS_MASK,__REGS_OFFSETOF_CS(%rsp)  # CS might have changed
	jmp  11b

#else
error_exit:
	RESTORE_ALL
	HYPERVISOR_IRET

/*
 * Xen event (virtual interrupt) entry point.
 */
TRAP_ENTRY hypervisor_callback, 0
#endif

ENTRY(asm_failsafe_callback)
#ifdef CONFIG_PARAVIRT
        popq  %rcx
        popq  %r11
#endif
        iretq


TRAP_ENTRY divide_error,        0
TRAP_ENTRY debug,               0
/* no NMI */
TRAP_ENTRY int3,                0
TRAP_ENTRY overflow,            0
TRAP_ENTRY bounds,              0
TRAP_ENTRY invalid_op,          0
TRAP_ENTRY no_device,           0
/* no Double Fault */
TRAP_ENTRY coproc_seg_overrun,  0
TRAP_ENTRY invalid_tss,         1
TRAP_ENTRY no_segment,          1
TRAP_ENTRY stack_error,         1 /* runs on exception stack */
TRAP_ENTRY gp_fault,            1
TRAP_ENTRY page_fault,          1
TRAP_ENTRY spurious_int,        1
TRAP_ENTRY coproc_error,        0
TRAP_ENTRY alignment_check,     1
/* no Machine Check */
TRAP_ENTRY simd_error,          0
/* no Virtualization Exception */


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
