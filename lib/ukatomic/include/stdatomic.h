/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2011 Ed Schouten <ed@FreeBSD.org>
 *                    David Chisnall <theraven@FreeBSD.org>
 * All rights reserved.
 *
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
 */
/**
 * Taken and adapted from FreeBSD.
 * sys/sys/stdatomic.h
 *
 * Commit: 5076e9b5463afa47cf2e7689431d3dd33e5b15cb
 */

#ifndef _STDATOMIC_H_
#define _STDATOMIC_H_

#if __STDC_VERSION__ > 201710L
#define __STDC_VERSION_STDATOMIC_H__	202311L
#endif

#ifdef __clang__
#define	__CLANG_ATOMICS
#else
#define	__GNUC_ATOMICS
#endif

/*
 * 7.17.1 Atomic lock-free macros.
 */

#ifdef __GCC_ATOMIC_BOOL_LOCK_FREE
#define	ATOMIC_BOOL_LOCK_FREE		__GCC_ATOMIC_BOOL_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_CHAR_LOCK_FREE
#define	ATOMIC_CHAR_LOCK_FREE		__GCC_ATOMIC_CHAR_LOCK_FREE
#endif
#if __STDC_VERSION__ > 201710L && defined(__GCC_ATOMIC_CHAR8_T_LOCK_FREE)
#define	ATOMIC_CHAR8_T_LOCK_FREE	__GCC_ATOMIC_CHAR8_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define	ATOMIC_CHAR16_T_LOCK_FREE	__GCC_ATOMIC_CHAR16_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define	ATOMIC_CHAR32_T_LOCK_FREE	__GCC_ATOMIC_CHAR32_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define	ATOMIC_WCHAR_T_LOCK_FREE	__GCC_ATOMIC_WCHAR_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_SHORT_LOCK_FREE
#define	ATOMIC_SHORT_LOCK_FREE		__GCC_ATOMIC_SHORT_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_INT_LOCK_FREE
#define	ATOMIC_INT_LOCK_FREE		__GCC_ATOMIC_INT_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_LONG_LOCK_FREE
#define	ATOMIC_LONG_LOCK_FREE		__GCC_ATOMIC_LONG_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_LLONG_LOCK_FREE
#define	ATOMIC_LLONG_LOCK_FREE		__GCC_ATOMIC_LLONG_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_POINTER_LOCK_FREE
#define	ATOMIC_POINTER_LOCK_FREE	__GCC_ATOMIC_POINTER_LOCK_FREE
#endif

/*
 * 7.17.2 Initialization.
 */

#define	ATOMIC_VAR_INIT(value)		(value)

#if defined(__CLANG_ATOMICS)
#define	atomic_init(obj, value)		__c11_atomic_init(obj, value)
#else
#define	atomic_init(obj, value)		atomic_store_explicit(obj,	\
	    value, __ATOMIC_RELAXED)
#endif

/*
 * 7.17.3 Order and consistency.
 *
 * The memory_order_* constants that denote the barrier behaviour of the
 * atomic operations.
 */

typedef enum {
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

#define	kill_dependency(y)		(y)

/*
 * 7.17.4 Fences.
 */

static __inline void
atomic_thread_fence(memory_order __order)
{

#ifdef __CLANG_ATOMICS
	__c11_atomic_thread_fence(__order);
#else
	__atomic_thread_fence(__order);
#endif
}

static __inline void
atomic_signal_fence(memory_order __order)
{

#ifdef __CLANG_ATOMICS
	__c11_atomic_signal_fence(__order);
#else
	__atomic_signal_fence(__order);
#endif
}

/*
 * 7.17.5 Lock-free property.
 */

#define	atomic_is_lock_free(obj) \
	__atomic_is_lock_free(sizeof(*(obj)), obj)

/*
 * 7.17.6 Atomic integer types.
 */

typedef _Atomic(_Bool)			atomic_bool;
typedef _Atomic(char)			atomic_char;
typedef _Atomic(signed char)		atomic_schar;
typedef _Atomic(unsigned char)		atomic_uchar;
typedef _Atomic(short)			atomic_short;
typedef _Atomic(unsigned short)		atomic_ushort;
typedef _Atomic(int)			atomic_int;
typedef _Atomic(unsigned int)		atomic_uint;
typedef _Atomic(long)			atomic_long;
typedef _Atomic(unsigned long)		atomic_ulong;
typedef _Atomic(long long)		atomic_llong;
typedef _Atomic(unsigned long long)	atomic_ullong;
#if __STDC_VERSION__ > 201710L
typedef _Atomic(unsigned char)		atomic_char8_t;
#endif
typedef _Atomic(__CHAR16_TYPE__)	atomic_char16_t;
typedef _Atomic(__CHAR32_TYPE__)	atomic_char32_t;
typedef _Atomic(__WCHAR_TYPE__)		atomic_wchar_t;
typedef _Atomic(__INT_LEAST8_TYPE__)	atomic_int_least8_t;
typedef _Atomic(__UINT_LEAST8_TYPE__)	atomic_uint_least8_t;
typedef _Atomic(__INT_LEAST16_TYPE__)	atomic_int_least16_t;
typedef _Atomic(__UINT_LEAST16_TYPE__)	atomic_uint_least16_t;
typedef _Atomic(__INT_LEAST32_TYPE__)	atomic_int_least32_t;
typedef _Atomic(__UINT_LEAST32_TYPE__)	atomic_uint_least32_t;
typedef _Atomic(__INT_LEAST64_TYPE__)	atomic_int_least64_t;
typedef _Atomic(__UINT_LEAST64_TYPE__)	atomic_uint_least64_t;
typedef _Atomic(__INT_FAST8_TYPE__)	atomic_int_fast8_t;
typedef _Atomic(__UINT_FAST8_TYPE__)	atomic_uint_fast8_t;
typedef _Atomic(__INT_FAST16_TYPE__)	atomic_int_fast16_t;
typedef _Atomic(__UINT_FAST16_TYPE__)	atomic_uint_fast16_t;
typedef _Atomic(__INT_FAST32_TYPE__)	atomic_int_fast32_t;
typedef _Atomic(__UINT_FAST32_TYPE__)	atomic_uint_fast32_t;
typedef _Atomic(__INT_FAST64_TYPE__)	atomic_int_fast64_t;
typedef _Atomic(__UINT_FAST64_TYPE__)	atomic_uint_fast64_t;
typedef _Atomic(__INTPTR_TYPE__)	atomic_intptr_t;
typedef _Atomic(__UINTPTR_TYPE__)	atomic_uintptr_t;
typedef _Atomic(__SIZE_TYPE__)		atomic_size_t;
typedef _Atomic(__PTRDIFF_TYPE__)	atomic_ptrdiff_t;
typedef _Atomic(__INTMAX_TYPE__)	atomic_intmax_t;
typedef _Atomic(__UINTMAX_TYPE__)	atomic_uintmax_t;

/*
 * 7.17.7 Operations on atomic types.
 */

/*
 * Compiler-specific operations.
 */

#if defined(__CLANG_ATOMICS)
#define	atomic_store_explicit(object, desired, order)			\
	__c11_atomic_store(object, desired, order)
#define	atomic_load_explicit(object, order)				\
	__c11_atomic_load(object, order)
#define	atomic_exchange_explicit(object, desired, order)		\
	__c11_atomic_exchange(object, desired, order)
#define	atomic_compare_exchange_strong_explicit(object, expected,	\
    desired, success, failure)						\
	__c11_atomic_compare_exchange_strong(object, expected, desired,	\
	    success, failure)
#define	atomic_compare_exchange_weak_explicit(object, expected,		\
    desired, success, failure)						\
	__c11_atomic_compare_exchange_weak(object, expected, desired,	\
	    success, failure)
#define	atomic_fetch_add_explicit(object, operand, order)		\
	__c11_atomic_fetch_add(object, operand, order)
#define	atomic_fetch_sub_explicit(object, operand, order)		\
	__c11_atomic_fetch_sub(object, operand, order)
#define	atomic_fetch_or_explicit(object, operand, order)		\
	__c11_atomic_fetch_or(object, operand, order)
#define	atomic_fetch_xor_explicit(object, operand, order)		\
	__c11_atomic_fetch_xor(object, operand, order)
#define	atomic_fetch_and_explicit(object, operand, order)		\
	__c11_atomic_fetch_and(object, operand, order)
#else
#define	__atomic_apply_stride(object, operand)				\
	(((__typeof__(*(object)))0) + (operand))
#define	atomic_store_explicit(object, desired, order)			\
	__atomic_store_n(object, desired, order)
#define	atomic_load_explicit(object, order)				\
	__atomic_load_n(object, order)
#define	atomic_exchange_explicit(object, desired, order)		\
	__atomic_exchange_n(object, desired, order)
#define	atomic_compare_exchange_strong_explicit(object, expected,	\
    desired, success, failure)						\
	__atomic_compare_exchange_n(object, expected,			\
	    desired, 0, success, failure)
#define	atomic_compare_exchange_weak_explicit(object, expected,		\
    desired, success, failure)						\
	__atomic_compare_exchange_n(object, expected,			\
	    desired, 1, success, failure)
#define	atomic_fetch_add_explicit(object, operand, order)		\
	__atomic_fetch_add(object,					\
	    __atomic_apply_stride(object, operand), order)
#define	atomic_fetch_sub_explicit(object, operand, order)		\
	__atomic_fetch_sub(object,					\
	    __atomic_apply_stride(object, operand), order)
#define	atomic_fetch_or_explicit(object, operand, order)		\
	__atomic_fetch_or(object, operand, order)
#define	atomic_fetch_xor_explicit(object, operand, order)		\
	__atomic_fetch_xor(object, operand, order)
#define	atomic_fetch_and_explicit(object, operand, order)		\
	__atomic_fetch_and(object, operand, order)
#endif

/*
 * Convenience functions.
 */

#define	atomic_store(object, desired)					\
	atomic_store_explicit(object, desired, memory_order_seq_cst)
#define	atomic_load(object)						\
	atomic_load_explicit(object, memory_order_seq_cst)
#define	atomic_exchange(object, desired)				\
	atomic_exchange_explicit(object, desired, memory_order_seq_cst)
#define	atomic_compare_exchange_strong(object, expected, desired)	\
	atomic_compare_exchange_strong_explicit(object, expected,	\
	    desired, memory_order_seq_cst, memory_order_seq_cst)
#define	atomic_compare_exchange_weak(object, expected, desired)		\
	atomic_compare_exchange_weak_explicit(object, expected,		\
	    desired, memory_order_seq_cst, memory_order_seq_cst)
#define	atomic_fetch_add(object, operand)				\
	atomic_fetch_add_explicit(object, operand, memory_order_seq_cst)
#define	atomic_fetch_sub(object, operand)				\
	atomic_fetch_sub_explicit(object, operand, memory_order_seq_cst)
#define	atomic_fetch_or(object, operand)				\
	atomic_fetch_or_explicit(object, operand, memory_order_seq_cst)
#define	atomic_fetch_xor(object, operand)				\
	atomic_fetch_xor_explicit(object, operand, memory_order_seq_cst)
#define	atomic_fetch_and(object, operand)				\
	atomic_fetch_and_explicit(object, operand, memory_order_seq_cst)

/*
 * 7.17.8 Atomic flag type and operations.
 */

typedef struct {
#if ATOMIC_BOOL_LOCK_FREE == 2
	atomic_bool	__flag;
#elif ATOMIC_CHAR_LOCK_FREE == 2
	atomic_uchar	__flag;
#else
#error "atomic_flag is required to be lock-free"
#endif
} atomic_flag;
#define	ATOMIC_FLAG_INIT		{ 0 }

static __inline _Bool
atomic_flag_test_and_set_explicit(volatile atomic_flag *__object,
    memory_order __order)
{

	return (atomic_exchange_explicit(&__object->__flag, 1, __order) != 0);
}

static __inline void
atomic_flag_clear_explicit(volatile atomic_flag *__object, memory_order __order)
{

	atomic_store_explicit(&__object->__flag, 0, __order);
}

static __inline _Bool
atomic_flag_test_and_set(volatile atomic_flag *__object)
{

	return (atomic_flag_test_and_set_explicit(__object,
	    memory_order_seq_cst));
}

static __inline void
atomic_flag_clear(volatile atomic_flag *__object)
{

	atomic_flag_clear_explicit(__object, memory_order_seq_cst);
}

#endif /* !_STDATOMIC_H_ */
