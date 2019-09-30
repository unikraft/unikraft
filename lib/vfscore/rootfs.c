/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Mount VFS root
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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
#include <errno.h>
#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/libparam.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <uk/init.h>

static const char *rootfs   = CONFIG_LIBVFSCORE_ROOTFS;

#ifndef CONFIG_LIBVFSCORE_ROOTDEV
static const char *rootdev  = "";
#else
static const char *rootdev  = CONFIG_LIBVFSCORE_ROOTDEV;
#endif

#ifndef CONFIG_LIBVFSCORE_ROOTOPTS
static const char *rootopts = "";
#else
static const char *rootopts = CONFIG_LIBVFSCORE_ROOTOPTS;
#endif

#ifndef CONFIG_LIBVFSCORE_ROOTFLAGS
static __u64 rootflags;
#else
static __u64 rootflags      = (__u64) CONFIG_LIBVFSCORE_ROOTFLAGS;
#endif

UK_LIB_PARAM_STR(rootfs);
UK_LIB_PARAM_STR(rootdev);
UK_LIB_PARAM_STR(rootopts);
UK_LIB_PARAM(rootflags, __u64);

static int vfscore_rootfs(void)
{
	/*
	 * Initialization of the root filesystem '/'
	 * NOTE: Any additional sub mount points (like '/dev' with devfs)
	 * have to be mounted later.
	 */
	if (!rootfs || rootfs[0] == '\0') {
		uk_pr_crit("Parameter 'vfs.rootfs' is invalid\n");
		return -1;
	}

	uk_pr_info("Mount %s to /...\n", rootfs);
	if (mount(rootdev, "/", rootfs, rootflags, rootopts) != 0) {
		uk_pr_crit("Failed to mount /: %d\n", errno);
		return -1;
	}

	/*
	 * TODO: Alternatively we could extract an archive found
	 * as initrd to a ramfs '/' if we have got fsname 'initrd'
	 */

	return 0;
}

uk_rootfs_initcall_prio(vfscore_rootfs, 4);
