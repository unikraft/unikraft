/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

/* This file contains definitions for interfacing with the Linux kernel. To
 * do so, it needs to use ABI-compatible definitions of types such as sigset_t
 * which are not exactly defined by the POSIX standard and hence can vary
 * between libc implementations. This is why we provide this file for use
 * in the plat/linuxu code instead of including a libc-provided signal.h.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

/* Signal numbers */
#define SIGALRM       14

/* type definitions */
typedef unsigned long k_sigset_t;
typedef struct {
	unsigned long fds_bits[128 / sizeof(long)];
} k_fd_set;

/* sigaction */
typedef void (*uk_sighandler_t)(int);
typedef void (*uk_sigrestore_t)(void);

struct uk_sigaction {
	uk_sighandler_t k_sa_handler;
	int k_sa_flags;
	uk_sigrestore_t k_sa_restorer;
	k_sigset_t k_sa_mask;
};

/* sigaction flags */
#define SA_SIGINFO      0x00000004
#define SA_RESTORER     0x04000000


/* Signal enabling/disabling definitions (sigprocmask) */
#ifndef SIG_BLOCK
#define SIG_BLOCK     0
#endif
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK   1
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK   2
#endif

/* sigset utils */
#define SIGSET_WORDS_NUM    (sizeof(k_sigset_t) / sizeof(unsigned long))

#define k_sigemptyset(set) \
	({ \
		unsigned int __count = 0; \
		unsigned long *__set = (set); \
		while (__count++ < SIGSET_WORDS_NUM) \
			*__set++ = 0; \
		0; \
	})

#define k_sigfillset(set) \
	({ \
		unsigned int __count = 0; \
		unsigned long *__set = (set); \
		while (__count++ < SIGSET_WORDS_NUM) \
			*__set++ = ~0UL; \
		0; \
	})

#define k_sigisemptyset(set) \
	({ \
		unsigned int __count = 0; \
		const unsigned long *__set = (set); \
		int __ret = __set[__count++]; \
		while (!__ret && __count < SIGSET_WORDS_NUM) \
			__ret = __set[__count++]; \
		__ret == 0; \
	})


#define sig_word_idx(sig) \
	(((sig) - 1) / (8 * sizeof(unsigned long)))

#define sig_word_mask(sig) \
	(1UL << (((sig) - 1) % (8 * sizeof(unsigned long))))


#define k_sigaddset(set, sig) \
	({ \
		unsigned long __word = sig_word_idx(sig); \
		unsigned long __mask = sig_word_mask(sig); \
		unsigned long *__set = (set); \
		__set[__word] |= __mask; \
		0; \
	})

#define k_sigdelset(set, sig) \
	({ \
		unsigned long __word = sig_word_idx(sig); \
		unsigned long __mask = sig_word_mask(sig); \
		unsigned long *__set = (set); \
		__set[__word] &= ~__mask; \
		0; \
	})

#define k_sigismember(set, sig) \
	({ \
		unsigned long __word = sig_word_idx(sig); \
		unsigned long __mask = sig_word_mask(sig); \
		unsigned long *__set = (set); \
		__set[__word] & __mask ? 1 : 0; \
	})


/* Signal event definitions */
typedef union uk_sigval {
	int sival_int;
	void *sival_ptr;
} uk_sigval_t;

typedef struct uk_sigevent {
	uk_sigval_t sigev_value;
	int sigev_signo;
	int sigev_notify;

	/* We aren't interested now in what follows here */
	int pad[64];

} uk_sigevent_t;

#endif /* __SIGNAL_H__ */
