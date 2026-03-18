/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PM_H__
#define __UK_PM_H__

#include <uk/arch.h>
#include <uk/arch/types.h>
#include <errno.h>
#include <uk/essentials.h>

#if CONFIG_LIBUKLCPU
#include <uk/lcpu.h>
#endif /* !CONFIG_LIBUKLCPU */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

enum uk_pm_shutdown_op {
	UK_PM_SHUTDOWN_OP_SYSHALT,
	UK_PM_SHUTDOWN_OP_SYSRESTART,
	UK_PM_SHUTDOWN_OP_SYSCRASH,
};

#if CONFIG_LIBUKLCPU
__noreturn __isr static inline void _uk_pm_syshalt_fallback(void)
{
	/**
	 * TODO: Should probably do this across all CPUs, but we don't
	 * implement that yet.
	 */
	uk_lcpu_disable_irq();
	uk_lcpu_halt();
}
#else /* !CONFIG_LIBUKLCPU */
__noreturn __isr static inline void _uk_pm_syshalt_fallback(void)
{
	for (;;)
		uk_arch_spinwait();
}
#endif /* !CONFIG_LIBUKLCPU */

#if CONFIG_LIBUKPM
/**
 * Function pointer type for a system halt handler.
 *
 * @return <0 error, since on success it should not return.
 */
typedef int (*uk_syshalt_func)(void);

/**
 * Function pointer type for a system restart handler.
 *
 * @return <0 error, since on success it should not return.
 */
typedef int (*uk_sysrestart_func)(void);

/**
 * Function pointer type for a system suspend handler.
 * The handler must be ISR-safe.
 *
 * @return 0 after guest suspend ended, <0 on errors
 */
typedef int (*uk_syssuspend_func)(void);

/**
 * Function pointer type for a system crash handler.
 *
 * @return <0 error, since on success it should not return.
 */
typedef int (*uk_syscrash_func)(void);

/**
 * Power management operations provided by a platform or driver.
 * Register an instance of this struct via uk_pm_ops_register().
 *
 * NOTE: Except for the suspend handler, all handlers must be __isr and
 * should never return on success, but on failure to run their mechanism, they
 * should always return an error. The handler must be written as able to be
 * returned from, despite their successful nature implying otherwise. This
 * is so the library's fallbacks can execute safely in case of error, instead
 * of having an undefined behavior upon return.
 */
struct uk_pm_ops {
	/**
	 * Halt the system.
	 * Does not return on success, or returns <0 otherwise.
	 */
	uk_syshalt_func syshalt;
	/**
	 * Restart the system.
	 * Does not return on success, or returns <0 otherwise.
	 */
	uk_sysrestart_func sysrestart;
	/** Suspend the system. Returns 0 on resume, <0 on errors. */
	uk_syssuspend_func syssuspend;
	/**
	 * Halt the system signalling a crash.
	 * Does not return on success, or returns <0 otherwise.
	 */
	uk_syscrash_func syscrash;
};

/**
 * Register power management operations handlers.
 *
 * @param ops Pointer to the operations struct
 * @return 0 on success, <0 on errors
 */
int uk_pm_ops_register(const struct uk_pm_ops *ops) __isr;

/**
 * NOTE:
 * The event handlers should never return UK_EVENT_HANDLED as that
 * would prevent propagating the event to the rest of the handlers.
 * What is more, all handlers must be __isr.
 */
#define UK_PM_EVENT_SYSHALT			uk_pm_event_syshalt
#define UK_PM_EVENT_SYSRESTART			uk_pm_event_sysrestart
#define UK_PM_EVENT_SYSSUSPEND			uk_pm_event_syssuspend
#define UK_PM_EVENT_SYSCRASH			uk_pm_event_syscrash

#define UK_PM_EVENT_SHUTDOWN_REQ		uk_pm_event_shutdown_req

#define UK_PM_EVENT_SYSMIGRATION		uk_pm_event_sysmigration

/**
 * Halts system.
 */
void uk_pm_syshalt(void) __noreturn __isr;

/**
 * Restarts system.
 */
void uk_pm_sysrestart(void) __noreturn __isr;

/**
 * Suspends system.
 * @return 0 after guest suspend ended, <0 on errors
 */
int uk_pm_syssuspend(void) __isr;

/**
 * Halts system with signalling a crash
 */
void uk_pm_syscrash(void) __noreturn __isr;

/**
 * Raise the shutdown request event specifying the kind of shutdown that was
 * requested. This does not raise the specific requested shutdown operation's
 * event.
 *
 * @param op Specify the shutdown operation
 *	     Invalid operations are mapped to UK_PM_SHUTDOWN_OP_SYSCRASH
 */
void uk_pm_raise_shutdown_event(enum uk_pm_shutdown_op op) __isr;

/**
 * Guest migration event.
 * @return 0 after guest migration, <0 on errors
 */
int uk_pm_sysmigration(void);

/**
 * Shutdown this guest and raise the corresponding shutdown operation's event.
 *
 * @param op Specify the shutdown operation
 *	     Invalid operations are mapped to UK_PM_SHUTDOWN_OP_SYSCRASH
 */
void uk_pm_shutdown(enum uk_pm_shutdown_op op) __noreturn __isr;
#else /* !CONFIG_LIBUKPM */
/* Fallbacks */
__noreturn __isr static inline void uk_pm_syshalt(void)
{
	_uk_pm_syshalt_fallback();
}

__noreturn __isr static inline void uk_pm_sysrestart(void)
{
	_uk_pm_syshalt_fallback();
}

__isr static inline int uk_pm_syssuspend(void)
{
	return -ENOTSUP;
}

__noreturn __isr static inline void uk_pm_syscrash(void)
{
	_uk_pm_syshalt_fallback();
}

__noreturn __isr static inline
void uk_pm_shutdown(enum uk_pm_shutdown_op op __unused)
{
	_uk_pm_syshalt_fallback();
}

static inline int uk_pm_sysmigration(void)
{
	return -ENOTSUP;
}

#endif /* !CONFIG_LIBUKPM */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PM_H__ */
