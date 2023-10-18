/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/arch/atomic.h>
#include <uk/boot.h>
#include "shutdown_req.h"

struct uk_boot_shutdown_ctl shutdown_ctl;

int uk_boot_shutdown_req(enum ukplat_gstate target)
{
	unsigned long already_requested;

	switch (target) {
	case UKPLAT_HALT:
		__fallthrough;
	case UKPLAT_RESTART:
		__fallthrough;
	case UKPLAT_CRASH:
		break;
	default:
		/* not a normal shutdown request */
		return -EINVAL;
	}

	already_requested = ukarch_exchange_n(
					&shutdown_ctl.request.already_requested,
					0x1);
	if (already_requested) {
		uk_pr_debug("Shutdown already in progress\n");
		return 1;
	}

	/* The first request sets the shutdown target and unblocks "init" */
	uk_pr_info("Shutdown requested (%d)\n", target);
	shutdown_ctl.request.target = target;
	uk_semaphore_up(&shutdown_ctl.barrier);
	return 0;
}
