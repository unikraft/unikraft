/* SPDX-License-Identifier: ISC */
/*
 * Authors: Jia He <justin.he@arm.com>
 *
 * Copyright (c) 2020 Arm Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <uk/plat/common/cpu.h>

void fpsimd_save_state(uintptr_t ptr)
{
	__u32 fpcr, fpsr;

	__asm__ __volatile__(
		"mrs	%0, fpcr\n"
		"mrs	%1, fpsr\n"
		"stp	q0,  q1,  [%2, #16 *  0]\n"
		"stp	q2,  q3,  [%2, #16 *  2]\n"
		"stp	q4,  q5,  [%2, #16 *  4]\n"
		"stp	q6,  q7,  [%2, #16 *  6]\n"
		"stp	q8,  q9,  [%2, #16 *  8]\n"
		"stp	q10, q11, [%2, #16 * 10]\n"
		"stp	q12, q13, [%2, #16 * 12]\n"
		"stp	q14, q15, [%2, #16 * 14]\n"
		"stp	q16, q17, [%2, #16 * 16]\n"
		"stp	q18, q19, [%2, #16 * 18]\n"
		"stp	q20, q21, [%2, #16 * 20]\n"
		"stp	q22, q23, [%2, #16 * 22]\n"
		"stp	q24, q25, [%2, #16 * 24]\n"
		"stp	q26, q27, [%2, #16 * 26]\n"
		"stp	q28, q29, [%2, #16 * 28]\n"
		"stp	q30, q31, [%2, #16 * 30]\n"
		: "=&r"(fpcr), "=&r"(fpsr) : "r"(ptr));

	((struct fpsimd_state *)ptr)->fpcr = fpcr;
	((struct fpsimd_state *)ptr)->fpsr = fpsr;
}

void fpsimd_restore_state(uintptr_t ptr)
{
	__u32 fpcr, fpsr;

	fpcr = ((struct fpsimd_state *)ptr)->fpcr;
	fpsr = ((struct fpsimd_state *)ptr)->fpsr;

	__asm__ __volatile__(
		"ldp	q0,  q1,  [%2, #16 *  0]\n"
		"ldp	q2,  q3,  [%2, #16 *  2]\n"
		"ldp	q4,  q5,  [%2, #16 *  4]\n"
		"ldp	q6,  q7,  [%2, #16 *  6]\n"
		"ldp	q8,  q9,  [%2, #16 *  8]\n"
		"ldp	q10, q11, [%2, #16 * 10]\n"
		"ldp	q12, q13, [%2, #16 * 12]\n"
		"ldp	q14, q15, [%2, #16 * 14]\n"
		"ldp	q16, q17, [%2, #16 * 16]\n"
		"ldp	q18, q19, [%2, #16 * 18]\n"
		"ldp	q20, q21, [%2, #16 * 20]\n"
		"ldp	q22, q23, [%2, #16 * 22]\n"
		"ldp	q24, q25, [%2, #16 * 24]\n"
		"ldp	q26, q27, [%2, #16 * 26]\n"
		"ldp	q28, q29, [%2, #16 * 28]\n"
		"ldp	q30, q31, [%2, #16 * 30]\n"
		"msr	fpcr, %0\n"
		"msr	fpsr, %1\n"
		: : "r"(fpcr), "r"(fpsr), "r"(ptr));
}
