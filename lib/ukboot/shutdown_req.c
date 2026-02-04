/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/atomic.h>
#include <uk/pm.h>
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
int uk_boot_shutdown_req(enum uk_pm_shutdown_op target)
#else /* __INTERRUPTSAFE__ */
int uk_boot_shutdown_req_isr(enum uk_pm_shutdown_op target)
#endif /* __INTERRUPTSAFE__ */
{
	unsigned long already_requested;

	switch (target) {
	case UK_PM_SHUTDOWN_OP_SYSHALT:
		__fallthrough;
	case UK_PM_SHUTDOWN_OP_SYSRESTART:
		__fallthrough;
	case UK_PM_SHUTDOWN_OP_SYSCRASH:
		break;
	default:
		/* not a normal shutdown request */
		return -EINVAL;
	}

	already_requested = uk_exchange_n(
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
	enum uk_pm_shutdown_op target = (enum uk_pm_shutdown_op)data;
	int rc;

	rc = uk_boot_shutdown_req_isr(target);
	if (unlikely(rc < 0))
		return UK_EVENT_NOT_HANDLED;
	return UK_EVENT_HANDLED_CONT;
}

UK_EVENT_HANDLER(UK_PM_EVENT_SHUTDOWN_REQ, shutdown_req_handler);

#endif /* CONFIG_LIBUKBOOT_SHUTDOWNREQ_HANDLER && __INTERRUPTSAFE__ */
