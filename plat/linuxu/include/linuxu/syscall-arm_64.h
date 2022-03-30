/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>,
 *			Florin Postolache <florin.postolache80@gmail.com>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation,
 *               2022, Florin Postolache, <florin.postolache80@gmail.com>
 *               All rights reserved.
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

#ifndef __SYSCALL_ARM_64_H__
#define __SYSCALL_ARM_64_H__

#include <stdint.h>

#define __SC_FCNTL	25
#define __SC_IOCTL	29
#define __SC_OPENAT	56 /* use openat because open is not on arm64 */
#define __SC_CLOSE	57
#define __SC_READ	63
#define __SC_WRITE	64
#define __SC_PSELECT6	72
#define __SC_FSTAT	80
#define __SC_EXIT	93
#define __SC_TIMER_CREATE	107
#define __SC_TIMER_GETTIME	108
#define __SC_TIMER_GETOVERRUN	109
#define __SC_TIMER_SETTIME	110
#define __SC_TIMER_DELETE	111
#define __SC_CLOCK_GETTIME	113
#define __SC_RT_SIGACTION	134
#define __SC_RT_SIGPROCMASK	135
#define __SC_ARCH_PRCTL	167
#define __SC_SOCKET	198
#define __SC_MUNMAP	215
#define __SC_MMAP	222 /* use mmap2() since mmap() is obsolete */



#ifndef O_TMPFILE
#define O_TMPFILE 020040000
#endif

/* NOTE: from `man syscall`:
 *
 * ARM64
 *
 * instruction  syscall #  retval
 * ------------------------------
 * svc #0       x8         x0
 *
 * arg1  arg2  arg3  arg4  arg5  arg6  arg7
 * ----------------------------------------
 * x0    x1    x2    x3    x4    x5
 *
 */

#define syscall0(num)					\
({							\
	register long _num asm("x8") = (num);		\
	register long _ret asm("x0");			\
							\
	asm volatile ("svc #0"%				\
		      : /* output */			\
			"=r" (_ret)			\
		      : /* input */			\
			"r" (_num)			\
		      : /* clobbers */			\
			"memory");			\
	_ret;						\
})

#define syscall1(num, arg0)				\
({							\
	register long _num   asm("x8") = (num);		\
	register long _a0ret asm("x0") = (arg0);	\
							\
	asm volatile ("svc #0"				\
		      : /* output */			\
			"=r" (_a0ret)			\
		      : /* input */			\
			"r" (_num),			\
			"r" (_a0ret)			\
		      : /* clobbers */			\
			"memory");			\
	_a0ret;						\
})

#define syscall2(num, arg0, arg1)			\
({							\
	register long _num   asm("x8") = (num);		\
	register long _a0ret asm("x0") = (arg0);	\
	register long _arg1  asm("x1") = (arg1);	\
							\
	asm volatile ("svc #0"				\
		      : /* output */			\
			"=r" (_a0ret)			\
		      : /* input */			\
			"r" (_num),			\
			"r" (_a0ret),			\
			"r" (_arg1)			\
		      : /* clobbers */			\
			"memory");			\
	_a0ret;						\
})

#define syscall3(num, arg0, arg1, arg2)			\
({							\
	register long _num   asm("x8") = (num);		\
	register long _a0ret asm("x0") = (arg0);	\
	register long _arg1  asm("x1") = (arg1);	\
	register long _arg2  asm("x2") = (arg2);	\
							\
	asm volatile ("svc #0"				\
		      : /* output */			\
			"=r" (_a0ret)			\
		      : /* input */			\
			"r" (_num),			\
			"r" (_a0ret),			\
			"r" (_arg1),			\
			"r" (_arg2)			\
		      : /* clobbers */			\
			"memory");			\
	_a0ret;						\
})

#define syscall4(num, arg0, arg1, arg2, arg3)		\
({							\
	register long _num   asm("x8") = (num);		\
	register long _a0ret asm("x0") = (arg0);	\
	register long _arg1  asm("x1") = (arg1);	\
	register long _arg2  asm("x2") = (arg2);	\
	register long _arg3  asm("x3") = (arg3);	\
							\
	asm volatile ("svc #0"				\
		      : /* output */			\
			"=r" (_a0ret)			\
		      : /* input */			\
			"r" (_num),			\
			"r" (_a0ret),			\
			"r" (_arg1),			\
			"r" (_arg2),			\
			"r" (_arg3)			\
		      : /* clobbers */			\
			"memory");			\
	_a0ret;						\
})

#define syscall5(num, arg0, arg1, arg2, arg3, arg4)	\
({							\
	register long _num   asm("x8") = (num);		\
	register long _a0ret asm("x0") = (arg0);	\
	register long _arg1  asm("x1") = (arg1);	\
	register long _arg2  asm("x2") = (arg2);	\
	register long _arg3  asm("x3") = (arg3);	\
	register long _arg4  asm("x4") = (arg4);	\
							\
	asm volatile ("svc #0"				\
		      : /* output */			\
			"=r" (_a0ret)			\
		      : /* input */			\
			"r" (_num),			\
			"r" (_a0ret),			\
			"r" (_arg1),			\
			"r" (_arg2),			\
			"r" (_arg3),			\
			"r" (_arg4)			\
		      : /* clobbers */			\
			"memory");			\
	_a0ret;						\
})

#define syscall6(num, arg0, arg1, arg2, arg3, arg4,	\
		 arg5)					\
({							\
	register long _num   asm("x8") = (num);		\
	register long _a0ret asm("x0") = (arg0);	\
	register long _arg1  asm("x1") = (arg1);	\
	register long _arg2  asm("x2") = (arg2);	\
	register long _arg3  asm("x3") = (arg3);	\
	register long _arg4  asm("x4") = (arg4);	\
	register long _arg5  asm("x5") = (arg5);	\
							\
	asm volatile ("svc #0"				\
		      : /* output */			\
			"=r" (_a0ret)			\
		      : /* input */			\
			"r" (_num),			\
			"r" (_a0ret),			\
			"r" (_arg1),			\
			"r" (_arg2),			\
			"r" (_arg3),			\
			"r" (_arg4),			\
			"r" (_arg5)			\
		      : /* clobbers */			\
			"memory");			\
	_a0ret;						\
})


#endif /* __SYSCALL_ARM_64_H__ */
