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

#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>

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

static inline int sys_pselect6(int nfds,
		fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		const struct timespec *timeout, const void *sigmask)
{
	return (int) syscall6(__SC_PSELECT6,
			      (long) nfds,
			      (long) readfds,
			      (long) writefds,
			      (long) exceptfds,
			      (long) timeout,
			      (long) sigmask);
}

#endif /* __SYSCALL_H__ */
