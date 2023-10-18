/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_BOOT_SHUTDOWN_REQ_H__
#define __UK_BOOT_SHUTDOWN_REQ_H__

#include <uk/config.h>
#if CONFIG_LIBUKBOOT_MAINTHREAD
#include <uk/semaphore.h>

/* Library-internal control structure to handle shutdown requests */
struct uk_boot_shutdown_ctl {
	struct {
		unsigned long already_requested;
		enum ukplat_gstate target;
	} request;
	struct uk_semaphore barrier;
};

/* Defined in shutdown_req.c */
extern struct uk_boot_shutdown_ctl shutdown_ctl;

/* The following definitions/functions have to called from init context, only */
#define uk_boot_shutdown_ctl_init()				\
	do {							\
		/* Initialize shutdown control structure */	\
		shutdown_ctl.request.already_requested = 0;	\
		shutdown_ctl.request.target = UKPLAT_HALT;	\
		uk_semaphore_init(&shutdown_ctl.barrier, 0);	\
	} while (0)

static inline enum ukplat_gstate uk_boot_shutdown_barrier(void)
{
	/* Block execution until we receive first shutdown request */
	uk_semaphore_down(&shutdown_ctl.barrier);
	/* Only `uk_boot_shutdown_req()` call can wake us up.
	 * Since only the first `uk_boot_shutdown_req` is modifying
	 * `shutdown.request.target` before it unblocks the "init"
	 * thread, here, the field can be accessed without locking.
	 */
	return shutdown_ctl.request.target;
}

#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

#endif /* __UK_BOOT_SHUTDOWN_REQ_H__ */
