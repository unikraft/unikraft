#ifndef __PLAT_KVM_X86_ASM_COMMON_S__
#define __PLAT_KVM_X86_ASM_COMMON_S__

#define ENTRY(x)	.globl x; .type x, %function; x:
#define END(x)		.size x, . - x

.macro GDT_64
.align 64
gdt64:
	.quad 0x0000000000000000
gdt64_cs:
	.quad GDT_DESC_CODE_VAL		/* 64bit CS	      */
gdt64_ds:
	.quad GDT_DESC_DATA_VAL		/* DS		      */
	.quad 0x0000000000000000	/* TSS part 1 (via C) */
	.quad 0x0000000000000000	/* TSS part 2 (via C) */
gdt64_end:
gdt64_ptr:
	.word gdt64_end-gdt64-1
	.quad gdt64
.endm

.macro PREPARE_OTHER_INSTR
	/* We will work on cr0 and cr4 multiple times.
	 * We put cr0 into rsi and cr4 into rdi, because cpuid and
	 * xgetbv/xsetbv work on eax/ebx/ecx/edx.
	 */
	movq 	%cr0, %rsi
	movq 	%cr4, %rdi

	/* FPU and SSE are part of base x86-64, so no need to check for their
	 * availability before enabling and initializing.
	 */
	andl 	$(~(X86_CR0_EM | X86_CR0_TS)), %esi
	orl 	$(X86_CR0_MP | X86_CR0_NE | X86_CR0_WP), %esi
	movq 	%rsi, %cr0
	fninit
#if __SSE__
	orl 	$(X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT), %edi
	movq 	%rdi, %cr4
	ldmxcsr (mxcsr_ptr)
#endif /* __SSE__ */
#if (__AVX__ || CONFIG_HAVE_X86PKU)
	/* Check capabilities subject to availability as indicated by cpuid.
	 * First, start off with "standard features"
	 */
	movl 	$0x1, %eax
	cpuid
	/* ecx and edx now contain capability information, so we can now
	 * enable capabilities based on the indicated features
	 */
	/* note: OSXSAVE needs to be enabled before AVX and PKU */
	testl 	$(X86_CPUID1_ECX_XSAVE), %ecx
	jz 	noxsave
	orl 	$(X86_CR4_OSXSAVE), %edi
	movq 	%rdi, %cr4
#if __AVX__
	/* now enable AVX. This needs to be last checking cpuid features from
	 * the eax=1 cpuid call, because it clobbers ecx
	 */
	testl 	$(X86_CPUID1_ECX_AVX), %ecx
	jz 	noavx
	xorl 	%ecx, %ecx
	xgetbv
	orl 	$(X86_XCR0_SSE | X86_XCR0_AVX), %eax
	xsetbv
noavx:
#endif /* __AVX__ */
/* Do not enable AVX without XSAVE, otherwise we'll get #UD */
noxsave:
#endif /* __AVX__ || CONFIG_HAVE_X86PKU */
	/* Now, check for extended features. */
	movl 	$0x7, %eax
	movl 	$0x0, %ecx
	cpuid
	/* ebx, ecx, edx now contain extended capabilities information. */
	/* check for and enable FS/GSBASE */
	testl 	$(X86_CPUID7_EBX_FSGSBASE), %ebx
	jz 	nofsgsbase
	orl $	(X86_CR4_FSGSBASE), %edi
	movq 	%rdi, %cr4
nofsgsbase:
#if CONFIG_HAVE_X86PKU
	/* check for Memory Protection Keys (PKU) */
	testl 	$(X86_CPUID7_ECX_PKU), %ecx
	jz 	nopku
	/* only enable PKU if we support XSAVE */
	testl 	$(X86_CR4_OSXSAVE), %edi
	jz 	nopku
	/* PKU is supported, enable it via CR4 */
	orl 	$(X86_CR4_PKE), %edi
	movq 	%rdi, %cr4
	/* also enable XSAVE for the PKRU */
	xorl 	%ecx, %ecx
	xgetbv
	orl 	$(X86_XCR0_PKRU), %eax
	xsetbv
nopku:
#endif /* CONFIG_HAVE_X86PKU */
.endm

#endif /* __PLAT_KVM_X86_ASM_COMMON_S__ */
