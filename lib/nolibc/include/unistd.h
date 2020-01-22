/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
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

#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_NULL
#define __NEED_size_t
#define __NEED_ssize_t
#define __NEED_off_t
#define __NEED_useconds_t

/*
 * Sysconf name values
 */
#define _SC_PAGESIZE 8
#define _SC_NPROCESSORS_ONLN 10

#include <nolibc-internal/shareddefs.h>

#if CONFIG_HAVE_TIME
unsigned int sleep(unsigned int seconds);
#endif

#if CONFIG_LIBPOSIX_PROCESS
int execl(const char *path, const char *arg, ...
		/* (char  *) NULL */);
int execlp(const char *file, const char *arg, ...
		/* (char  *) NULL */);
int execle(const char *path, const char *arg, ...
		/*, (char *) NULL, char * const envp[] */);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[],
		char *const envp[]);
int execve(const char *filename, char *const argv[],
		char *const envp[]);
#endif

#if CONFIG_LIBVFSCORE
int close(int fd);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count);
void sync(void);
int fsync(int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);
int unlink(const char *pathname);
off_t lseek(int fd, off_t offset, int whence);
#endif

#define STDIN_FILENO	0	/* standard input file descriptor */
#define STDOUT_FILENO	1	/* standard output file descriptor */
#define STDERR_FILENO	2	/* standard error file descriptor */

#ifdef __cplusplus
}
#endif
#endif /* __UNISTD_H__ */
