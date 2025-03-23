/*
 * Unikraft NoLibC
 *
 * Authors: Alexander Jung <a.jung@lancs.ac.uk>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
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
 */

#include <sys/stat.h>
#include <errno.h>

/*
 * Linux Standard Base (LSB) compatibility functions for stat
 * The 'ver' parameter is used to indicate the version of the stat structure
 * but is currently unused in Unikraft.
 */

#if UK_LIBC_SYSCALLS
 static int __xstat(int ver __unused, const char *pathname,
		    struct stat *st)
 {
    return stat(pathname, st);
 }
 #ifdef __xstat64
 #undef __xstat64
 #endif

 LFS64(__xstat);
 #endif /* UK_LIBC_SYSCALLS */
 #if UK_LIBC_SYSCALLS
 int __fxstatat(int ver __unused, int dirfd, const char *pathname,
		struct stat *st, int flags)
 {
    return fstatat(dirfd, pathname, st, flags);
 }
 #ifdef __fxstatat64
 #undef __fxstatat64
 #endif

 LFS64(__fxstatat);
 #endif /* UK_LIBC_SYSCALLS */
  #if UK_LIBC_SYSCALLS
int __fxstat(int ver __unused, int fd, struct stat *st)
{
    return fstat(fd, st);
}

#ifdef __fxstat64
#undef __fxstat64
#endif

LFS64(__fxstat);
#endif /* UK_LIBC_SYSCALLS */
#if UK_LIBC_SYSCALLS
 int __lxstat(int ver __unused, const char *pathname, struct stat *st)
 {
    return lstat(pathname, st);
 }

 #ifdef __lxstat64
 #undef __lxstat64
 #endif

 LFS64(__lxstat);
 #else
 int __lxstat(int ver, const char *pathname, struct stat *st);
 #endif /* UK_LIBC_SYSCALLS */
  #if UK_LIBC_SYSCALLS
 int __xmknod(int ver, const char *pathname,
	      mode_t mode, dev_t *dev __unused)
 {
    return mknod(pathname, mode, *dev);
 }
 #endif /* UK_LIBC_SYSCALLS */
