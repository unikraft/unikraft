/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include "signal/signal.h"
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

#include <uk/config.h>
#include <uk/init.h>
#include <uk/process.h>

#include "process.h"

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
static inline void force_kill(struct posix_process *pproc)
{
	UK_ASSERT(pproc);

	if (pproc->state != POSIX_PROCESS_RUNNING)
		return;

	/* Execute the exit part to let libraries clean up, but
	 * don't bother with releasing; we're going down anyway.
	 */
	pprocess_exit(pproc, POSIX_PROCESS_KILLED, SIGKILL);
}

/* We can enter this path if UK_PID_INIT returns, or
 * while it's still running via the shutdown signal.
 */
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
/* Since signals are enabled, if the application has not returned,
 * send a signal and block until it terminates. Once the application
 * is terminated, kill any remaining processes.
 */
static void pprocess_system_shutdown(struct uk_term_ctx *ctx __unused)
{
	struct posix_process *pproc_init;
	struct posix_process *pproc;

	pproc_init = pid2pprocess(UK_PID_INIT);

	if (pproc_init && pproc_init->state == POSIX_PROCESS_RUNNING) {
		pprocess_signal_send(pproc_init, SIGTERM, NULL);
		uk_semaphore_down(&pproc_init->exit_semaphore);
	}

	uk_pprocess_foreach(pproc)
		force_kill(pproc);
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
/* With signals not enabled, if the application did not return
 * by now, we have no choice but kill it. Set exit status to
 * SIGKILL to signify forceful termination. Along with the
 * application, force-kill any remaining process.
 */
static void pprocess_system_shutdown(struct uk_term_ctx *ctx)
{
	struct posix_process *pproc_init;
	struct posix_process *pproc;

	pproc_init = pid2pprocess(UK_PID_INIT);

	/* If terminated, we come from a return,
	 * hence the exit_code is already set.
	 */
	if (pproc_init && pproc_init->state == POSIX_PROCESS_RUNNING)
		ctx->exit_code = 0x80 | SIGKILL;

	uk_pprocess_foreach(pproc)
		force_kill(pproc);
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
#else /* !CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */
/* Multithreading is not enabled. Return the exit code
 * saved by the exit_group() / _exit() stubs.
 */
static void pprocess_system_shutdown(struct uk_term_ctx *ctx)
{
	ctx->exit_code = pprocess_exit_status;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

/* init last, term first */
uk_late_initcall(0, pprocess_system_shutdown);
