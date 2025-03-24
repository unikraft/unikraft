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
	stp	x20, x21, [sp, #-16]
	mov	x20, x0
	mov	x21, x3

	/* Now reorder arguments */

	/* flags */
	mov	x0, x2

	/* stack is already in x1 */

	/* parent_tid */
	mov	x2, x4

	/* tls */
	mov	x3, x5

	/* child_tid */
	mov	x4, x6

	bl	uk_syscall_e_clone

	/* Let the parent just return */
	cbnz	x0, 1f

	/* Mark the outermost frame (avoid unwinding beyond this) */
	mov	x29, xzr

	/* The child should now just call the saved fn(arg) */
	mov	x0, x21
	blr	x20

	/* If managed to return from fn(arg) we must call exit(x0) */
	bl	uk_syscall_e_exit

0:
	/* Put a wfi safeguard here, just in care, cus exit is noreturn */
	wfi
	b	0b

1:
	/* Restore the registers saved in the beginning */
	ldp	x20, x21, [sp, #-16]
	ret
#endif /* UK_LIBC_SYSCALLS */
