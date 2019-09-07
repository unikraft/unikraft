/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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

#ifndef __UK_9PFS__
#define __UK_9PFS__

#include <stdbool.h>
#include <uk/9pdev.h>
#include <uk/9pfid.h>

#include <vfscore/prex.h>

/*
 * Currently supports only the 9P2000.u variant of the protocol.
 */
enum uk_9pfs_proto {
	UK_9P_PROTO_2000U,
	UK_9P_PROTO_MAX
};

struct uk_9pfs_mount_data {
	/* 9P device. */
	struct uk_9pdev		*dev;
	/* Wanted transport. */
	struct uk_9pdev_trans	*trans;
	/* Protocol version used. */
	enum uk_9pfs_proto	proto;
	/* Username to attempt to mount as on the remote server. */
	const char		*uname;
	/* File tree to access when offered multiple exported filesystems. */
	const char		*aname;
};

struct uk_9pfs_file_data {
	/* Fid associated with the 9pfs file. */
	struct uk_9pfid        *fid;
	/*
	 * Buffer for persisting results from a 9P read operation across
	 * readdir() calls.
	 */
	char                   *readdir_buf;
	/*
	 * Offset within the buffer where the stat of the next child can
	 * be found.
	 */
	int                    readdir_off;
	/* Total size of the data in the readdir buf. */
	int                    readdir_sz;
};

struct uk_9pfs_node_data {
	/* Fid associated with the vfs node. */
	struct uk_9pfid        *fid;
	/* Number of files opened from the vfs node. */
	int                    nb_open_files;
	/* Is a 9P remove call required when nb_open_files reaches 0? */
	bool                   removed;
};

int uk_9pfs_allocate_vnode_data(struct vnode *vp, struct uk_9pfid *fid);
void uk_9pfs_free_vnode_data(struct vnode *vp);

/* Default readdir buffer size. */
#define UK_9PFS_READDIR_BUFSZ	8192

#define UK_9PFS_FD(file) ((struct uk_9pfs_file_data *) (file)->f_data)
#define UK_9PFS_ND(vnode) ((struct uk_9pfs_node_data *) (vnode)->v_data)
#define UK_9PFS_VFID(vnode) (UK_9PFS_ND(vnode)->fid)
#define UK_9PFS_MD(mount) ((struct uk_9pfs_mount_data *) (mount)->m_data)

#endif /* __UK_9PFS__ */
