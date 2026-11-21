/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __POSIX_POLL_SIGNAL_WAIT_H__
#define __POSIX_POLL_SIGNAL_WAIT_H__

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <sys/select.h>
#include <uk/config.h>
#include <uk/essentials.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include <uk/process.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

/* A poll(), ppoll(), select() or pselect6() that watches no fd and has no
 * timeout waits for a signal alone, which the epoll wait they share does
 * not wake for: it is pause(), or sigsuspend() given a mask.  arm64 has no
 * pause syscall, so libcs make pause() there as ppoll(NULL, 0, NULL, NULL).
 * -ENOTSUP: the thread is not a POSIX one and has no signals to wait for;
 * the call goes on as before: the epoll wait, or ENOSYS given a mask.
 * (Without CONFIG_LIBPOSIX_PROCESS_SIGNAL the predicates below are false
 * and this is never called.)
 */
static inline int posix_poll_signal_wait(const void *sigmask __maybe_unused,
					 size_t sigsetsize __maybe_unused)
{
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	return sigmask ? uk_sys_rt_sigsuspend(sigmask, sigsetsize)
		       : uk_sys_pause();
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
	return -ENOTSUP;
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
}

/* Whether poll() watches no fd: negative ones are skipped.  An array that
 * is not there is left for uk_sys_ppoll() to refuse.
 */
static inline bool posix_poll_watches_nothing(const struct pollfd *fds
					      __maybe_unused,
					      nfds_t nfds __maybe_unused)
{
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	if (!fds)
		return !nfds;
	for (nfds_t i = 0; i < nfds; i++) {
		if (fds[i].fd >= 0)
			return false;
	}
	return true;
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
	return false;
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
}

/* As uk_sys_ppoll() does, and Linux on every return: no events */
static inline void posix_poll_clear_revents(struct pollfd *fds, nfds_t nfds)
{
	for (nfds_t i = 0; fds && i < nfds; i++)
		fds[i].revents = 0;
}

/* Whether select() watches no fd.  An nfds out of range is left for
 * uk_sys_pselect() to refuse.
 */
static inline bool
posix_select_watches_nothing(int nfds __maybe_unused,
			     const fd_set *readfds __maybe_unused,
			     const fd_set *writefds __maybe_unused,
			     const fd_set *exceptfds __maybe_unused)
{
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	if (nfds < 0 || nfds > FD_SETSIZE)
		return false;
	for (int fd = 0; fd < nfds; fd++) {
		if ((readfds && FD_ISSET(fd, readfds)) ||
		    (writefds && FD_ISSET(fd, writefds)) ||
		    (exceptfds && FD_ISSET(fd, exceptfds)))
			return false;
	}
	return true;
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
	return false;
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
}

#endif /* __POSIX_POLL_SIGNAL_WAIT_H__ */
