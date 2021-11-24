/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <a.jung@lancs.ac.uk>
 *
 * Copyright (c) 2021, Lancaster University. All rights reserved.
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
#ifndef __LINUXU_STAT_H__
#define __LINUXU_STAT_H__

#include <linuxu/time.h>
#include <linuxu/mode.h>

typedef __u64 k_dev_t;
typedef __u64 k_ino_t;
typedef __u32 k_nlink_t;
typedef unsigned int k_uid_t;
typedef unsigned int k_gid_t;
typedef unsigned int k_id_t;
typedef __off k_off_t;
typedef long k_blksize_t;
typedef __s64 k_blkcnt_t;


struct k_stat {
	k_dev_t st_dev;
	k_ino_t st_ino;
	k_nlink_t st_nlink;
	k_mode_t st_mode;
	k_uid_t st_uit;
	k_gid_t st_gid;

	unsigned int __pad0;

	k_dev_t st_rdev;
	k_off_t st_size;
	k_blksize_t st_blksize;
	k_blkcnt_t st_blocks;

	struct k_timespec st_atim;
	struct k_timespec st_mtim;
	struct k_timespec st_ctim;
};

#endif /* __LINUXU_STAT_H__ */
