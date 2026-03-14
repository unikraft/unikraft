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

#include <uk/arch/x86_64.h>
#include <uk/arch/limits.h>
#include <uk/lcpu.h>
#include <xen-x86/traps.h>
#include <uk/config.h>
#include <xen/xen.h>
#include <xen/elfnote.h>
#include <uk/asm.h>
#include <uk/reloc.h>
#include <uk/pcpuvar.h>

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

#ifdef XEN_PARAVIRT
ELFNOTE(Xen, XEN_ELFNOTE_GUEST_OS, .asciz "Unikraft")
ELFNOTE(Xen, XEN_ELFNOTE_LOADER, .asciz "generic")
ELFNOTE(Xen, XEN_ELFNOTE_HYPERCALL_PAGE, _WORD hypercall_page)
ELFNOTE(Xen, XEN_ELFNOTE_XEN_VERSION, .asciz "xen-3.0")
.text

.globl _libxenplat_start

.section .text.boot, "ax"
_libxenplat_start:

#endif
	cld

	/* esi contains pointer to start_info page
	 * Temporarily store it on the default scratch stack given by Xen */
	pushq %rsi

	/* Zero-out the first two arguments  of do_uk_reloc.
	 * do_uk_reloc can detect the base load address on its own in 64-bit */
	xorq %rsi, %rsi
	xorq %rdi, %rdi
	call do_uk_reloc
	popq %rsi

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
	andl $(~(UK_ARCH_X86_64_CR0_EM | UK_ARCH_X86_64_CR0_TS)), %esi
	orl $(UK_ARCH_X86_64_CR0_MP | UK_ARCH_X86_64_CR0_NE), %esi
	movq %rsi, %cr0
	fninit
#if __SSE__
	orl $(UK_ARCH_X86_64_CR4_OSFXSR | UK_ARCH_X86_64_CR4_OSXMMEXCPT), %edi
	movq %rdi, %cr4

	leaq mxcsr_ptr(%rip),%r9
	ldmxcsr (%r9d)
#endif /* __SSE__ */
#if (__AVX__ || CONFIG_HAVE_X86PKU)
	/* Check capabilities subject to availability as indicated by cpuid.
	 * First, start off with "standard features" */
	movl $0x1, %eax
	cpuid
	/* ecx and edx now contain capability information, so we can now
	 * enable capabilities based on the indicated features */
	/* note: OSXSAVE needs to be enabled before AVX and PKU */
	testl $(UK_ARCH_X86_64_CPUID1_ECX_XSAVE), %ecx
	jz noxsave
	orl $(UK_ARCH_X86_64_CR4_OSXSAVE), %edi
	movq %rdi, %cr4
#if __AVX__
	/* now enable AVX. This needs to be last checking cpuid features from
	 * the eax=1 cpuid call, because it clobbers ecx */
	testl $(UK_ARCH_X86_64_CPUID1_ECX_AVX), %ecx
	jz noavx
	xorl %ecx, %ecx
	xgetbv
	orl $(UK_ARCH_X86_64_XCR0_SSE | UK_ARCH_X86_64_XCR0_AVX), %eax
	xsetbv
noavx:
#endif /* __AVX__ */
/* Do not enable AVX without XSAVE, otherwise we'll get #UD */
noxsave:
#endif /* __AVX__ || CONFIG_HAVE_X86PKU */
	/* Now, check for extended features. */
	movl $0x7, %eax
	movl $0x0, %ecx
	cpuid
	/* ebx, ecx, edx now contain extended capabilities information. */
	/* check for and enable FS/GSBASE */
	testl $(UK_ARCH_X86_64_CPUID7_EBX_FSGSBASE), %ebx
	jz nofsgsbase
	orl $(UK_ARCH_X86_64_CR4_FSGSBASE), %edi
	movq %rdi, %cr4
nofsgsbase:
#if CONFIG_HAVE_X86PKU
	/* check for Memory Protection Keys (PKU) */
	testl $(UK_ARCH_X86_64_CPUID7_ECX_PKU), %ecx
	jz nopku
	/* only enable PKU if we support XSAVE */
	testl $(UK_ARCH_X86_64_CR4_OSXSAVE), %edi
	jz nopku
	/* PKU is supported, enable it via CR4 */
	orl $(UK_ARCH_X86_64_CR4_PKE), %edi
	movq %rdi, %cr4
	/* also enable XSAVE for the PKRU */
	xorl %ecx, %ecx
	xgetbv
	orl $(UK_ARCH_X86_64_XCR0_PKRU), %eax
	xsetbv
nopku:
#endif /* CONFIG_HAVE_X86PKU */
	/* Done setting up CPU capabilities, hand over to C entry point. */
	movq %r8, %rdi /* pass pointer to start_info page to C entry */
	call _libxenplat_x86entry /* should not return */
	ud2 /* trap in case it does */

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


#define XEN_GET_VCPU_INFO(reg)	movq HYPERVISOR_shared_info(%rip),reg
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
#ifdef XEN_PARAVIRT
	pop %rcx
	pop %r11 /* rsp now points to the error code */
#endif
.endm

#define TRAP_ENTRY(name, has_ec) \
	_TRAP_ENTRY name, UK_ARCH_X86_64_TRAPNUM_##name, has_ec

.macro _TRAP_ENTRY trapname, trapnum, has_ec
ENTRY(ASM_TRAP_SYM(\trapname))
	SAVE_PARAVIRT
.if !\has_ec
	pushq	$0 /* push fake error code on stack */
.endif
	pushq	%rax /* push old rax */
	movq	$\trapnum, %rax
	jmp error_entry
.endm

.macro RESTORE_ALL
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R15(%rsp), %r15
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R14(%rsp), %r14
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R13(%rsp), %r13
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R12(%rsp), %r12
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RBP(%rsp), %rbp
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RBX(%rsp), %rbx
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R11(%rsp), %r11
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R10(%rsp), %r10
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R9(%rsp),  %r9
	movq UK_LCPU_X86_64_REGS_OFFSETOF_R8(%rsp),  %r8
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RAX(%rsp), %rax
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RCX(%rsp), %rcx
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RDX(%rsp), %rdx
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RSI(%rsp), %rsi
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RDI(%rsp), %rdi
	addq $UK_LCPU_X86_64_REGS_OFFSETOF_RIP,%rsp
.endm

.macro SAVE_ALL
	/* rdi slot contains rax, oldrax contains error code */
	cld
	subq $UK_LCPU_X86_64_REGS_OFFSETOF_RDI, %rsp
	movq %rsi, UK_LCPU_X86_64_REGS_OFFSETOF_RSI(%rsp)
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RDI(%rsp), %rsi    /* load rax from rdi slot */
	movq %rdx, UK_LCPU_X86_64_REGS_OFFSETOF_RDX(%rsp)
	movq %rcx, UK_LCPU_X86_64_REGS_OFFSETOF_RCX(%rsp)
	movq %rsi, UK_LCPU_X86_64_REGS_OFFSETOF_RAX(%rsp)    /* store rax */
	movq %r8,  UK_LCPU_X86_64_REGS_OFFSETOF_R8(%rsp)
	movq %r9,  UK_LCPU_X86_64_REGS_OFFSETOF_R9(%rsp)
	movq %r10, UK_LCPU_X86_64_REGS_OFFSETOF_R10(%rsp)
	movq %r11, UK_LCPU_X86_64_REGS_OFFSETOF_R11(%rsp)
	movq %rbx, UK_LCPU_X86_64_REGS_OFFSETOF_RBX(%rsp)
	movq %rbp, UK_LCPU_X86_64_REGS_OFFSETOF_RBP(%rsp)
	movq %r12, UK_LCPU_X86_64_REGS_OFFSETOF_R12(%rsp)
	movq %r13, UK_LCPU_X86_64_REGS_OFFSETOF_R13(%rsp)
	movq %r14, UK_LCPU_X86_64_REGS_OFFSETOF_R14(%rsp)
	movq %r15, UK_LCPU_X86_64_REGS_OFFSETOF_R15(%rsp)
	movq %rdi, UK_LCPU_X86_64_REGS_OFFSETOF_RDI(%rsp)    /* put rdi into the slot */
.endm

.macro HYPERVISOR_IRET
#ifdef XEN_PARAVIRT
	testl $NMI_MASK,2*8(%rsp)
	jnz   2f

	/* Direct iret to kernel space. Correct CS and SS. */
	orb   $3,1*8(%rsp)
	orb   $3,4*8(%rsp)
#endif
	iretq

#ifdef XEN_PARAVIRT
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

	movq	%rax, %rdi /* Load trap number */
	movq	%rsp, %rsi /* Load regs ptr */
	movq UK_LCPU_X86_64_REGS_OFFSETOF_ORIG_RAX(%rsp),%rdx /* Load error code */
	call uk_plat_xen_except_err_handler
	jmp error_exit


#ifdef XEN_PARAVIRT
/*
 * Xen event (virtual interrupt) entry point.
 */
ENTRY(ASM_TRAP_SYM(HYPERVISOR_CALLBACK))
	SAVE_PARAVIRT
	pushq	$0	/* push fake error code */
	pushq	%rax	/* push real oldrax to the rdi slot */
	SAVE_ALL
	movq	%rsp, %rdi /* Load regs ptr */
	movq	UK_LCPU_X86_64_REGS_OFFSETOF_ORIG_RAX(%rsp), %rsi /* Load err */
	call	hypervisor_callback2
	jmp error_exit

hypervisor_callback2:
	movq %rdi, %rsp

	/* check against event re-entrant */
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RIP(%rsp),%rax
	leaq scrit(%rip),%r9
	cmpq %r9,%rax
	jb 11f
	leaq ecrit(%rip),%r9
	cmpq %r9,%rax
	jb  critical_region_fixup

11:
	/* Switch stack iff we're not in nested exception AND auxsp is set */
	xor %eax, %eax
	incl UK_PCPUVAR_X86_64_GSREL(xen_irqcount)
	cmovzq UK_PCPUVAR_X86_64_GSREL(uk_plat_xen_auxsp), %rax
	test %rax, %rax
	cmovnzq %rax, %rsp
	mov %rdi, %rbx
	call do_hypervisor_callback
	mov %rbx, %rsp
	decl UK_PCPUVAR_X86_64_GSREL(xen_irqcount)

error_exit:
	movl UK_LCPU_X86_64_REGS_OFFSETOF_RFLAGS(%rsp), %eax
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
	jmp ASM_TRAP_SYM(HYPERVISOR_CALLBACK)

# [How we do the fixup]. We want to merge the current stack frame with the
# just-interrupted frame. How we do this depends on where in the critical
# region the interrupted handler was executing, and so if rax has been
# restored. We determine by comparing interrupted rip with "restore_end".
# We always copy all registers below RIP from the current stack frame
# to the end of the previous activation frame so that we can continue
# as if we've never even reached 11 running in the old activation frame.

critical_region_fixup:
	# Set up source and destination region pointers
	leaq UK_LCPU_X86_64_REGS_OFFSETOF_RIP(%rsp),%rsi   # esi points at end of src region
	# Acquire interrupted rsp which was saved-on-stack. This points to
	# the end of dst region. Note that it is not necessarily current rsp
	# plus 0xb0, because the second interrupt might align the stack frame.
	movq UK_LCPU_X86_64_REGS_OFFSETOF_RSP(%rsp),%rdi   # edi points at end of dst region

	leaq restore_end(%rip),%r9
	cmpq %r9,%rax
	jae  13f

	# If interrupted rip is before restore_end
	# then rax hasn't been restored yet
	movq (%rdi),%rax
	movq %rax, UK_LCPU_X86_64_REGS_OFFSETOF_RAX(%rsp)  # save rax
	addq $RSP_OFFSET,%rdi

	# Set up the copy
13:	movq $UK_LCPU_X86_64_REGS_OFFSETOF_RIP,%rcx
	shr  $3,%rcx          # convert bytes into count of 64-bit entities
15:	subq $8,%rsi          # pre-decrementing copy loop
	subq $8,%rdi
	movq (%rsi),%rax
	movq %rax,(%rdi)
	loop 15b
16:	movq %rdi,%rsp        # final rdi is top of merged stack
	andb $KERNEL_CS_MASK,UK_LCPU_X86_64_REGS_OFFSETOF_CS(%rsp)  # CS might have changed
	jmp  11b
#endif

ENTRY(asm_failsafe_callback)
#ifdef XEN_PARAVIRT
        popq  %rcx
        popq  %r11
#endif
        iretq


TRAP_ENTRY(DIVIDE_ERROR,        0)
TRAP_ENTRY(DEBUG,               0)
/* no NMI */
TRAP_ENTRY(INT3,                0)
TRAP_ENTRY(OVERFLOW,            0)
TRAP_ENTRY(BOUNDS,              0)
TRAP_ENTRY(INVALID_OP,          0)
TRAP_ENTRY(NO_DEVICE,           0)
/* no Double Fault */
TRAP_ENTRY(COPROC_SEG_OVERRUN,  0)
TRAP_ENTRY(INVALID_TSS,         1)
TRAP_ENTRY(NO_SEGMENT,          1)
TRAP_ENTRY(STACK_ERROR,         1) /* runs on exception stack */
TRAP_ENTRY(GP_FAULT,            1)
TRAP_ENTRY(PAGE_FAULT,          1)
TRAP_ENTRY(SPURIOUS_INT,        1)
TRAP_ENTRY(COPROC_ERROR,        0)
TRAP_ENTRY(ALIGNMENT_CHECK,     1)
/* no Machine Check */
TRAP_ENTRY(SIMD_ERROR,          0)
/* no Virtualization Exception */
TRAP_ENTRY(SECURITY_ERROR,      0)

/*
 * We do not want to unmap anything.
 */
.section .bss
.align	4
.globl	bpt_unmap_mrd
bpt_unmap_mrd:
.space	64
