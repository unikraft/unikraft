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

#ifndef __SYSCALL_X86_64_H__
#define __SYSCALL_X86_64_H__

#include <stdint.h>

#define __SC_READ    0
#define __SC_WRITE   1
#define __SC_OPEN    2
#define __SC_CLOSE   3
#define __SC_MMAP    9
#define __SC_MUNMAP 11
#define __SC_RT_SIGACTION   13
#define __SC_RT_SIGPROCMASK 14
#define __SC_IOCTL  16
#define __SC_EXIT   60
#define __SC_ARCH_PRCTL       158
#define __SC_TIMER_CREATE     222
#define __SC_TIMER_SETTIME    223
#define __SC_TIMER_GETTIME    224
#define __SC_TIMER_GETOVERRUN 225
#define __SC_TIMER_DELETE     226
#define __SC_CLOCK_GETTIME    228
#define __SC_PSELECT6 270

/* NOTE: from linux-4.6.3 (arch/x86/entry/entry_64.S):
 *
 * 64-bit SYSCALL saves rip to rcx, clears rflags.RF, then saves rflags to r11,
 * then loads new ss, cs, and rip from previously programmed MSRs.
 * rflags gets masked by a value from another MSR (so CLD and CLAC
 * are not needed). SYSCALL does not save anything on the stack
 * and does not change rsp.
 *
 * Registers on entry:
 * rax  system call number
 * rcx  return address
 * r11  saved rflags (note: r11 is callee-clobbered register in C ABI)
 * rdi  arg0
 * rsi  arg1
 * rdx  arg2
 * r10  arg3 (needs to be moved to rcx to conform to C ABI)
 * r8   arg4
 * r9   arg5
 * (note: r12-r15, rbp, rbx are callee-preserved in C ABI)
 */

#define syscall0(num)					\
({							\
	register long _nret asm("rax") = (num);		\
							\
	asm volatile ("syscall"				\
		      : /* output */			\
			"=r" (_nret)			\
		      : /* input */			\
			"r" (_nret)			\
		      : /* clobbers */			\
			"r10", "r11", "rcx", "memory");	\
	_nret;						\
})

#define syscall1(num, arg0)				\
({							\
	register long _nret asm("rax") = (num);		\
	register long _arg0 asm("rdi") = (arg0);	\
							\
	asm volatile ("syscall"				\
		      : /* output */			\
			"=r" (_nret)			\
		      : /* input */			\
			"r" (_nret),			\
			"r" (_arg0)			\
		      : /* clobbers */			\
			"r10", "r11", "rcx", "memory");	\
	_nret;						\
})

#define syscall2(num, arg0, arg1)			\
({							\
	register long _nret asm("rax") = (num);		\
	register long _arg0 asm("rdi") = (arg0);	\
	register long _arg1 asm("rsi") = (arg1);	\
							\
	asm volatile ("syscall"				\
		      : /* output */			\
			"=r" (_nret)			\
		      : /* input */			\
			"r" (_nret),			\
			"r" (_arg0),			\
			"r" (_arg1)			\
		      : /* clobbers */			\
			"r10", "r11", "rcx", "memory");	\
	_nret;						\
})

#define syscall3(num, arg0, arg1, arg2)			\
({							\
	register long _nret asm("rax") = (num);		\
	register long _arg0 asm("rdi") = (arg0);	\
	register long _arg1 asm("rsi") = (arg1);	\
	register long _arg2 asm("rdx") = (arg2);	\
							\
	asm volatile ("syscall"				\
		      : /* output */			\
			"=r" (_nret)			\
		      : /* input */			\
			"r" (_nret),			\
			"r" (_arg0),			\
			"r" (_arg1),			\
			"r" (_arg2)			\
		      : /* clobbers */			\
			"r10", "r11", "rcx", "memory");	\
	_nret;						\
})

#define syscall4(num, arg0, arg1, arg2, arg3)		\
({							\
	register long _nret asm("rax") = (num);		\
	register long _arg0 asm("rdi") = (arg0);	\
	register long _arg1 asm("rsi") = (arg1);	\
	register long _arg2 asm("rdx") = (arg2);	\
	register long _arg3 asm("r10") = (arg3);	\
							\
	asm volatile ("syscall"				\
		      : /* output */			\
			"=r" (_nret)			\
		      : /* input */			\
			"r" (_nret),			\
			"r" (_arg0),			\
			"r" (_arg1),			\
			"r" (_arg2),			\
			"r" (_arg3)			\
		      : /* clobbers */			\
			"r11", "rcx", "memory");	\
	_nret;						\
})

#define syscall5(num, arg0, arg1, arg2, arg3, arg4)	\
({							\
	register long _nret asm("rax") = (num);		\
	register long _arg0 asm("rdi") = (arg0);	\
	register long _arg1 asm("rsi") = (arg1);	\
	register long _arg2 asm("rdx") = (arg2);	\
	register long _arg3 asm("r10") = (arg3);	\
	register long _arg4 asm("r8")  = (arg4);	\
							\
	asm volatile ("syscall"				\
		      : /* output */			\
			"=r" (_nret)			\
		      : /* input */			\
			"r" (_nret),			\
			"r" (_arg0),			\
			"r" (_arg1),			\
			"r" (_arg2),			\
			"r" (_arg3),			\
			"r" (_arg4)			\
		      : /* clobbers */			\
			"r11", "rcx", "memory");	\
	_nret;						\
})

#define syscall6(num, arg0, arg1, arg2, arg3, arg4,	\
		 arg5)					\
({							\
	register long _nret asm("rax") = (num);		\
	register long _arg0 asm("rdi") = (arg0);	\
	register long _arg1 asm("rsi") = (arg1);	\
	register long _arg2 asm("rdx") = (arg2);	\
	register long _arg3 asm("r10") = (arg3);	\
	register long _arg4 asm("r8")  = (arg4);	\
	register long _arg5 asm("r9")  = (arg5);	\
							\
	asm volatile ("syscall"				\
		      : /* output */			\
			"=r" (_nret)			\
		      : /* input */			\
			"r" (_nret),			\
			"r" (_arg0),			\
			"r" (_arg1),			\
			"r" (_arg2),			\
			"r" (_arg3),			\
			"r" (_arg4),			\
			"r" (_arg5)			\
		      : /* clobbers */			\
			"r11", "rcx", "memory");	\
	_nret;						\
})

#endif /* __SYSCALL_X86_64_H__ */
