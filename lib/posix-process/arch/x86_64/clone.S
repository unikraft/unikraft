/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/syscall.h>

/*
 * NOTE: We need to replicate this assembly libc wrapper in our
 * supported/ported external libc libraries (e.g. musl) because their
 * wrappers do not typically tailor to our use case of having system calls
 * regular function calls.
 *
 * Whenever porting a new libc library, make sure to replicate this code there
 * as well, unless the library in question does somehow indeed have a more
 * special wrapper that still allows our clone functionality to still work
 * correctly.
 */
#if UK_LIBC_SYSCALLS
.global clone
clone:
	/* Save fn and arg in callee-saved registers, but first make sure
	 * we can also restore them before returning in the parent.
	 */
	pushq	%rbx
	pushq	%rbp
	movq	%rdi, %rbx
	movq	%rcx, %rbp

	/* Now reorder arguments */
	/* flags */
	movq	%rdx, %rdi

	/* stack is already in rsi */

	/* parent_tid */
	movq	%r8, %rdx

	/* child_tid: this is passed on the stack as it does not fit
	 * in the registers used for the arguments
	 *
	 * NOTE: Although this runs on native builds (syscall == function call),
	 * we mustn't use rcx for the 4th argument like for a regular function
	 * call but instead we will have to use r10 as is supposed to happen
	 * for system calls. This is because `clone` is registered as an
	 * execenv system call and execenv registrations assume that the
	 * 4th argument is that according to the syscall calling convention.
	 */
	movq	24(%rsp), %r10

	/* tls */
	movq	%r9, %r8

	call	uk_syscall_e_clone

	/* Let the parent just return */
	test	%eax, %eax
	jnz	1f

	/* The child should now just call the saved fn(arg) */
	movq	%rbp, %rdi

	/* Mark the outermost frame (avoid unwinding beyond this) */
	xorl	%ebp, %ebp
	call	*%rbx

	/* If managed to return from fn(arg) we must call exit(rax) */
	movq	%rax, %rdi
	call	uk_syscall_e_exit

0:
	/* Put a hlt safeguard here, just in care, cus exit is noreturn */
	hlt
	jmp	0b
1:
	/* Restore the registers saved in the beginning */
	popq	%rbp
	popq	%rbx
	ret
#endif /* UK_LIBC_SYSCALLS */
