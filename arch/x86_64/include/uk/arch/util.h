/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ARCH_UTIL_H__
#define __UK_ARCH_UTIL_H__

#if !__ASSEMBLY__
#include <uk/essentials.h>
#include <uk/arch.h>

/**
 * Read stack pointer
 *
 * @return Current stack pointer value
 */
static inline unsigned long uk_arch_read_sp(void)
{
	unsigned long sp;

	asm volatile (
		"mov	%%rsp, %0"
		: "=r" (sp)
	);

	return sp;
}

/**
 * Spin-wait hint
 */
static inline void uk_arch_spinwait(void)
{
	asm volatile (
		"pause"
		:
		:
		: "memory"
	);
}

/**
 * Query CPU features and information
 *
 * @param fn Main function number
 * @param subfn Sub-function number
 * @param eax Pointer to store EAX output
 * @param ebx Pointer to store EBX output
 * @param ecx Pointer to store ECX output
 * @param edx Pointer to store EDX output
 */
static inline void uk_arch_cpuid(__u32 fn, __u32 subfn,
				 __u32 *eax, __u32 *ebx,
				 __u32 *ecx, __u32 *edx)
{
	asm volatile (
		"cpuid"
		: "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
		: "a" (fn), "c" (subfn)
	);
}

/**
 * Read RFLAGS register
 *
 * @return Current RFLAGS value
 */
static inline unsigned long uk_arch_rflags_get(void)
{
	unsigned long flags;

	asm volatile (
		"pushfq\n\t"
		"popq	%0\n\t"
		: "=g" (flags)
		:
		: "memory"
	);

	return flags;
}

/**
 * Enable interrupts
 */
static inline void uk_arch_enable_irq(void)
{
	asm volatile (
		"sti"
		:
		:
		: "memory"
	);
}

/**
 * Disable interrupts
 */
static inline void uk_arch_disable_irq(void)
{
	asm volatile (
		"cli"
		:
		:
		: "memory"
	);
}

/**
 * Halt processor
 */
static inline void uk_arch_halt(void)
{
	asm volatile (
		"hlt"
		:
		:
		: "memory"
	);
}

/**
 * Full memory barrier
 */
static inline void uk_arch_mb(void)
{
	asm volatile (
		"mfence"
		:
		:
		: "memory"
	);
}

/**
 * Read memory barrier
 */
static inline void uk_arch_rmb(void)
{
	asm volatile (
		"lfence"
		:
		:
		: "memory"
	);
}

/**
 * Write memory barrier
 */
static inline void uk_arch_wmb(void)
{
	asm volatile (
		"sfence"
		:
		:
		: "memory"
	);
}

/**
 * No operation
 */
static inline void uk_arch_nop(void)
{
	asm volatile (
		"nop"
		:
		:
		: "memory"
	);
}

/**
 * Read CR2 (page-fault address)
 *
 * @return Current CR2 value
 */
static inline unsigned long uk_arch_rdcr2(void)
{
	unsigned long cr2;

	asm volatile (
		"mov %%cr2, %0"
		: "=r"(cr2)
	);

	return cr2;
}

/**
 * Write CR3 (page table base)
 *
 * @param cr3 Value to write
 */
static inline void uk_arch_wrcr3(unsigned long cr3)
{
	asm volatile (
		"mov	%0, %%cr3"
		:
		: "r" (cr3)
		: "memory"
	);
}

/**
 * Invalidate TLB entry
 *
 * @param va Virtual address
 */
static inline void uk_arch_invlpg(unsigned long va)
{
	asm volatile (
		"invlpg	%0"
		:
		: "m" (*(const char *)(va))
		: "memory"
	);
}

/**
 * Read MSR
 *
 * @param msr MSR address
 * @param lo Pointer to store lower 32 bits
 * @param hi Pointer to store upper 32 bits
 */
static inline void uk_arch_rdmsr(unsigned int msr, __u32 *lo, __u32 *hi)
{
	asm volatile (
		"rdmsr"
		: "=a" (*lo), "=d" (*hi)
		: "c" (msr)
	);
}

/**
 * Read 64-bit MSR
 *
 * @param msr MSR address
 * @return 64-bit MSR value
 */
static inline __u64 uk_arch_rdmsrl(unsigned int msr)
{
	__u32 lo, hi;

	uk_arch_rdmsr(msr, &lo, &hi);
	return ((__u64)lo | (__u64)hi << 32);
}

/**
 * Write MSR
 *
 * @param msr MSR address
 * @param lo Lower 32 bits
 * @param hi Upper 32 bits
 */
static inline void uk_arch_wrmsr(unsigned int msr, __u32 lo, __u32 hi)
{
	asm volatile (
		"wrmsr"
		:
		: "c" (msr), "a" (lo), "d" (hi)
	);
}

/**
 * Write 64-bit MSR
 *
 * @param msr MSR address
 * @param val 64-bit value
 */
static inline void uk_arch_wrmsrl(unsigned int msr, __u64 val)
{
	uk_arch_wrmsr(msr, (__u32)(val & 0xffffffffULL), (__u32)(val >> 32));
}

/**
 * Read timestamp counter
 *
 * @return 64-bit TSC value
 */
static inline __u64 uk_arch_rdtsc(void)
{
	__u64 l, h;

	asm volatile (
		"rdtsc"
		: "=a" (l), "=d" (h)
	);
	return (h << 32) | l;
}

/**
 * Read 8-bit from memory
 *
 * @param addr Memory address
 * @return 8-bit value
 */
static inline __u8 uk_arch_readb(const volatile void *addr)
{
	__u8 v;

	asm volatile (
		"movb	%1, %0"
		: "=q" (v)
		: "m" (*(volatile __u8 *)addr)
	);
	return v;
}

/**
 * Read 16-bit from memory
 *
 * @param addr Memory address
 * @return 16-bit value
 */
static inline __u16 uk_arch_readw(const volatile void *addr)
{
	__u16 v;

	asm volatile (
		"movw	%1, %0"
		: "=r" (v)
		: "m" (*(volatile __u16 *)addr)
	);
	return v;
}

/**
 * Read 32-bit from memory
 *
 * @param addr Memory address
 * @return 32-bit value
 */
static inline __u32 uk_arch_readl(const volatile void *addr)
{
	__u32 v;

	asm volatile (
		"movl	%1, %0"
		: "=r" (v)
		: "m" (*(volatile __u32 *)addr)
	);
	return v;
}

/**
 * Read 64-bit from memory
 *
 * @param addr Memory address
 * @return 64-bit value
 */
static inline __u64 uk_arch_readq(const volatile void *addr)
{
	__u64 v;

	asm volatile (
		"movq	%1, %0"
		: "=r" (v)
		: "m" (*(volatile __u64 *)addr)
	);
	return v;
}

/**
 * Write 8-bit to memory
 *
 * @param addr Memory address
 * @param v Value to write
 */
static inline void uk_arch_writeb(volatile void *addr, __u8 v)
{
	asm volatile (
		"movb	%0, %1"
		:
		: "q" (v), "m" (*(volatile __u8 *)addr)
	);
}

/**
 * Write 16-bit to memory
 *
 * @param addr Memory address
 * @param v Value to write
 */
static inline void uk_arch_writew(volatile void *addr, __u16 v)
{
	asm volatile (
		"movw	%0, %1"
		:
		: "r" (v), "m" (*(volatile __u16 *)addr)
	);
}

/**
 * Write 32-bit to memory
 *
 * @param addr Memory address
 * @param v Value to write
 */
static inline void uk_arch_writel(volatile void *addr, __u32 v)
{
	asm volatile (
		"movl	%0, %1"
		:
		: "r" (v), "m" (*(volatile __u32 *)addr)
	);
}

/**
 * Write 64-bit to memory
 *
 * @param addr Memory address
 * @param v Value to write
 */
static inline void uk_arch_writeq(volatile void *addr, __u64 v)
{
	asm volatile (
		"movq	%0, %1"
		:
		: "r" (v), "m" (*(volatile __u64 *)addr)
	);
}

/**
 * Read byte from I/O port
 *
 * @param port I/O port address
 * @return 8-bit value
 */
static inline __u8 uk_arch_inb(__u16 port)
{
	__u8 v;

	asm volatile (
		"inb	%1,%0"
		: "=a" (v)
		: "dN" (port)
	);
	return v;
}

/**
 * Read word from I/O port
 *
 * @param port I/O port address
 * @return 16-bit value
 */
static inline __u16 uk_arch_inw(__u16 port)
{
	__u16 v;

	asm volatile (
		"inw	%1, %0"
		: "=a" (v)
		: "dN" (port)
		);
	return v;
}

/**
 * Read dword from I/O port
 *
 * @param port I/O port address
 * @return 32-bit value
 */
static inline __u32 uk_arch_inl(__u16 port)
{
	__u32 v;

	asm volatile (
		"inl	%1, %0"
		: "=a" (v)
		: "dN" (port)
	);
	return v;
}

/**
 * Read qword from I/O port (two 32-bit reads)
 *
 * @param port_lo Lower I/O port address
 * @return 64-bit value
 */
static inline __u64 uk_arch_inq(__u16 port_lo)
{
	__u16 port_hi = port_lo + 4;
	__u32 lo, hi;

	asm volatile (
		"inl	%1, %0"
		: "=a" (lo)
		: "dN" (port_lo)
	);
	asm volatile (
		"inl	%1, %0"
		: "=a" (hi)
		: "dN" (port_hi)
	);

	return ((__u64)lo) | ((__u64)hi << 32);
}

/**
 * Write byte to I/O port
 *
 * @param port I/O port address
 * @param v Value to write
 */
static inline void uk_arch_outb(__u16 port, __u8 v)
{
	asm volatile (
		"outb	%0, %1"
		:
		: "a" (v), "dN" (port)
	);
}

/**
 * Write word to I/O port
 *
 * @param port I/O port address
 * @param v Value to write
 */
static inline void uk_arch_outw(__u16 port, __u16 v)
{
	asm volatile (
		"outw	%0, %1"
		:
		: "a" (v),
		"dN" (port)
	);
}

/**
 * Write dword to I/O port
 *
 * @param port I/O port address
 * @param v Value to write
 */
static inline void uk_arch_outl(__u16 port, __u32 v)
{
	asm volatile (
		"outl	%0, %1"
		:
		: "a" (v), "dN" (port)
	);
}

/**
 * Multiply 64-bit by 32-bit with shift
 *
 * @param a 64-bit multiplicand
 * @param b 32-bit multiplier
 * @return (a * b) >> 32
 */
static inline __u64 uk_arch_mul64_32(__u64 a, __u32 b)
{
	__u64 prod;

	asm volatile (
		"mul %%rdx ; "
		"shrd $32, %%rdx, %%rax"
		: "=a" (prod)
		: "0" (a), "d" ((__u64)b)
	);

	return prod;
}

/*
 * The compiler seems to generate warnings because of the *(__off *)offset line
 * as it thinks we are trying to dereference the zero-page. Silence these
 * warnings for these fsgsbase utils by using pragma. Another way we could
 * have solved this is by treating 'offset' as a register operand:
 *	asm volatile (
 *		"movb	%1, %%gs:(%0)\n"
 *		:
 *		: "r" (offset) "r" (val)
 *		: "memory"
 *	);
 * This way 'offset' is loaded into a register first, then used for indirect
 * addressing (e.g. 'movb %al, %gs:(%rcx)'), whereas in the method we use below
 * the offset value itself is used as a displacement in the segment-relative
 * addressing (e.g. 'movb %al, %gs:0x10'). The latter is more efficient as
 * it avoids an extra register allocation and the compiler can directly
 * encode the offset in the instruction (if constant).
 * Therefore, go with the pragma for now.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
/**
 * Write to GS:offset
 *
 * @param val Value to write
 * @param offset Offset from GS base
 */
static inline void uk_arch_wrgsbase8(__u8 val, __off offset)
{
	asm volatile (
		"movb	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Write to GS:offset
 *
 * @param val Value to write
 * @param offset Offset from GS base
 */
static inline void uk_arch_wrgsbase16(__u16 val, __off offset)
{
	asm volatile (
		"movw	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Write to GS:offset
 *
 * @param val Value to write
 * @param offset Offset from GS base
 */
static inline void uk_arch_wrgsbase32(__u32 val, __off offset)
{
	asm volatile (
		"movl	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Write to GS:offset
 *
 * @param val Value to write
 * @param offset Offset from GS base
 */
static inline void uk_arch_wrgsbase64(__u64 val, __off offset)
{
	asm volatile (
		"movq	%1, %%gs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Read from GS:offset
 *
 * @param offset Offset from GS base
 * @return 8-bit value
 */
static inline __u8 uk_arch_rdgsbase8(__off offset)
{
	__u8 val;

	asm volatile (
		"movb	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

/**
 * Read from GS:offset
 *
 * @param offset Offset from GS base
 * @return 16-bit value
 */
static inline __u16 uk_arch_rdgsbase16(__off offset)
{
	__u16 val;

	asm volatile (
		"movw	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

/**
 * Read from GS:offset
 *
 * @param offset Offset from GS base
 * @return 32-bit value
 */
static inline __u32 uk_arch_rdgsbase32(__off offset)
{
	__u32 val;

	asm volatile (
		"movl	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

/**
 * Read from GS:offset
 *
 * @param offset Offset from GS base
 * @return 64-bit value
 */
static inline __u64 uk_arch_rdgsbase64(__off offset)
{
	__u64 val;

	asm volatile (
		"movq	%%gs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

/**
 * Write to FS:offset
 *
 * @param val Value to write
 * @param offset Offset from FS base
 */
static inline void uk_arch_wrfsbase8(__u8 val, __off offset)
{
	asm volatile (
		"movb	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Write to FS:offset
 *
 * @param val Value to write
 * @param offset Offset from FS base
 */
static inline void uk_arch_wrfsbase16(__u16 val, __off offset)
{
	asm volatile (
		"movw	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Write to FS:offset
 *
 * @param val Value to write
 * @param offset Offset from FS base
 */
static inline void uk_arch_wrfsbase32(__u32 val, __off offset)
{
	asm volatile (
		"movl	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Write to FS:offset
 *
 * @param val Value to write
 * @param offset Offset from FS base
 */
static inline void uk_arch_wrfsbase64(__u64 val, __off offset)
{
	asm volatile (
		"movq	%1, %%fs:%0\n"
		: "=m" (*(__off *)offset)
		: "r" (val)
		: "memory"
	);
}

/**
 * Read from FS:offset
 *
 * @param offset Offset from FS base
 * @return 8-bit value
 */
static inline __u8 uk_arch_rdfsbase8(__off offset)
{
	__u8 val;

	asm volatile (
		"movb	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

/**
 * Read from FS:offset
 *
 * @param offset Offset from FS base
 * @return 16-bit value
 */
static inline __u16 uk_arch_rdfsbase16(__off offset)
{
	__u16 val;

	asm volatile (
		"movw	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

/**
 * Read from FS:offset
 *
 * @param offset Offset from FS base
 * @return 32-bit value
 */
static inline __u32 uk_arch_rdfsbase32(__off offset)
{
	__u32 val;

	asm volatile (
		"movl	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}

/**
 * Read from FS:offset
 *
 * @param offset Offset from FS base
 * @return 64-bit value
 */
static inline __u64 uk_arch_rdfsbase64(__off offset)
{
	__u64 val;

	asm volatile (
		"movq	%%fs:%1, %0\n"
		: "=r" (val)
		: "m" (*(__off *)offset)
		: "memory"
	);

	return val;
}
#pragma GCC diagnostic pop

/**
 * Load Global Descriptor Table
 *
 * @param gdtptr Pointer to GDT descriptor structure
 */
static inline void uk_arch_lgdt(const struct uk_arch_desc_table_ptr64 *gdtptr)
{
	asm volatile (
		"lgdt	%0\n"
		:
		: "m" (*gdtptr)
		: "memory"
	);
}

/**
 * Load Task Register
 *
 * @param tss_selector TSS segment selector (GDT offset)
 */
static inline void uk_arch_ltr(__u16 tss_selector)
{
	asm volatile (
		"ltr	%0\n"
		:
		: "r" (tss_selector)
	);
}

/**
 * Load Interrupt Descriptor Table
 *
 * @param idtptr Pointer to IDT descriptor structure
 */
static inline void uk_arch_lidt(const struct uk_arch_desc_table_ptr64 *idtptr)
{
	asm volatile (
		"lidt	%0\n"
		:
		: "m" (*idtptr)
		: "memory"
	);
}

/**
 * Set CS, ES, SS and DS in that order.
 *
 * Performs a far return to load CS, then updates ES, SS, and DS.
 * Useful if a new GDT has been loaded before as the updates won't be
 * propagated until the segments have been refreshed.
 *
 * @param cs_offset Code segment descriptor offset
 * @param ds_offset Data segment descriptor offset
 */
static inline void uk_arch_set_segs(__u32 cs_offset, __u32 ds_offset)
{
	asm volatile (
		/* Perform a far return to enable the new CS */
		"leaq	1f(%%rip), %%rax\n"
		"pushq	%0\n"
		"pushq	%%rax\n"
		"lretq\n"

		/* Update remaining segment registers */
		"1:\n"
		"movl	%1, %%es\n"
		"movl	%1, %%ss\n"
		"movl	%1, %%ds\n"
		:
		: "r" ((__u64)cs_offset),
		  "r" (ds_offset)
		: "rax", "memory"
	);
}

/**
 * Set current instruction pointer to a custom place by jumping to it while
 * while switching the stack pointer to a custom value beforehand.
 *
 * This function does not return!
 *
 * @param sp Stack pointer to switch to before jumping
 * @param ip The place to jump to
 */
static inline void __noreturn uk_arch_jump_to(__u64 sp, __u64 ip)
{
	asm volatile (
		"movq	%0, %%rsp\n"

		/* According to System V AMD64 the stack pointer must be
		 * aligned to 16-bytes. In other words, the value (RSP+8) must
		 * be a multiple of 16 when control is transferred to the
		 * function entry point (i.e., the compiler expects a
		 * misalignment due to the return address having been pushed
		 * onto the stack).
		 */
		"andq	$~0xf, %%rsp\n"
		"subq	$0x8, %%rsp\n"

#if !__OMIT_FRAMEPOINTER__
		"xorq	%%rbp, %%rbp\n"
#endif /* __OMIT_FRAMEPOINTER__ */

		"jmp	*%1\n"
		:
		: "r" (sp), "r" (ip)
		: /* clobbers not needed */);

	/* just make the compiler happy about returning function */
	__builtin_unreachable();
}

/**
 * Set current instruction pointer to a custom place by jumping to it while
 * switching the stack pointer to a custom value beforehand, and passing a
 * single argument to the target function.
 *
 * The argument is placed in %rdi as per the System V AMD64 calling
 * convention, making it the first parameter of the function being
 * jumped to.
 *
 * This function does not return!
 *
 * @param sp  Stack pointer to switch to before jumping
 * @param ip  The place to jump to
 * @param arg The argument to pass to the target function
 */
static inline void __noreturn uk_arch_jump_to_with_arg(__u64 sp, __u64 ip,
						       __u64 arg)
{
	asm volatile (
		"movq	%0, %%rsp\n"

		/* According to System V AMD64 the stack pointer must be
		 * aligned to 16-bytes. In other words, the value (RSP+8) must
		 * be a multiple of 16 when control is transferred to the
		 * function entry point (i.e., the compiler expects a
		 * misalignment due to the return address having been pushed
		 * onto the stack).
		 */
		"andq	$~0xf, %%rsp\n"
		"subq	$0x8, %%rsp\n"

#if !__OMIT_FRAMEPOINTER__
		"xorq	%%rbp, %%rbp\n"
#endif /* __OMIT_FRAMEPOINTER__ */

		"jmp	*%1\n"
		:
		: "r" (sp), "r" (ip), "D" (arg)
		: /* clobbers not needed */);

	/* just make the compiler happy about noreturn function */
	__builtin_unreachable();
}

#endif /* !__ASSEMBLY__ */
#endif /* __UK_ARCH_UTIL_H__ */
