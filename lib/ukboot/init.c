/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <uk/essentials.h>
#include <uk/plat/time.h>
#include <uk/process.h>
#include <uk/sched.h>

#include <uk/pm.h>

#include "init.h"

#if CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN
#define GRACEFUL_SHUTDOWN_SIGNAL			    \
	CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN_SIGNAL

#define GRACEFUL_SHUTDOWN_TIMEOUT			    \
	CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN_TIMEOUT
#endif /* CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN */

/* We use the shell convention to set the high bit
 * when the process is terminated by signal.
 */
#define TERM_BY_SIGNAL_BIT  0x80

static pid_t application_pid;

static int application_returned;
static int application_status;

static inline pid_t wait_for_child(int options)
{
	int wstatus;
	pid_t pid;

	pid = waitpid(-1, &wstatus, options);

	/* Save the application's return code. This will be
	 * the value Unikraft returns upon exit. If the
	 * application was killed we set the high bit as
	 * by convention.
	 */
	if (pid == application_pid) {
		application_returned = 1;
		UK_ASSERT(WIFEXITED(wstatus) || WIFSIGNALED(wstatus));
		if (WIFEXITED(wstatus))
			application_status = WEXITSTATUS(wstatus);
		else if (WIFSIGNALED(wstatus))
			application_status = TERM_BY_SIGNAL_BIT | WTERMSIG(wstatus);
	}

	return pid;
}

#if CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN
#define SHUTDOWN_POLL_MS 50L
#define GRACEFUL_SHUTDOWN_REAP_MS 100L

static inline long now_ms(void)
{
	return ukplat_monotonic_clock() / 1000000UL;
}

static void graceful_shutdown(void)
{
	const long timeout = GRACEFUL_SHUTDOWN_TIMEOUT;
	const int options = timeout < 0 ? 0 : WNOHANG;
	long deadline = now_ms() + MAX(timeout, 0L);
	bool forced = false;

	/* Broadcast to the application processes while leaving init alone. */
	kill(-1, GRACEFUL_SHUTDOWN_SIGNAL);

	for (;;) {
		pid_t pid = wait_for_child(options);
		long remaining;

		if (pid < 0 && errno == ECHILD)
			return;
		if (unlikely(pid < 0 && errno != EINTR)) {
			uk_pr_err("Could not reap children (%d)\n", errno);
			return;
		}
		if (timeout < 0)
			continue;
		remaining = deadline - now_ms();
		if (remaining <= 0) {
			if (forced)
				return;
			/* Broadcast SIGKILL without signalling init. */
			kill(-1, SIGKILL);
			forced = true;
			deadline = now_ms() + GRACEFUL_SHUTDOWN_REAP_MS;
		} else if (pid <= 0) {
			long interval = MIN(remaining, SHUTDOWN_POLL_MS);

			uk_sched_thread_sleep(interval * 1000000UL);
		}
	}
}
#endif /* CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN */

static __noreturn void init_mainlike(void *argc_arg, void *argv_arg)
{
	const int argc = (int)(intptr_t)argc_arg;
	char **const argv = (char **)argv_arg;
	sigset_t mask;
	int r;

	/* Like a process that init starts on Linux, the application begins
	 * with no blocked signals; do_init() blocks all of them only for
	 * itself.
	 */
	sigemptyset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	r = do_main(argc, argv);
	/* INIT main should never return; it should call exit() when done */
	uk_pr_debug("INIT main returned %d\n", r);
	uk_pm_syshalt();
}

int do_init(int argc, char *argv[])
{
	bool graceful __maybe_unused = false;
	struct signalfd_siginfo info;
	ssize_t bytes;
	sigset_t mask;
	int sigfd;
	int ret;

	/* Block all signals */
	sigfillset(&mask);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	/* Prepare mask for signalfd */
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGTERM);

	/* Create signalfd for SIGCHLD and SIGTERM */
	sigfd = signalfd(-1, &mask, SFD_CLOEXEC);
	if (sigfd < 0) {
		uk_pr_err("signalfd error (%d)\n", errno);
		return errno;
	}

	/* Spawn application process */
	application_pid = uk_posix_process_run(init_mainlike, argc,
					       (const char **)argv);

	/* Wait for application to exit and reap reparented children */
	while (1) {
		bytes = read(sigfd, &info, sizeof(info));
		if (bytes != sizeof(info)) {
			uk_pr_err("Read from signalfd failed\n");
			break;
		}

		if (info.ssi_signo == SIGCHLD) {
			/* Reap children. If the last child terminated,
			 * initiate shutdown.
			 */
			do {
				ret = wait_for_child(WNOHANG);
			} while (ret > 0);

			UK_ASSERT(ret == 0 || (ret == -1 && errno == ECHILD));

			if (ret == -1 && errno == ECHILD) {
				uk_pr_info("All children terminated. Initiating shutdown...\n");
				break;
			}
		} else if (info.ssi_signo == SIGTERM) {
			uk_pr_info("Received SIGTERM. Initiating shutdown...\n");
			graceful = true;
			break;
		}
	}

	/* Consume one shutdown request before notifying the applications. */
	close(sigfd);
#if CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN
	if (graceful)
		graceful_shutdown();
#endif /* CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN */

	/* If the application is still running, set the error code to SIGKILL
	 * to signify it was (actually, will be) force-killed. Return back to
	 * Unikraft to terminate all remaining processes and shut down the
	 * system.
	 */
	if (!application_returned) {
		uk_pr_debug("The application did not exit gracefully\n");
		application_status = TERM_BY_SIGNAL_BIT | SIGKILL;
	}

	return application_status;
}
