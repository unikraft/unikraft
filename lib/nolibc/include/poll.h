/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Hugo Lefeuvre <hugo.lefeuvre@manchester.ac.uk>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation,
 *                     All rights reserved.
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _POLL_H
#define _POLL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Requestable events. If poll() finds any of these set, they are
 * copied to revents on return.
 * XXX Note that FreeBSD doesn't make much distinction between POLLPRI
 * and POLLRDBAND since none of the file types have distinct priority
 * bands - and only some have an urgent "mode".
 * XXX Note POLLIN isn't really supported in true SYSV terms. Under SYSV
 * POLLIN includes all of normal, band and urgent data. Most poll handlers
 * on FreeBSD only treat it as "normal" data.
 */
#define POLLIN		0x0001	/* File descriptor is ready for read */
#define POLLPRI		0x0002	/* Exceptional condition */
#define POLLOUT		0x0004	/* File descriptor is ready for write */
#define POLLRDNORM	0x0040	/* Equivalent to POLLIN */
#define POLLRDBAND	0x0080	/* UNUSED */
#define POLLWRNORM	0x0100	/* Equivalent to POLLOUT */
#define POLLWRBAND	0x0200	/* UNUSED */
#define POLLMSG		0x0400	/* UNUSED */
#define POLLRDHUP	0x2000	/* Stream socket peer closed connection */

/*
 * These events are set if they occur regardless of whether they were
 * requested.
 */
#define POLLERR		0x0008	/* Error or read end closed for write end fd */
#define POLLHUP		0x0010	/* Peer closed its end of channel */
#define POLLNVAL	0x0020	/* Invalid request: fd not open */

typedef unsigned int nfds_t;

/*
 * This structure is passed as an array to poll().
 */
struct pollfd {
	int   fd;	/* Which file descriptor to poll */
	short events;	/* Events we are interested in */
	short revents;	/* Events found on return */
};

int poll(struct pollfd _pfd[], nfds_t _nfds, int _timeout);

#ifdef _GNU_SOURCE
#define __NEED_time_t
#define __NEED_struct_timespec
#define __NEED_sigset_t
#include <nolibc-internal/shareddefs.h>
int ppoll(struct pollfd *fds, nfds_t nfds,
	  const struct timespec *tmo_p, const sigset_t *sigmask);
#endif

#ifdef __cplusplus
}
#endif
#endif /* _POLL_H */
