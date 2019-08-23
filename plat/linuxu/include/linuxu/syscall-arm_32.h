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

#ifndef __SYSCALL_ARM_32_H__
#define __SYSCALL_ARM_32_H__

#include <stdint.h>

#define __SC_READ       3
#define __SC_WRITE      4
#define __SC_OPEN       5
#define __SC_CLOSE      6
#define __SC_MMAP     192 /* use mmap2() since mmap() is obsolete */
#define __SC_MUNMAP    91
#define __SC_EXIT       1
#define __SC_IOCTL     54
#define __SC_RT_SIGPROCMASK   126
#define __SC_ARCH_PRCTL       172
#define __SC_RT_SIGACTION     174
#define __SC_TIMER_CREATE     257
#define __SC_TIMER_SETTIME    258
#define __SC_TIMER_GETTIME    259
#define __SC_TIMER_GETOVERRUN 260
#define __SC_TIMER_DELETE     261
#define __SC_CLOCK_GETTIME    263
#define __SC_PSELECT6 335

/* NOTE: from `man syscall`:
 *
 * ARM/EABI
 *
 * instruction  syscall #  retval
 * ------------------------------
 * swi 0x0      r7         r0
 *
 * arg1  arg2  arg3  arg4  arg5  arg6  arg7
 * ----------------------------------------
 * r0    r1    r2    r3    r4    r5    r6
 *
 */

#define syscall0(num)					\
({							\
	register long _num asm("r7") = (num);		\
	register long _ret asm("r0");			\
							\
	asm volatile ("swi #0"%				\
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
	register long _num   asm("r7") = (num);		\
	register long _a0ret asm("r0") = (arg0);	\
							\
	asm volatile ("swi #0"				\
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
	register long _num   asm("r7") = (num);		\
	register long _a0ret asm("r0") = (arg0);	\
	register long _arg1  asm("r1") = (arg1);	\
							\
	asm volatile ("swi #0"				\
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
	register long _num   asm("r7") = (num);		\
	register long _a0ret asm("r0") = (arg0);	\
	register long _arg1  asm("r1") = (arg1);	\
	register long _arg2  asm("r2") = (arg2);	\
							\
	asm volatile ("swi #0"				\
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
	register long _num   asm("r7") = (num);		\
	register long _a0ret asm("r0") = (arg0);	\
	register long _arg1  asm("r1") = (arg1);	\
	register long _arg2  asm("r2") = (arg2);	\
	register long _arg3  asm("r3") = (arg3);	\
							\
	asm volatile ("swi #0"				\
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
	register long _num   asm("r7") = (num);		\
	register long _a0ret asm("r0") = (arg0);	\
	register long _arg1  asm("r1") = (arg1);	\
	register long _arg2  asm("r2") = (arg2);	\
	register long _arg3  asm("r3") = (arg3);	\
	register long _arg4  asm("r4") = (arg4);	\
							\
	asm volatile ("swi #0"				\
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
	register long _num   asm("r7") = (num);		\
	register long _a0ret asm("r0") = (arg0);	\
	register long _arg1  asm("r1") = (arg1);	\
	register long _arg2  asm("r2") = (arg2);	\
	register long _arg3  asm("r3") = (arg3);	\
	register long _arg4  asm("r4") = (arg4);	\
	register long _arg5  asm("r5") = (arg5);	\
							\
	asm volatile ("swi #0"				\
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

#define syscall7(num, arg0, arg1, arg2, arg3, arg4,	\
		 arg5, arg6)				\
({							\
	register long _num   asm("r7") = (num);		\
	register long _a0ret asm("r0") = (arg0);	\
	register long _arg1  asm("r1") = (arg1);	\
	register long _arg2  asm("r2") = (arg2);	\
	register long _arg3  asm("r3") = (arg3);	\
	register long _arg4  asm("r4") = (arg4);	\
	register long _arg5  asm("r5") = (arg5);	\
	register long _arg6  asm("r6") = (arg6);	\
							\
	asm volatile ("swi #0"				\
		      : /* output */			\
			"=r" (_a0ret)			\
		      : /* input */			\
			"r" (_num),			\
			"r" (_a0ret),			\
			"r" (_arg1),			\
			"r" (_arg2),			\
			"r" (_arg3),			\
			"r" (_arg4),			\
			"r" (_arg5),			\
			"r" (_arg6)			\
		      : /* clobbers */			\
			"memory");			\
	_a0ret;						\
})

#endif /* __SYSCALL_ARM_32_H__ */
