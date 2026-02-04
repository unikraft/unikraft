/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/atomic.h>
#include <uk/event.h>
#include <uk/lcpu.h>
#include <uk/pm.h>
#include <uk/print.h>

/**
 * TODO:
 * Note that at this moment only one set of system-wide power management
 * operations are allowed. In reality, most platforms allow for multiple
 * shutdown mechanism, so we should, when need arises, enhance the library
 * by adding the possibility of maintaining an internal, priority-based,
 * list of all registered operations from all drivers that are able to
 * perform such actions.
 */
static const struct uk_pm_ops *pm_ops;

int uk_pm_ops_register(const struct uk_pm_ops *ops)
{
	const struct uk_pm_ops *expected = __NULL;

	if (unlikely(!ops))
		return -EINVAL;

	/**
	 * Again, we do not support multiple ways to shut down even though it
	 * is common to have them available. To keep things deterministic, we
	 * throw an error if there are multiple drivers configured to provide
	 * this interface at the same time.
	 */
	if (unlikely(uk_compare_exchange_sync(&pm_ops, expected, ops) != ops)) {
		uk_pr_err("PM ops already registered (%p), ignoring new registration (%p)\n",
			  pm_ops, ops);
		return -EEXIST;
	}

	uk_pr_info("Registered PM ops: %p\n", ops);

	return 0;
}

UK_EVENT(UK_PM_EVENT_SYSHALT);

__noreturn __isr void uk_pm_syshalt(void)
{
	int rc;

	rc = uk_raise_event(UK_PM_EVENT_SYSHALT, __NULL);
	if (unlikely(rc < 0))
		uk_pr_err("Failed system halt event raising: %d\n", rc);

	/* Make sure no handler returns UK_EVENT_HANDLED as that
	 * would prevent propagating the event to the rest of
	 * the handlers.
	 */
	UK_ASSERT(rc != UK_EVENT_HANDLED);

	if (pm_ops && pm_ops->syshalt) {
		rc = pm_ops->syshalt();
		if (unlikely(rc))
			uk_pr_err("PM driver failed the operation: %d. Falling back on busy loop...\n",
				  rc);
	}

	_uk_pm_syshalt_fallback();
}

UK_EVENT(UK_PM_EVENT_SYSRESTART);

__noreturn __isr void uk_pm_sysrestart(void)
{
	int rc;

	rc = uk_raise_event(UK_PM_EVENT_SYSRESTART, __NULL);
	if (unlikely(rc < 0))
		uk_pr_err("Failed system restart event raising: %d\n", rc);

	UK_ASSERT(rc != UK_EVENT_HANDLED);

	if (pm_ops && pm_ops->sysrestart) {
		rc = pm_ops->sysrestart();
		if (unlikely(rc))
			uk_pr_err("PM driver failed the operation: %d. Falling back on busy loop...\n",
				  rc);
	}

	_uk_pm_syshalt_fallback();
}

UK_EVENT(UK_PM_EVENT_SYSSUSPEND);

__isr int uk_pm_syssuspend(void)
{
	int rc;

	rc = uk_raise_event(UK_PM_EVENT_SYSSUSPEND, __NULL);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed system suspend event raising: %d\n", rc);
		return rc;
	}

	UK_ASSERT(rc != UK_EVENT_HANDLED);

	if (!pm_ops || !pm_ops->syssuspend) {
		uk_pr_err("No suitable driver suspend operation available, falling back on busy loop...\n");
		_uk_pm_syshalt_fallback();
	}

	return pm_ops->syssuspend();
}

UK_EVENT(UK_PM_EVENT_SYSCRASH);

__noreturn __isr void uk_pm_syscrash(void)
{
	int rc;

	rc = uk_raise_event(UK_PM_EVENT_SYSCRASH, __NULL);
	if (unlikely(rc < 0))
		uk_pr_err("Failed system crash event raising: %d\n", rc);

	UK_ASSERT(rc != UK_EVENT_HANDLED);

	if (pm_ops && pm_ops->syscrash) {
		rc = pm_ops->syscrash();
		if (unlikely(rc))
			uk_pr_err("PM driver failed the operation: %d. Falling back on busy loop...\n",
				  rc);
	}

	_uk_pm_syshalt_fallback();
}

UK_EVENT(UK_PM_EVENT_SHUTDOWN_REQ);

__isr void uk_pm_raise_shutdown_event(enum uk_pm_shutdown_op op)
{
	int rc __maybe_unused;

	rc = uk_raise_event(UK_PM_EVENT_SHUTDOWN_REQ, (void *)op);

	UK_ASSERT(rc != UK_EVENT_HANDLED);
}

__noreturn __isr void uk_pm_shutdown(enum uk_pm_shutdown_op op)
{
	switch (op) {
	case UK_PM_SHUTDOWN_OP_SYSHALT:
		uk_pm_syshalt();
		__builtin_unreachable();
	case UK_PM_SHUTDOWN_OP_SYSRESTART:
		uk_pm_sysrestart();
		__builtin_unreachable();
	case UK_PM_SHUTDOWN_OP_SYSCRASH:
		__fallthrough;
	default:
		uk_pm_syscrash();
	}
}
