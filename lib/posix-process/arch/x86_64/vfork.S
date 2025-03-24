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
 * special wrapper that still allows our vfork functionality to still work
 * correctly.
 */
#if UK_LIBC_SYSCALLS
.global vfork
vfork:
	jmp	uk_syscall_e_vfork
#endif /* UK_LIBC_SYSCALLS */
