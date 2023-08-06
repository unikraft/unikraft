/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/console.h>
#include <uk/print.h>

struct uk_console_ops *uk_console_ops;
extern struct uk_console_ops uk_console_kvm_ops;

void uk_console_init(struct ukplat_bootinfo *bi)
{
	uk_console_ops = &uk_console_kvm_ops;

	/* Initialize the corresponding console */
	uk_console_ops->init(bi);

	uk_pr_info("Console init finished\n");
}
