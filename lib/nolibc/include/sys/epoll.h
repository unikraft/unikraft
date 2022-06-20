/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
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
#ifndef _SYS_EPOLL_H
#define _SYS_EPOLL_H

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define __NEED_sigset_t
#include <nolibc-internal/shareddefs.h>

/* Flags for epoll_create1() */
#define EPOLL_CLOEXEC O_CLOEXEC

/* Valid opcodes to issue to epoll_ctl() */
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

/* Epoll event masks */
#define EPOLLIN		0x00000001	/* File descriptor is ready for read */
#define EPOLLPRI	0x00000002	/* Exceptional condition */
#define EPOLLOUT	0x00000004	/* File descriptor is ready for write */
#define EPOLLRDNORM	0x00000040	/* Equivalent to POLLIN */
#define EPOLLRDBAND	0x00000080	/* UNUSED */
#define EPOLLWRNORM	0x00000100	/* Equivalent to POLLOUT */
#define EPOLLWRBAND	0x00000200	/* UNUSED */
#define EPOLLMSG	0x00000400	/* UNUSED */
#define EPOLLRDHUP	0x00002000	/* Stream peer closed connection */

/*
 * These events are set if they occur regardless of whether they were
 * requested.
 */
#define EPOLLERR	0x00000008	/* Error condition */
#define EPOLLHUP	0x00000010	/* Peer closed its end of channel */
#define EPOLLNVAL	0x00000020	/* Invalid request: fd not open */

#define EPOLLEXCLUSIVE	(1U << 28)	/* Set exclusive wakeup mode for fd */
#define EPOLLWAKEUP	(1U << 29)
#define EPOLLONESHOT	(1U << 30)	/* Set one-shot behavior for fd */
#define EPOLLET		(1U << 31)	/* Set edge-triggered behavior for fd */

typedef union epoll_data {
	void *ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;
} epoll_data_t;

struct epoll_event {
	uint32_t events;	/* Epoll events */
	epoll_data_t data;	/* User data variable */
} __packed;

int epoll_create(int size);
int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents,
	       int timeout);
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
		int timeout, const sigset_t *sigmask);

#ifdef __cplusplus
}
#endif
#endif /* _SYS_EPOLL_H */
