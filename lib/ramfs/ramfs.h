/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2006-2007, Kohsuke Ohtani
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RAMFS_H
#define _RAMFS_H

#include <vfscore/prex.h>
#include <stdbool.h>

/**
 * struct ramfs_node - A filesystem entry node for RamFS
 */
struct ramfs_node {
	/* Next node in the same directory */
	struct ramfs_node *rn_next;
	/*
	 * First child node if the current node is a directory,
	 * else NULL
	 */
	struct ramfs_node *rn_child;
	/* Inode number of this node, should be unique and stable */
	uint64_t rn_ino;
	/*
	 * Entry type: regular file - VREG, symbolic link - VLNK,
	 * or directory - VDIR
	 */
	int rn_type;
	/* Name of the file/directory (NULL-terminated) */
	char *rn_name;
	/* Length of the name not including the terminator */
	size_t rn_namelen;
	/* Size of the file */
	size_t rn_size;
	/* Buffer to the file data */
	char *rn_buf;
	/* Size of the allocated buffer */
	size_t rn_bufsize;
	/* Last change time */
	struct timespec rn_ctime;
	/* Last access time */
	struct timespec rn_atime;
	/* Last modification time */
	struct timespec rn_mtime;
	/* Node access mode */
	int rn_mode;
	/* Whether the rn_buf was allocated in this ramfs_node */
	bool rn_owns_buf;
};

/**
 * Allocates a new ramfs node.
 *
 * @param name
 *   The entry name
 * @param type
 *   The entry type (regular file - VREG, symbolic link - VLNK,
 *   or directory - VDIR)
 * @param mode
 *   The mode bits of the newly created node
 * @return
 *   Pointer to the new ramfs_node
 */
struct ramfs_node *ramfs_allocate_node(const char *name, int type, mode_t mode);

/**
 * Frees a ramfs node.
 *
 * @param node
 *   Pointer to the ramfs_node that will be freed
 */
void ramfs_free_node(struct ramfs_node *node);

/**
 * Transforms a vnode into a ramfs_node.
 *
 * @param vnode
 *   Virtual node (abstraction offered by vfscore)
 *   holding a reference to a ramfs_node

 * @return
 *   Pointer to the corresponding ramfs_node
 */
#define RAMFS_NODE(vnode) ((struct ramfs_node *) (vnode->v_data))

#endif /* !_RAMFS_H */
