/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <linuxu/time.h>
#include <sys/types.h>
#include <linuxu/signal.h>

#if defined __X86_64__
#include <linuxu/syscall-x86_64.h>
#elif defined __ARM_32__
#include <linuxu/syscall-arm_32.h>
#else
#error "Unsupported architecture"
#endif

static inline ssize_t sys_read(int fd, const char *buf, size_t len)
{
	return (ssize_t) syscall3(__SC_READ,
				  (long) (fd),
				  (long) (buf),
				  (long) (len));
}

static inline ssize_t sys_write(int fd, const char *buf, size_t len)
{
	return (ssize_t) syscall3(__SC_WRITE,
				  (long) (fd),
				  (long) (buf),
				  (long) (len));
}

static inline int sys_exit(int status)
{
	return (int) syscall1(__SC_EXIT,
			      (long) (status));
}

static inline int sys_clock_gettime(k_clockid_t clk_id, struct k_timespec *tp)
{
	return (int) syscall2(__SC_CLOCK_GETTIME,
			      (long) clk_id,
			      (long) tp);
}

/*
 * Please note that on failure sys_mmap() is returning -errno
 */
#define MAP_SHARED    (0x01)
#define MAP_ANONYMOUS (0x20)
#define PROT_NONE     (0x0)
#define PROT_READ     (0x1)
#define PROT_WRITE    (0x2)
#define PROT_EXEC     (0x4)
static inline void *sys_mmap(void *addr, size_t len, int prot, int flags,
		int fd, off_t offset)
{
	return (void *) syscall6(__SC_MMAP,
				 (long) (addr),
				 (long) (len),
				 (long) (prot),
				 (long) (flags),
				 (long) (fd),
				 (long) (offset));
}

#define sys_mapmem(addr, len)				  \
	sys_mmap((addr), (len), (PROT_READ | PROT_WRITE), \
		 (MAP_SHARED | MAP_ANONYMOUS), -1, 0)


static inline int sys_sigaction(int signum, const struct uk_sigaction *action,
		struct uk_sigaction *oldaction)
{
	return (int) syscall4(__SC_RT_SIGACTION,
			      (long) signum,
			      (long) action,
			      (long) oldaction,
			      sizeof(k_sigset_t));
}

static inline int sys_sigprocmask(int how,
		const k_sigset_t *set, k_sigset_t *oldset)
{
	return (int) syscall4(__SC_RT_SIGPROCMASK,
			      (long) how,
			      (long) set,
			      (long) oldset,
			      sizeof(k_sigset_t));
}

#define ARCH_SET_FS 0x1002
static inline int sys_arch_prctl(int code, unsigned long addr)
{
	return (int) syscall2(__SC_ARCH_PRCTL,
			      (long) code,
			      (long) addr);
}

static inline int sys_pselect6(int nfds,
		k_fd_set *readfds, k_fd_set *writefds, k_fd_set *exceptfds,
		const struct k_timespec *timeout, const void *sigmask)
{
	return (int) syscall6(__SC_PSELECT6,
			      (long) nfds,
			      (long) readfds,
			      (long) writefds,
			      (long) exceptfds,
			      (long) timeout,
			      (long) sigmask);
}

static inline int sys_timer_create(k_clockid_t which_clock,
		struct uk_sigevent *timer_event_spec,
		k_timer_t *created_timer_id)
{
	return (int) syscall3(__SC_TIMER_CREATE,
			      (long) which_clock,
			      (long) timer_event_spec,
			      (long) created_timer_id);

}

static inline int sys_timer_settime(k_timer_t timerid, int flags,
		const struct k_itimerspec *value, struct k_itimerspec *oldvalue)
{
	return (int) syscall4(__SC_TIMER_SETTIME,
			      (long) timerid,
			      (long) flags,
			      (long) value,
			      (long) oldvalue);

}

static inline int sys_timer_delete(k_timer_t timerid)
{
	return (int) syscall1(__SC_TIMER_DELETE,
			      (long) timerid);
}

#endif /* __SYSCALL_H__ */
