/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PCPUVAR_H__
#error "Do not include this header directly"
#endif

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __ASSEMBLY__
/**
 * GS segment relative read of per-CPU variable.
 *
 * @param _sym
 *   Symbol name
 */
#define __UK_PCPUVAR_X86_64_GSREL(_sym)					\
	%gs:(_sym)(%rip)
#define _UK_PCPUVAR_X86_64_GSREL(_sym)					\
	__UK_PCPUVAR_X86_64_GSREL(_sym)
#define UK_PCPUVAR_X86_64_GSREL(_sym)					\
	_UK_PCPUVAR_X86_64_GSREL(_sym)

#else /* !__ASSEMBLY__ */
#define __UK_PCPUVAR_X86_64_GSREL(_sym)					\
	"%%gs:" STRINGIFY(_sym) "(%%rip)"
#define _UK_PCPUVAR_X86_64_GSREL(_sym)					\
	__UK_PCPUVAR_X86_64_GSREL(_sym)
#define UK_PCPUVAR_X86_64_GSREL(_sym)					\
	_UK_PCPUVAR_X86_64_GSREL(_sym)

/* To be used in global scope inline assembly... because that requires a
 * single %-letter...
 */
#define __UK_PCPUVAR_X86_64_GSREL_GLOBAL(_sym)				\
	"%gs:" STRINGIFY(_sym) "(%rip)"
#define _UK_PCPUVAR_X86_64_GSREL_GLOBAL(_sym)				\
	__UK_PCPUVAR_X86_64_GSREL_GLOBAL(_sym)
#define UK_PCPUVAR_X86_64_GSREL_GLOBAL(_sym)				\
	_UK_PCPUVAR_X86_64_GSREL_GLOBAL(_sym)
#endif /* !__ASSEMBLY__ */

#if __ASSEMBLY__
/**
 * Get the value of the gsbase register of CPU of given index and store it
 * in the destination register.
 *
 * movabsq is used deliberately instead of a plain mov with a 32-bit
 * immediate. A 32-bit immediate form (mov $imm32, %reg) would cause the
 * assembler to emit an R_X86_64_32S relocation when the symbol's value
 * is not yet known, as is the case for a section-relative symbol. That
 * relocation requires sign-extension and must be resolved at load time,
 * making it unusable in early boot code that runs before relocation fixup.
 * movabsq encodes a full 64-bit immediate operand: because
 * _uk_pcpuvar_tmpl_size is defined with ABSOLUTE() in the linker script
 * it carries section index SHN_ABS, so the assembler resolves the operand
 * to its absolute integer value at link time and emits no relocation entry
 * at all. The instruction is therefore self-contained and safe to execute
 * at any point during boot, including before the page table or relocator
 * has run.
 *
 * @param dest
 *   The destination 64-bit register
 * @param idx
 *   The 64-bit register holding the index of the CPU whose gsbase value to get
 */
.macro uk_pcpuvar_x86_64_gsval dest:req, idx:req
	movabsq	$_uk_pcpuvar_tmpl_size, \dest
	imulq	\idx, \dest
.endm
#endif /* __ASSEMBLY__ */

#if !__ASSEMBLY__
/**
 * Read current CPU's copy of a per-CPU variable using GS segment.
 *
 * @param _sym
 *   Symbol name
 * @return
 *   Value of the variable
 */
#define __uk_pcpuvar_arch_current_get(_sym)				\
	({								\
		__typeof__(_sym) _##_sym##_val;				\
									\
		asm volatile (						\
			"mov	" UK_PCPUVAR_X86_64_GSREL(_sym) ", %0"	\
			: "=r" (_##_sym##_val)				\
			:						\
			: "memory"					\
		);							\
		_##_sym##_val;						\
	})

/**
 * Write to current CPU's copy of a per-CPU variable using GS segment.
 *
 * @param _sym
 *   Symbol name
 * @param _val
 *   Value to write
 */
#define __uk_pcpuvar_arch_current_set(_sym, _val)			\
	do {								\
		__typeof__(_sym) _##_sym##_tmp = (_val);		\
									\
		asm volatile (						\
			"mov	%0, " UK_PCPUVAR_X86_64_GSREL(_sym)	\
			:						\
			: "r" (_##_sym##_tmp)				\
			: "memory"					\
		);							\
	} while (0)

/**
 * Read a struct member from current CPU's copy using GS.
 *
 * @param _sym
 *   Symbol name
 * @param _member
 *   Member name
 * @return
 *   Value of the member
 */
#define __uk_pcpuvar_arch_current_member_get(_sym, _member)		\
	({								\
		__typeof__((((__typeof__(_sym) *)0)->_member)) _val;	\
									\
		asm volatile (						\
			"mov	%%gs:" STRINGIFY(_sym) "+%c1(%%rip), %0"\
			: "=r" (_val)					\
			: "i" (__offsetof(__typeof__(_sym), _member))	\
			: "memory"					\
		);							\
		_val;							\
	})

/**
 * Write to a struct member of current CPU's copy using GS.
 *
 * @param _sym
 *   Symbol name
 * @param _member
 *   Member name
 * @param _val
 *   Value to write
 */
#define __uk_pcpuvar_arch_current_member_set(_sym, _member, _val)	\
	do {								\
		__typeof__((((__typeof__(_sym) *)0)->_member))		\
			_tmp = (_val);					\
		asm volatile (						\
			"mov	%0, %%gs:" STRINGIFY(_sym) "+%c1(%%rip)"\
			:						\
			: "r" (_tmp),					\
			  "i" (__offsetof(__typeof__(_sym), _member))	\
			: "memory"					\
		);							\
	} while (0)

/**
 * Get pointer to current CPU's copy of a per-CPU variable.
 *
 * @param _sym
 *   Symbol name
 * @return
 *   Pointer to the variable
 */
#if __FSGSBASE__
#define __uk_pcpuvar_arch_current_ptr_get(_sym)				\
	({								\
		__typeof__(_sym) *_ptr;					\
		__u64 _symaddr;						\
									\
		asm volatile (						\
			"rdgsbase %0\n\t"				\
			"lea	" STRINGIFY(_sym) "(%%rip), %1\n\t"	\
			"add	%1, %0"					\
			: "=&r" (_ptr), "=&r" (_symaddr)		\
			:						\
			: "memory"					\
		);							\
		_ptr;							\
	})
#else /* !__FSGSBASE__ */
/* Fallback for when CR4.FSGSBASE not supported */
#define __uk_pcpuvar_arch_current_ptr_get(_sym)				\
	({								\
		__typeof__(_sym) *_ptr;					\
		__u64 _idx;						\
									\
		asm volatile (						\
			/* slot_offset = tmpl_size * cpu_idx (no rdgsbase) */ \
			"movabsq	$_uk_pcpuvar_tmpl_size, %0\n\t"	\
			"imulq	"					\
				UK_PCPUVAR_X86_64_GSREL(uk_pcpuvar_cpu_idx)\
				", %0\n\t"				\
			/* Add template address of _sym */		\
			"lea	" STRINGIFY(_sym) "(%%rip), %1\n\t"	\
			"add	%1, %0\n\t"				\
			: "=&r" (_ptr), "=&r" (_idx)			\
			:						\
			: "memory"					\
		);							\
		_ptr;							\
	})
#endif /* !__FSGSBASE__ */

#endif /* !__ASSEMBLY__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
