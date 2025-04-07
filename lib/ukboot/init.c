/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <uk/essentials.h>
#include <uk/process.h>
#include <uk/sched.h>

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

static inline pid_t wait_nonblocking(void)
{
	int wstatus;
	pid_t pid;

	pid = waitpid(-1, &wstatus, WNOHANG);

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
/* Monotonic time in msec */
static inline long now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;
}

/* Wait for processes to terminate gracefully. Although we return main()'s exit
 * status back to libukboot, when it comes to reaping we don't differentiate
 * between the application processes and reparented children, as these are
 * likely the result of daemonization so we also need to treat as equal.
 *
 * Notice: We don't return on error as we don't have a way to handle these
 *         other than printing an error message (do_init() is expected to
 *         return the application's code, that we don't want to shadow if
 *         the application has already returned by this point).
 */
static void graceful_shutdown(int sigfd)
{
	struct epoll_event ev;
	struct signalfd_siginfo info;
	long deadline; /* msec */
	long timeout;  /* msec */
	ssize_t bytes;
	int epollfd;
	int nfds;
	int ret;

	uk_pr_debug("Signalling children\n");

	kill(0, GRACEFUL_SHUTDOWN_SIGNAL);

	/* Make signalfd non-blocking */
	ret = fcntl(sigfd, F_SETFL, fcntl(sigfd, F_GETFL) | O_NONBLOCK);
	if (unlikely(ret < 0)) {
		uk_pr_err("Could not set signalfd to non-blocking mode (%d)\n",
			  errno);
		return;
	}

	/* Create epoll instance */
	epollfd = epoll_create1(0);
	if (unlikely(epollfd == -1)) {
		uk_pr_err("epoll_create() failed (%d)\n", errno);
		return;
	}

	/* Add existing signalfd to epoll */
	ev.events = EPOLLIN;
	ev.data.fd = sigfd;
	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, sigfd, &ev);
	if (unlikely(ret == -1)) {
		uk_pr_err("epoll_ctl failed (%d)\n", errno);
		goto err_close_epollfd;
	}

	/* Reap terminated children */
	if (GRACEFUL_SHUTDOWN_TIMEOUT > 0)
		deadline = now() + GRACEFUL_SHUTDOWN_TIMEOUT;
	timeout = GRACEFUL_SHUTDOWN_TIMEOUT;
	while (1) {
		/* Wait for events or timeout */
		nfds = epoll_wait(epollfd, &ev, 1, timeout);
		if (unlikely(nfds == -1)) {
			uk_pr_err("epoll_wait failed (%d)\n", errno);
			break;
		}

		if (!nfds) {
			uk_pr_debug("Reached timeout, terminating forcefully\n");
			break;
		}

		/* Process SIGCHLD */
		bytes = read(sigfd, &info, sizeof(info));
		if (bytes == sizeof(info) && info.ssi_signo == SIGCHLD) {
			/* Reap terminated children */
			do {
				ret = wait_nonblocking();
			} while (ret > 0);

			UK_ASSERT(ret == 0 || (ret == -1 && errno == ECHILD));

			if (ret == -1 && errno == ECHILD) {
				uk_pr_debug("All children exited gracefully\n");
				break;
			}
		}

		if (timeout >= 0) {
			if (!timeout) /* User set to zero */
				break;
			timeout = deadline - now();
			if (timeout <= 0)
				break;
		}
	}

err_close_epollfd:
	close(epollfd);
}
#endif /* CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN */

int do_init(int argc, char *argv[])
{
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
	application_pid = uk_posix_process_run(do_main, argc,
					       (const char **)argv);

	/* Wait for application to exit and reap reparented children */
	while (1) {
		bytes = read(sigfd, &info, sizeof(info));
		if (bytes != sizeof(info)) {
			uk_pr_err("Read from signalfd failed\n");
			goto err_close_signalfd;
		}

		if (info.ssi_signo == SIGCHLD) {
			/* Reap children. If the last child terminated,
			 * initiate shutdown.
			 */
			do {
				ret = wait_nonblocking();
			} while (ret > 0);

			UK_ASSERT(ret == 0 || (ret == -1 && errno == ECHILD));

			if (ret == -1 && errno == ECHILD) {
				uk_pr_info("All children terminated. Initiating shutdown...\n");
				break;
			}
		} else if (info.ssi_signo == SIGTERM) {
			uk_pr_info("Received SIGTERM. Initiating shutdown...\n");
			break;
		}
	}

#if CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN
	graceful_shutdown(sigfd);
#endif /* CONFIG_LIBUKBOOT_GRACEFUL_SHUTDOWN */

err_close_signalfd:
	/* If the application is still running, set the error code to SIGKILL
	 * to signify it was (actually, will be) force-killed. Return back to
	 * Unikraft to terminate all remaining processes and shut down the
	 * system.
	 */
	if (!application_returned) {
		uk_pr_debug("The application did not exit gracefully\n");
		application_status = TERM_BY_SIGNAL_BIT | SIGKILL;
	}

	close(sigfd);

	return application_status;
}
