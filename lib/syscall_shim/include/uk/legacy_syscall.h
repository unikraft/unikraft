/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Răzvan Vîrtan <virtanrazvan@gmail.com>
 *
 *
 * Copyright (c) 2022, University Politehnica of Bucharest. All rights reserved.
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

/**
 * This file defines the syscalls that are marked as legacy.
 *
 * A legacy syscall is a syscall that is missing on some architectures, but
 * it's functionality can be implemented using a newer syscall. Therefore,
 * this syscalls will not generate build failures on the architectures that
 * don't implement them.
 *
 * The format for marking a system call as legacy is the following one:
 * `#define LEGACY_SYS_<syscall_name>` (see the list below).
 */

#ifndef __UK_LEGACY_SYSCALL_H__
#define __UK_LEGACY_SYSCALL_H__

#define LEGACY_SYS_fork /* modern: clone */
#define LEGACY_SYS_vfork /* modern: clone */
#define LEGACY_SYS_getpgrp /* modern: getpgid */
#define LEGACY_SYS_readlink /* modern: readlinkat */
#define LEGACY_SYS_symlink /* modern: symlinkat */
#define LEGACY_SYS_unlink /* modern: unlinkat */
#define LEGACY_SYS_link /* modern: linkat */
#define LEGACY_SYS_access /* modern: faccessat */
#define LEGACY_SYS_open /* modern: openat */
#define LEGACY_SYS_creat /* modern: openat */
#define LEGACY_SYS_mkdir /* modern: mkdirat */
#define LEGACY_SYS_rmdir /* modern: unlinkat */
#define LEGACY_SYS_mknod /* modern: mknodat */
#define LEGACY_SYS_chmod /* modern: fchmodat */
#define LEGACY_SYS_lchown /* modern: fchownat */
#define LEGACY_SYS_chown /* modern: fchownat */
#define LEGACY_SYS_rename /* modern: renameat */
#define LEGACY_SYS_lstat /* modern: fstatat */
#define LEGACY_SYS_stat /* modern: fstatat */
#define LEGACY_SYS_getdents /* modern: getdents64 */
#define LEGACY_SYS_time /* modern: gettimeofday */
#define LEGACY_SYS_futimesat /* modern: utimensat */
#define LEGACY_SYS_utime /* modern: utimensat */
#define LEGACY_SYS_utimes /* modern: utimensat */
#define LEGACY_SYS_dup2 /* modern: dup3 */
#define LEGACY_SYS_pipe /* modern: pipe2 */
#define LEGACY_SYS_pause /* modern: sigsuspend */
#define LEGACY_SYS_alarm /* modern: timer_settime */
#define LEGACY_SYS_poll /* modern: ppoll */
#define LEGACY_SYS_select /* modern: pselect */
#define LEGACY_SYS_epoll_create /* modern: epoll_create1 */
#define LEGACY_SYS_epoll_wait /* modern: epoll_pwait */
#define LEGACY_SYS_eventfd /* modern: eventfd2 */

#endif
