/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *
 * Copyright (c) 2018, NEC Labs Europe, NEC Corporation. All rights reserved.
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

/* This header does by design not have include guards, so that it can be
 * included from multiple files. The __NEED_x macros instead make sure that
 * only those definitions are included that are required by that specific
 * file, and only if they haven't been defined on a previous pass through
 * this file.
 */

#include <uk/config.h>
#include <uk/arch/types.h>

#if (defined __NEED_NULL && !defined __DEFINED_NULL)
#ifdef __cplusplus
#define NULL 0L
#else
#define NULL ((void *) 0)
#endif
#define __DEFINED__NULL
#endif

#if (defined __NEED_size_t && !defined __DEFINED_size_t)
typedef __sz size_t;
#define __DEFINED_size_t
#endif

#if (defined __NEED_ssize_t && !defined __DEFINED_ssize_t)
typedef __ssz ssize_t;
#define __DEFINED_ssize_t
#endif

#if (defined __NEED_off_t && !defined __DEFINED_off_t)
typedef __off off_t;
#define __DEFINED_off_t
#endif

#if (defined __NEED_mode_t && !defined __DEFINED_mode_t)
typedef unsigned mode_t;
#define __DEFINED_mode_t
#endif

#if defined(__NEED_uid_t) && !defined(__DEFINED_uid_t)
typedef unsigned uid_t;
#define __DEFINED_uid_t
#endif

#if defined(__NEED_gid_t) && !defined(__DEFINED_gid_t)
typedef unsigned gid_t;
#define __DEFINED_gid_t
#endif

#if defined(__NEED_max_align_t) && !defined(__DEFINED_max_align_t)
typedef struct {
	long long __longlongf;

	long double __longdoublef;
} max_align_t;
#define __DEFINED_max_align_t
#endif

#if (defined __NEED_FILE && !defined __DEFINED_FILE)
typedef struct _nolibc_file FILE;
#define __DEFINED_FILE
#endif

#if defined(__NEED_useconds_t) && !defined(__DEFINED_useconds_t)
typedef unsigned useconds_t;
#define __DEFINED_useconds_t
#endif

#if defined(__NEED_pid_t) && !defined(__DEFINED_pid_t)
typedef int pid_t;
#define __DEFINED_pid_t
#endif

#if defined(__NEED_id_t) && !defined(__DEFINED_id_t)
typedef unsigned id_t;
#define __DEFINED_id_t
#endif

#if defined(__NEED_dev_t) && !defined(__DEFINED_dev_t)
typedef __u64 dev_t;
#define __DEFINED_dev_t
#endif

#if defined(__NEED_ino_t) && !defined(__DEFINED_ino_t)
typedef __u64 ino_t;
#define __DEFINED_ino_t
#endif

#if defined(__NEED_nlink_t) && !defined(__DEFINED_nlink_t)
typedef __u64 nlink_t;
#define __DEFINED_nlink_t
#endif

#if defined(__NEED_blkcnt_t) && !defined(__DEFINED_blkcnt_t)
typedef __s64 blkcnt_t;
#define __DEFINED_blkcnt_t
#endif

#if defined(__NEED_blksize_t) && !defined(__DEFINED_blksize_t)
typedef long blksize_t;
#define __DEFINED_blksize_t
#endif

#if defined(__NEED_locale_t) && !defined(__DEFINED_locale_t)
typedef struct __locale_struct *locale_t;
#define __DEFINED_locale_t
#endif

#if defined(__NEED_struct_iovec) && !defined(__DEFINED_struct_iovec)
struct iovec { void *iov_base; size_t iov_len; };
#define __DEFINED_struct_iovec
#endif

#if defined(__NEED_fsblkcnt_t) && !defined(__DEFINED_fsblkcnt_t)
typedef unsigned long long fsblkcnt_t;
#define __DEFINED_fsblkcnt_t
#endif

#if defined(__NEED_fsfilcnt_t) && !defined(__DEFINED_fsfilcnt_t)
typedef unsigned long long fsfilcnt_t;
#define __DEFINED_fsfilcnt_t
#endif

#if defined(__NEED_sigset_t) && !defined(__DEFINED_sigset_t)
typedef unsigned long __sigset_t;
typedef __sigset_t sigset_t;
#define __DEFINED_sigset_t
#endif

#if defined(__NEED_sig_atomic_t) && !defined(__DEFINED_sig_atomic_t)
typedef int __sig_atomic_t;
typedef __sig_atomic_t sig_atomic_t;
#define __DEFINED_sig_atomic_t
#endif

#if defined(__NEED_socklen_t) && !defined(__DEFINED_socklen_t)
typedef unsigned int __socklen_t;
typedef __socklen_t socklen_t;
#define __DEFINED_socklen_t
#endif

#if (defined __NEED_time_t && !defined __DEFINED_time_t)
typedef long time_t;
#define __DEFINED_time_t
#endif

#if (defined __NEED_suseconds_t && !defined __DEFINED_suseconds_t)
typedef long suseconds_t;
#define __DEFINED_suseconds_t
#endif

#if (defined __NEED_struct_timeval && !defined __DEFINED_struct_timeval)
struct timeval {
	time_t      tv_sec;
	suseconds_t tv_usec;
};
#define __DEFINED_struct_timeval
#endif

#if (defined __NEED_struct_timespec && !defined __DEFINED_struct_timespec)
struct timespec {
	time_t tv_sec;
	long   tv_nsec;
};
#define __DEFINED_struct_timespec
#endif

#if defined(__NEED_timer_t) && !defined(__DEFINED_timer_t)
typedef void *timer_t;
#define __DEFINED_timer_t
#endif

#if (defined __NEED_clockid_t && !defined __DEFINED_clockid_t)
typedef int clockid_t;
#define __DEFINED_clockid_t
#endif

#if defined(__NEED_clock_t) && !defined(__DEFINED_clock_t)
typedef long clock_t;
#define __DEFINED_clock_t
#endif

#if defined(__NEED_sa_family_t) && !defined(__DEFINED_sa_family_t)
typedef unsigned short __sa_family_t;
typedef __sa_family_t sa_family_t;
#define __DEFINED_sa_family_t
#endif

#if defined(__NEED_BSD_TYPES) && !defined(__DEFINED_BSD_TYPES)
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#define __DEFINED_BSD_TYPES
#endif

#if (defined __NEED_pthread_t && !defined __DEFINED_pthread_t)
#ifdef __cplusplus
typedef unsigned long pthread_t;
#else
typedef struct __pthread * pthread_t;
#endif
#define __DEFINED_pthread_t
#endif

#if (defined __NEED_pthread_attr_t && !defined __DEFINED_pthread_attr_t)
typedef struct { union { int __i[sizeof(long)==8?14:9]; volatile int __vi[sizeof(long)==8?14:9]; unsigned long __s[sizeof(long)==8?7:9]; } __u; } pthread_attr_t;
#define __DEFINED_pthread_attr_t
#endif
