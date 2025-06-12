/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, University Politehnica of Bucharest.
 * Copyright (c) 2025, Florin Postolache.
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

#ifndef __PROCFS_H__
#define __PROCFS_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <vfscore/uio.h>
#include <stdbool.h>
#include <vfscore/mount.h>

struct procfs_entry;

#define NAME_MAX 255

typedef enum {
    PROCFS_NODE_FILE,
    PROCFS_NODE_DIR,
	PROCFS_NODE_LNK
} procfs_node_type_t;

typedef int (*procop_read_t)(struct procfs_entry *, struct uio *, int);
typedef int (*procop_write_t)(struct procfs_entry *, struct uio *, int);
typedef int (*procop_readlink_t)(struct procfs_entry *, struct uio *);

/*
 * Proc file operations
 *
 * Holds pointers to the proc-specific implementations
 * of the I/O operations listed below
 */
struct procops {
	procop_read_t	read;
	procop_write_t	write;
    procop_readlink_t readlink;
};

#define	proc_noop_write	((procop_write_t)procop_noop)
#define proc_noop_readlink	((procop_readlink_t)procop_noop)

struct procfs_entry {
    const char *name;
	int name_len;
    procfs_node_type_t type;
    mode_t mode;
	size_t rn_size;
    uint16_t uid;
    uint16_t gid;

    // For files
    struct procops *ops;                // Device operations for the file

    // For directories
    struct procfs_entry *children;   // Array of pointers to children
    struct procfs_entry *sibling;   // Array of pointers to sibling
    struct procfs_entry *next;     // Pointer to the parent directory
    struct procfs_entry *parent;   // Pointer to the parent directory

    // Common fields
	struct timespec rn_ctime;
	struct timespec rn_atime;
	struct timespec rn_mtime;
	bool rn_owns_buffer;

    void* data; // Pointer to the data associated with the entry
};

/**
 * Performs a scatter/gather read on the entry pointed to by proc.
 *
 * @param proc
 *   The procfs entry to read from
 * @param uio
 *   The structure containing the scatter-gather list
 * @param ioflags
 *   The flags to be sent to the read operation
 * @return
 *  - (0):  Completed successfully
 *  - (<0): Negative value with error code
 */
int proc_file_read(struct procfs_entry* proc, struct uio* uio, int ioflags);


int proc_link_read(struct procfs_entry *proc, struct uio *uio);
/**
 * Creates a new proc object.
 *
 * @param name
 *   The name of the new proc entry
 * @param type
 *   The type of the proc entry (VREG or VDIR)
 * @param ops
 *   The operations to be used for the proc entry (NULL for directories)
 * @return
 *   - (0):  Completed successfully
 *   - (<0): Negative value with error code
 */
int procfs_create_entry(char *name, procfs_node_type_t type, struct procops *ops);

/**
 * Reads the contents of a procfs entry.
 *
 * @param proc
 *   The procfs entry to read from
 * @param uio
 *   The structure containing the scatter-gather list
 * @param ioflags
 *   The flags to be sent to the read operation
 * @return
 *  - (0):  Completed successfully
 *  - (<0): Negative value with error code
 */
int proc_file_readdir(struct procfs_entry *proc, struct vfscore_file *fp, struct dirent64 *dir);

/**
 * Looks up a procfs entry by name.
 *
 * @param dvp
 *   The directory vnode
 * @param name
 *   The name of the entry to look up
 * @param vpp
 *   Pointer to the vnode pointer to be filled in
 * @return
 *  - (0):  Completed successfully
 *  - (<0): Negative value with error code
 */
int proc_file_lookup(struct vnode *dvp, const char *name, struct vnode **vpp);

int create_root(struct mount *mp);

/**
 * Always returns 0.
 */
int procop_noop();

/**
 * Always returns EPERM.
 */
int procop_eperm();

#ifdef CONFIG_LIBPROCFS_VERSION
int procfs_register_version();
#endif /* CONFIG_LIBPROCFS_VERSION */

#ifdef CONFIG_LIBPROCFS_CMDLINE
int procfs_register_cmdline();
#endif /* CONFIG_LIBPROCFS_CMDLINE */

#ifdef CONFIG_LIBPROCFS_MOUNTS
int procfs_register_mounts();
#endif /* CONFIG_LIBPROCFS_MOUNTS */

#ifdef CONFIG_LIBPROCFS_FILESYSTEMS
int procfs_register_filesystems();
#endif /* CONFIG_LIBPROCFS_FILESYSTEMS */

#ifdef CONFIG_LIBPROCFS_PROC_FOLDER
int procfs_register_proc_folder(size_t id);
#endif /* CONFIG_LIBPROCFS_PROC_FOLDER */

#ifdef CONFIG_LIBPROCFS_SELF_LINK
int procfs_register_self_link();
#endif /* CONFIG_LIBPROCFS_SELF_LINK */

#ifdef CONFIG_LIBPROCFS_SYS_FOLDER
int procfs_register_sys_folder();
#endif /* CONFIG_LIBPROCFS_SYS_FOLDER */

#endif /* !__PROCFS_H__ */
