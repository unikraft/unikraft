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
 */

#ifndef __UK_9PFS__
#define __UK_9PFS__

#include <stdbool.h>
#include <uk/9pdev.h>
#include <uk/9pfid.h>

#include <vfscore/prex.h>

/**
 * Protocol version; the default version is `9P2000.L`,
 * it can be changed via the `CONFIG_LIBVFSCORE_ROOTOPTS`
 * config option.
 */
enum uk_9pfs_proto {
	/*
	 * Extensions and minor adjustments are added to
	 * the basic protocol for Unix systems
	 */
	UK_9P_PROTO_2000U,
	/*
	 * Extensions and minor adjustments are added to
	 * the basic protocol for Linux systems
	 */
	UK_9P_PROTO_2000L,
	/*
	 * The number of existent protocols;
	 * in this case, two (`UK_9P_PROTO_2000U` and `UK_9P_PROTO_2000L`)
	 */
	UK_9P_PROTO_MAX
};

/**
 * An entry containing the necessary data for mounting the filesystem
 */
struct uk_9pfs_mount_data {
	/* 9P device, used to interact with a `virtio` device;
	 * the initialization for this field is done in
	 * `uk_9p_dev_connect()`
	 */
	struct uk_9pdev		*dev;
	/* Wanted transport */
	struct uk_9pdev_trans	*trans;
	/* Protocol version used */
	enum uk_9pfs_proto	proto;
	/* User name to attempt to mount as on the remote server */
	char			*uname;
	/*
	 * File system name to access when the server is
	 * offering several exported file systems.
	 */
	char			*aname;
};

/**
 * An entry containing the necessary data
 */
struct uk_9pfs_file_data {
	/* File id of the 9pfs file */
	struct uk_9pfid        *fid;
	/*
	 * Buffer for persisting results from a 9P read operation across
	 * `readdir()` calls
	 */
	char                   *readdir_buf;
	/*
	 * Offset within the buffer where the `uk_9p_stat`
	 * stat structure of the next child can be found
	 */
	int                    readdir_off;
	/* Total size of the data in the `readdir` buf */
	int                    readdir_sz;
};

/**
 * A filesystem entry node for 9pfs
 */
struct uk_9pfs_node_data {
	/* File id of the vfs node */
	struct uk_9pfid        *fid;
	/* Number of files opened from the vfs node */
	int                    nb_open_files;
	/* Is a 9P remove call required when `nb_open_files` reaches 0? */
	bool                   removed;
};

/**
 * Allocates a new `uk_9pfs_node_data` entry and associates it with a `vnode`.
 *
 * @param vp
 *   Pointer to the `vnode` which will contain the `uk_9pfs_node_data` entry
 * @param fid
 *   File id of the vfs node
 * @return
 *   0 on success, -ENOMEM for memory allocation error
 */
int uk_9pfs_allocate_vnode_data(struct vnode *vp, struct uk_9pfid *fid);

/**
 * Frees a new `uk_9pfs_node_data` entry and removes it from the `vnode`.
 *
 * @param vp
 *   Pointer to the `vnode` which contains the `uk_9pfs_node_data`
 *   that will be freed
 */
void uk_9pfs_free_vnode_data(struct vnode *vp);

/**
 * Default `readdir` buffer size.
 */
#define UK_9PFS_READDIR_BUFSZ	8192

/**
 * Converts a `vfscore_file` into a `uk_9pfs_file_data`.
 *
 * @param file
 *   Virtual file (abstraction offered by `vfscore`)
 *   holding a reference to a `uk_9pfs_file_data`
 * @return
 *   Pointer to the corresponding `uk_9pfs_file_data`
 */
#define UK_9PFS_FD(file) ((struct uk_9pfs_file_data *) (file)->f_data)

/**
 * Converts a `vnode` into a `uk_9pfs_node_data`.
 *
 * @param vnode
 *   Virtual node (abstraction offered by `vfscore`)
 *   holding a reference to a `uk_9pfs_node_data`
 * @return
 *   Pointer to the corresponding `uk_9pfs_node_data`
 */
#define UK_9PFS_ND(vnode) ((struct uk_9pfs_node_data *) (vnode)->v_data)

/**
 * Returns the file id of a `vnode`.
 *
 * @param vnode
 *   Virtual node (abstraction offered by `vfscore`)
 *   holding a reference to a `uk_9pfs_node_data`, which
 *   contains the file id.
 * @return
 *   File id of the vnode
 */
#define UK_9PFS_VFID(vnode) (UK_9PFS_ND(vnode)->fid)

/**
 * Converts a mount into `uk_9pfs_mount_data`.
 *
 * @param mount
 *   Mount data (abstraction offered by `vfscore`)
 *   holding a reference to a `uk_9pfs_mount_data`
 * @return
 *   Pointer to the corresponding `uk_9pfs_mount_data`
 */
#define UK_9PFS_MD(mount) ((struct uk_9pfs_mount_data *) (mount)->m_data)

#endif /* __UK_9PFS__ */
