/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florin Postolache <florin.postolache80@gmail.com>
 *
 * Copyright (c) 2025, University Politehnica of Bucharest.
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
 */

#include <procfs/proc.h>
#include <vfscore/vnode.h>
#include <string.h>
#include <uk/store.h>

#ifdef CONFIG_LIBPROCFS_FILESYSTEMS
static char *filesystems_name = "filesystems";

static int
filesystems_read(struct procfs_entry *entry __unused, struct uio *uio,
		 int flags __unused)
{
	const struct uk_store_entry *store_entry;
	int rc;
	char *buff;

	store_entry = uk_store_static_entry_get(uk_libid("libvfscore"), 0x01);
	if (!store_entry) {
		uk_pr_crit("Failed to get filesystems entry from store\n");
		return -ENOENT;
	}
	rc = uk_store_get_value(store_entry, charp, &buff);
	vfscore_uiomove(buff, strlen(buff), uio);
	return 0;
}

static struct procops filesystems_procops = {
	.read = filesystems_read,
	.write = proc_noop_write,
	.readlink = proc_noop_readlink
};

#endif /* CONFIG_LIBPROCFS_FILESYSTEMS */

int procfs_register_filesystems(void)
{
	int rc = 0;

#ifdef CONFIG_LIBPROCFS_FILESYSTEMS
	uk_pr_debug("Register '%s' to procfs\n", filesystems_name);

	/* register /proc/filesystems */
	rc = procfs_create_entry(filesystems_name,
				 PROCFS_NODE_FILE, &filesystems_procops);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to procfs: %d\n",
			  filesystems_name, rc);
		return -rc;
	}
#endif /* CONFIG_LIBPROCFS_FILESYSTEMS */

	return rc;
}
