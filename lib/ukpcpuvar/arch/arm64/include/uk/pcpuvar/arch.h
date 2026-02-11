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
 * Read per-CPU variable into register.
 *
 * @dst: Destination register (will contain the value)
 * @sym: Symbol name
 * @tmp: Scratch register
 */
.macro uk_pcpuvar_arm64_ldr dst:req, sym:req, tmp:req
	adrp	\tmp, \sym
	add	\tmp, \tmp, #:lo12:\sym
	adrp	\dst, _uk_pcpuvar_base
	add	\dst, \dst, #:lo12:_uk_pcpuvar_base
	sub	\tmp, \tmp, \dst
	mrs	\dst, tpidr_el1
	ldr	\dst, [\dst, \tmp]
.endm
#else /* !__ASSEMBLY__ */
#define uk_pcpuvar_arm64_ldr(_dst, _sym, _tmp)				\
	"adrp	" _tmp ", " STRINGIFY(_sym) "\n\t"			\
	"add	" _tmp ", " _tmp ", #:lo12:" STRINGIFY(_sym) "\n\t"	\
	"adrp	" _dst ", _uk_pcpuvar_base\n\t"				\
	"add	" _dst ", " _dst ", #:lo12:_uk_pcpuvar_base\n\t"	\
	"sub	" _tmp ", " _tmp ", " _dst "\n\t"			\
	"mrs	" _dst ", tpidr_el1\n\t"				\
	"ldr	" _dst ", [" _dst ", " _tmp "]"
#endif /* !__ASSEMBLY__ */

#if __ASSEMBLY__
/**
 * Write register value to per-CPU variable.
 *
 * @src: Source register (value to write)
 * @sym: Symbol name
 * @tmp1: First scratch register
 * @tmp2: Second scratch register
 */
.macro uk_pcpuvar_arm64_str src:req, sym:req, tmp1:req, tmp2:req
	adrp	\tmp1, \sym
	add	\tmp1, \tmp1, #:lo12:\sym
	adrp	\tmp2, _uk_pcpuvar_base
	add	\tmp2, \tmp2, #:lo12:_uk_pcpuvar_base
	sub	\tmp1, \tmp1, \tmp2
	mrs	\tmp2, tpidr_el1
	str	\src, [\tmp2, \tmp1]
.endm
#else /* !__ASSEMBLY__ */
#define uk_pcpuvar_arm64_str(_src, _sym, _tmp1, _tmp2)			\
	"adrp	" _tmp1 ", " STRINGIFY(_sym) "\n\t"			\
	"add	" _tmp1 ", " _tmp1 ", #:lo12:" STRINGIFY(_sym) "\n\t"	\
	"adrp	" _tmp2 ", _uk_pcpuvar_base\n\t"			\
	"add	" _tmp2 ", " _tmp2 ", #:lo12:_uk_pcpuvar_base\n\t"	\
	"sub	" _tmp1 ", " _tmp1 ", " _tmp2 "\n\t"			\
	"mrs	" _tmp2 ", tpidr_el1\n\t"				\
	"str	" _src ", [" _tmp2 ", " _tmp1 "]"
#endif /* !__ASSEMBLY__ */

#if __ASSEMBLY__
/**
 * Get the value of the TPIDR_EL1 register of CPU of given index and store it
 * in the destination register.
 *
 * The literal pool form `ldr dest, =_uk_pcpuvar_tmpl_size` is used to
 * load the size. Because _uk_pcpuvar_tmpl_size is defined with ABSOLUTE()
 * in the linker script it carries section index SHN_ABS, which causes the
 * assembler/linker to resolve the literal pool entry to a plain integer
 * constant rather than emitting an R_AARCH64_ABS64 relocation for it.
 * The result is identical to having written the numeric value inline:
 * zero relocation entries, safe to execute before runtime relocations
 * have been applied.
 *
 * @param dest
 *   The destination 64-bit register
 * @param idx
 *   The 64-bit register holding the index of the CPU whose TPIDR_EL1
 *   value to compute
 */
.macro uk_pcpuvar_arm64_tpidrval dest:req, idx:req
	ldr	\dest, =_uk_pcpuvar_tmpl_size
	mul	\dest, \dest, \idx
.endm
#endif /* __ASSEMBLY__ */

#if !__ASSEMBLY__
/**
 * Read current CPU's copy of a per-CPU variable using TPIDR_EL1.
 *
 * @param _sym
 *   Symbol name
 * @return
 *   Value of the variable
 */
#define __uk_pcpuvar_arch_current_get(_sym)				\
	({								\
		__u64 _offset = (__u64)&(_sym) - (__u64)_uk_pcpuvar_base; \
		__typeof__(_sym) _val;					\
									\
		asm volatile (						\
			"mrs	%x0, tpidr_el1\n\t"			\
			"ldr	%0, [%x0, %1]"				\
			: "=&r" (_val)					\
			: "r" (_offset)					\
			: "memory"					\
		);							\
		_val;							\
	})

/**
 * Write to current CPU's copy of a per-CPU variable using TPIDR_EL1.
 *
 * @param _sym
 *   Symbol name
 * @param _val
 *   Value to write
 */
#define __uk_pcpuvar_arch_current_set(_sym, _val)			\
	do {								\
		__u64 _offset = (__u64)&(_sym) - (__u64)_uk_pcpuvar_base; \
		__typeof__(_sym) _tmp = (_val);				\
		__u64 _tpidr;						\
									\
		asm volatile (						\
			"mrs	%0, tpidr_el1\n\t"			\
			"str	%1, [%0, %2]"				\
			: "=&r" (_tpidr)				\
			: "r" (_tmp), "r" (_offset)			\
			: "memory"					\
		);							\
	} while (0)

/**
 * Read a struct member from current CPU's copy using TPIDR_EL1.
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
		__u64 _offset = (__u64)&(_sym) - (__u64)_uk_pcpuvar_base + \
				__offsetof(__typeof__(_sym), _member);	\
		__typeof__(((__typeof__(_sym) *)0)->_member) _val;	\
									\
		asm volatile (						\
			"mrs	%x0, tpidr_el1\n\t"			\
			"ldr	%0, [%x0, %1]"				\
			: "=&r" (_val)					\
			: "r" (_offset)					\
			: "memory"					\
		);							\
		_val;							\
	})

/**
 * Write to a struct member of current CPU's copy using TPIDR_EL1.
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
		__typeof__(((__typeof__(_sym) *)0)->_member) _tmp = (_val); \
		__u64 _offset = (__u64)&(_sym) - (__u64)_uk_pcpuvar_base + \
				__offsetof(__typeof__(_sym), _member);	\
		__u64 _tpidr;						\
									\
		asm volatile (						\
			"mrs	%0, tpidr_el1\n\t"			\
			"str	%1, [%0, %2]"				\
			: "=&r" (_tpidr)				\
			: "r" (_tmp), "r" (_offset)			\
			: "memory"					\
		);							\
	} while (0)

/**
 * Get pointer to current CPU's copy of a per-CPU variable using TPIDR_EL1.
 *
 * @param _sym
 *   Symbol name
 * @return
 *   Pointer to the variable
 */
#define __uk_pcpuvar_arch_current_ptr_get(_sym)				\
	({								\
		__typeof__(_sym) *_ptr;					\
		__u64 _offset = (__u64)&(_sym) - (__u64)_uk_pcpuvar_base; \
									\
		asm volatile (						\
			"mrs	%0, tpidr_el1\n\t"			\
			"add	%0, %0, %1"				\
			: "=&r" (_ptr)					\
			: "r" (_offset)					\
			: "memory"					\
		);							\
		_ptr;							\
	})

#endif /* !__ASSEMBLY__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
