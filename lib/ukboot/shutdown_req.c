/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/arch/atomic.h>
#if !__INTERRUPTSAFE__
#include <uk/boot.h>
#else /* __INTERRUPTSAFE__ */
#include <uk/isr/boot.h>
#include <uk/isr/semaphore.h>
#include <uk/event.h>
#endif /* __INTERRUPTSAFE__ */
#include "shutdown_req.h"

#if !__INTERRUPTSAFE__
struct uk_boot_shutdown_ctl shutdown_ctl;
#endif /* !__INTERRUPTSAFE__ */

#if !__INTERRUPTSAFE__
int uk_boot_shutdown_req(enum ukplat_gstate target)
#else /* __INTERRUPTSAFE__ */
int uk_boot_shutdown_req_isr(enum ukplat_gstate target)
#endif /* __INTERRUPTSAFE__ */
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
#if !__INTERRUPTSAFE__
	uk_semaphore_up(&shutdown_ctl.barrier);
#else /* __INTERRUPTSAFE__ */
	uk_semaphore_up_isr(&shutdown_ctl.barrier);
#endif /* __INTERRUPTSAFE__ */
	return 0;
}

#if __INTERRUPTSAFE__ && CONFIG_LIBUKBOOT_SHUTDOWNREQ_HANDLER
static int shutdown_req_handler(void *data)
{
	enum ukplat_gstate request = (enum ukplat_gstate)data;
	int rc;

	rc = uk_boot_shutdown_req_isr(request);
	if (unlikely(rc < 0))
		return UK_EVENT_NOT_HANDLED;
	return UK_EVENT_HANDLED_CONT;
}
UK_EVENT_HANDLER(UKPLAT_SHUTDOWN_EVENT, shutdown_req_handler);

#endif /* CONFIG_LIBUKBOOT_SHUTDOWNREQ_HANDLER && __INTERRUPTSAFE__ */
