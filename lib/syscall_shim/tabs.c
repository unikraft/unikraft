/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/syscall.h>

__thread unsigned long uk_syscall_nested_depth;

void _uk_syscall_wrapper_do_entertab(struct ukarch_execenv *execenv)
{
	struct uk_syscall_enter_ctx enter_ctx;

	uk_syscall_nested_depth++;

	uk_syscall_enter_ctx_init(&enter_ctx,
				  execenv,
				  uk_syscall_nested_depth,
				  0);
	uk_syscall_entertab_run(&enter_ctx);
}

void _uk_syscall_wrapper_do_exittab(struct ukarch_execenv *execenv)
{
	struct uk_syscall_exit_ctx exit_ctx;

	uk_syscall_exit_ctx_init(&exit_ctx,
				 execenv,
				 uk_syscall_nested_depth,
				 0);
	uk_syscall_exittab_run(&exit_ctx);

	uk_syscall_nested_depth--;
}
