/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
 * Copyright (C) 2014 Cloudius Systems, Ltd.
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

#ifndef _VFS_H
#define _VFS_H

#define _GNU_SOURCE
#include <vfscore/mount.h>

#include <limits.h>
#include <fcntl.h>
#include <sys/statfs.h>
#include <sys/time.h>

/*
 * Tunable parameters
 */

/* max length of 'file system' name */
#define FSMAXNAMES	16

#ifdef DEBUG_VFS

extern int vfs_debug;

#define	VFSDB_CORE	0x00000001
#define	VFSDB_SYSCALL	0x00000002
#define	VFSDB_VNODE	0x00000004
#define	VFSDB_BIO	0x00000008
#define	VFSDB_CAP	0x00000010
#define VFSDB_FLAGS	0x00000013

/**
 * Prints debug messages.
 *
 * @param _m
 *	Type of event. It could be one of VFSDB_* defined above.
 * @param X
 *	Format string, followed by a variable-length list of arguments,
 *	similar to arguments passed to printf().
 */
#define DPRINTF(_m, X)	do { if (vfs_debug & (_m)) uk_pr_debug X; } while (0)
#else
#define DPRINTF(_m, X)
#endif

#define ASSERT(e)	assert(e)

/*
 * per task data
 */
struct task {
	/* current working directory */
	char t_cwd[PATH_MAX];
	/* vfscore_file structure associated with current working directory */
	struct vfscore_file *t_cwdfp;
};

/**
 * Opens the file specified by path.
 *
 * @param path
 *	Path to the file to be opened
 * @param flags
 *	Flags that specifies how the file should be opened. It must contain one
 *	of the following `access` modes:
 *		* O_RDONLY  - uses the file only for reading purposes.
 *		* O_WRONLY  - uses the file only for writing purposes.
 *		* O_RDWR  - uses the file for both reading and writing.
 *		* Besides that, other flags can be specified as well:
 *			* O_APPEND - positions the cursor at the end of the file
 *				before the write operations.
 *			* O_EXCL - if used with O_CREAT, it makes the sys_open()
 *				call fail if the file already exists.
 *			* O_NOFOLLOW - makes the sys_open() call fail if the
 *				final component of the path refers to a
 *				symbolic link.
 *			* O_TRUNC  - truncates a regular file to zero bytes if
 *				the file was successfully opened in O_RDWR or
 *				O_WRONLY mode.
 *
 * @param mode
 *	Mode bits to be applied when the new file is created
 *	(these are ignored unless O_CREAT is provided in flags)
 * @param[out] fp
 *	Pointer to the task structure containing information about the opened
 *	file on success, NULL on failure.
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_open(char *path, int flags, mode_t mode, struct vfscore_file **fp);

/**
 * Reads data from the descriptor provided by the fp structure into
 * niov buffers of type iovec, starting at offset. The number of read bytes is
 * returned in count.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param iov
 *	Array of buffers to store the data into
 * @param niov
 *	Number of elements in the iov array
 * @param offset
 *	Offset to start reading at
 * @param[out] count
 *	Number of read bytes
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_read(struct vfscore_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count);

/**
 * Writes data from the array of buffers iov of size niov at the
 * file descriptor provided by the fp structure, starting at offset.
 * The number of written bytes is returned in count.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param iov
 *	Array of buffers to write the data into
 * @param niov
 *	Number of elements in the iov array
 * @param offset
 *	Offset to start writing at
 * @param[out] count
 *	Number of bytes written
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_write(struct vfscore_file *fp, const struct iovec *iov, size_t niov,
		off_t offset, size_t *count);


/**
 * Synchronizes the in-core data referred by vfscore_file to the backing
 * store device.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fsync(struct vfscore_file *fp);

/**
 * Gets the next directory entry in the directory stream.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param[out] dirent
 *	Pointer to the directory that will be returned
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_readdir(struct vfscore_file *fp, struct dirent64 *dirent);

/**
 * Resets the location in the directory stream.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_rewinddir(struct vfscore_file *fp);

/**
 * Sets the location in the directory stream from which the
 * next readdir() call will start.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param loc
 *	Location in the directory stream
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_seekdir(struct vfscore_file *fp, long loc);

/**
 * Gets the current location associated fp structure.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param[out] loc
 *	Location in the directory stream
 *
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_telldir(struct vfscore_file *fp, long *loc);

/**
 * Sets the path variable to the current file path from the fp variable.
 * The path variable will be fp->f_dentry->d_path concatenated with
 * fp->f_dentry->d_path.
 *
 * @param fp
 *	Pointer to a vfscore_file structure containing the current working
 *	directory
 * @param[out] path
 *	Path to the destination directory
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fchdir(struct vfscore_file *fp, char *path);

/**
 * Creates a new directory with the name specified by path.
 *
 * @param path
 *	Path to the directory to be created
 * @param mode
 *	Mode bits to be applied when the new directory is created
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_mkdir(char *path, mode_t mode);

/**
 * Removes an empty directory with the name specified by path.
 *
 * @param path
 *	Path to the directory to be removed
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_rmdir(char *path);

/**
 * Creates a filesystem node.
 *
 * @param path
 *	Name of the filesystem node
 * @param mode
 *	Mode bits that specifies file mode to use and the
 *	type of node to be created
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_mknod(char *path, mode_t mode);

/**
 * Changes the name or location of a file.
 *
 * @param src
 *	Old path of the file
 * @param dest
 *	New path of the file
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_rename(char *src, char *dest);

/**
 * Creates a new (hard) link to an existing file. If new path exists, it will
 * not be overwritten.
 *
 * @param oldpath
 *	Old path of the (hard) link
 * @param newpath
 *	New path of the (hard) link
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_link(char *oldpath, char *newpath);

/**
 * Deletes a link (name) from the filesystem. If that name was the last link
 * to  the file, the file is deleted and the space it was using is made
 * available for reuse.
 *
 * @param path
 *	Path to the file to be deleted
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_unlink(char *path);

/**
 * Creates a symbolic link pointing to oldpath with the name newpath.
 *
 * @param oldpath
 *	Path to which the symlink will point to
 * @param newpath
 *	Name of the symlink
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_symlink(const char *oldpath, const char *newpath);

/**
 * Check user's permissions for the file referenced by path.
 * If pathname is a symbolic link, it is dereferenced.
 *
 * @param path
 *	Path to the file to be checked
 * @param mode
 *	The mode specifies the accessibility check(s) to be performed,
 *	and is either the value F_OK, or a mask consisting of the bitwise
 *	OR of one or more of R_OK, W_OK, and X_OK
 *		* F_OK - test the existence of the file
 *		* R_OK - test if file exists with read permissions
 *		* W_OK - test if file exists with write permissions
 *		* X_OK - test if file exists with execute permissions
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_access(char *path, int mode);

/**
 * Gets information about the file pointed to by path.
 *
 * @param path
 *	Path to the file
 * @param[out] st
 *	Buffer to store information about the file
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_stat(char *path, struct stat *st);

/**
 * The sys_lstat() function is equivalent to sys_stat(),
 * except when path refers to a symbolic link.
 * In that case sys_lstat() shall return information about the link,
 * while sys_stat() shall return information about the file
 * the link references.
 *
 * @param path
 *	Path to the file
 * @param[out] st
 *	Buffer to store information about the file
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_lstat(char *path, struct stat *st);

/**
 * Returns information about a mounted filesystem.
 *
 * @param path
 *	Path to any file within the mounted filesystem
 * @param[out] buf
 *	Buffer to store information about the filesystem
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_statfs(char *path, struct statfs *buf);

/**
 * Similar to sys_statfs() function, but it takes a vfscore_file
 * structure as parameter.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param[out] buf
 *	Buffer to store information about the filesystem
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fstatfs(struct vfscore_file *fp, struct statfs *buf);

/**
 * Truncates a file to a specific length. If the file previously was larger
 * than this size, the extra data is lost. If the file was previously shorter,
 * it is extended, and the extended part reads as null bytes ('\0').
 *
 * @param path
 *	Path to the file to be truncated
 * @param length
 *	The new size (in bytes) of the file.
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_truncate(char *path, off_t length);

/**
 * Similar to the sys_truncate() function, but it takes a vfscore_file
 * instead of path.
 *
 * @param fp
 *	Pointer to the vfscore_file
 * @param length
 *	The new size (in bytes) of the file to which fp is referring to
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_ftruncate(struct vfscore_file *fp, off_t length);

/**
 * Store at most bufsize bytes from the content to which the symlink
 * at path is pointing in the char array buf. The number of bytes written
 * in buf is stored in size.
 *
 * @param path
 *	Path to the file
 * @param[out] buf
 *	Pre-allocated buffer to store the content
 * @param bufsize
 *	Size of buf char array
 * @param[out] size
 *	Number of bytes written in buf
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_readlink(char *path, char *buf, size_t bufsize, ssize_t *size);

/**
 * Changes file last access and modification times.
 *
 * @param path
 *	Path to the file
 * @param times
 *	This should be an array with 2 elements: times[0] should point to
 *	the new last accessed timestamp, whereas times[1] should point to
 *	the last modification time.
 * @param flags
 *	If the flag is set to AT_SYMLINK_NOFOLLOW, symlink is not followed.
 *	Otherwise, the flags parameter should be set to 0.
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_utimes(char *path, const struct timeval *times, int flags);

/**
 * Changes last access and modification times with nanoseconds
 * precision for the file provided by path. If the path is relative,
 * then it is interpreted relative to the directory referred to by the
 * open file descriptor, dirfd. If the path is absolute, then
 * the dirfd is ignored.
 *
 * @param dirfd
 *	File descriptor of the relative directory. If this value is set to
 *	AT_FDCWD, then the pathname is considered to be relative to the current
 *	working directory.
 * @param pathname
 *	Path to the file for which to change the timestamps
 * @param times
 *	This should be an array with 2 elements: times[0] should point to
 *	the new last accessed timestamp, whereas times[1] should point to
 *	the last modification time.
 * @param flags
 *	If the flag is set to AT_SYMLINK_NOFOLLOW, symlink is not followed.
 *	Otherwise, the flag should be set to 0.
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_utimensat(int dirfd, const char *pathname,
				   const struct timespec times[2], int flags);

/**
 * Similar to sys_utimensat(), but the file to be updated is specified via
 * an open file descriptor fd.
 *
 * @param fd
 *	An open file descriptor pointing to the file whose timestamp will
 *	be updated
 * @param times
 *	The structure containing the new timestamps
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_futimens(int fd, const struct timespec times[2]);

/**
 * Manipulates the file space for the file referred by fd, starting
 * at offset and continuing for len bytes.
 *
 * @param fp
 *	Pointer to a vfscore_file structure
 * @param mode
 *	Specifies how to manipulates the space. The mode can have the
 *	the following values:
 *		* 0 - allocates len bytes starting from offset.
 *		* FALLOC_FL_KEEP_SIZE - the file size will not change even
 *			if offset + len is greater than the file size.
 *		* FALLOC_FL_PUNCH_HOLE - deallocates len bytes starting at
 *			offset and creates a hole in the file. This flag should
 *			always come together with FALLOC_FL_KEEP_SIZE, otherwise
 *			the sys_futimens call will fail.
 * @param offset
 *	Starting byte from which this operation will apply
 * @param len
 *	Number of bytes for which this operation will apply
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fallocate(struct vfscore_file *fp, int mode, loff_t offset, loff_t len);

/**
 * This function is not used at this point.
 */
int sys_pivot_root(const char *new_root, const char *old_put);

/**
 * Calls the wrapper Unikraft sync() syscall function.
 */
void sync(void);

/**
 * Changes mode of the file whose pathname is given in pathname.
 *
 * @param path
 *	Path to the file
 * @param mode
 *	The new file mode
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int	sys_chmod(const char *path, mode_t mode);

/**
 * Changes mode of the file specified by the open file descriptor fd.
 *
 * @param fd
 *	An open file descriptor
 * @param mode
 *	The new file mode
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int sys_fchmod(int fd, mode_t mode);

/**
 * Converts to full path from the cwd of task and path.
 *
 * @param t
 *	Pointer to task containing the cwd
 * @param path
 *	The target path
 * @param mode
 *	Unused parameter
 * @param[out] full
 *	Full path to be returned
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int task_conv(struct task *t, const char *path, int mode, char *full);

/**
 * Converts to full path from the wd of task and cpath.
 *
 * @param wd
 *	Working directory
 * @param cpath
 *	The target path
 * @param[out] full
 *	Full path to be returned
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int path_conv(char *wd, const char *cpath, char *full);

/**
 * This function is not implemented or used at this point.
 */
int sec_vnode_permission(char *path);

/**
 * Converts a pathname into a pointer to a dentry.
 *
 * @param path
 *	The full path name
 * @param[out] dpp
 *	Dentry to be returned
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int namei(const char *path, struct dentry **dpp);

/**
 * Resolve a pathname into a pointer to a dentry and a realpath.
 *
 * @param path
 *	The full path name
 * @param[out] dpp
 *	Dentry to be returned
 * @param[out] realpath
 *	if not NULL, return path after resolving all symlinks
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int namei_resolve(const char *path, struct dentry **dpp, char *realpath);

/**
 * Converts the last component in the path to a pointer to a dentry.
 *
 * @param path
 *	The full path name
 * @param ddp
 *	Pointer to dentry of parent
 * @param[out] dp
 *	Dentry to be returned
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int namei_last_nofollow(char *path, struct dentry *ddp, struct dentry **dp);

/**
 * Same as namei_last_nofollow, to be called with the ddp->d_vnode lock held.
 *
 * @param path
 *	The full path name
 * @param ddp
 *	Pointer to dentry of parent
 * @param[out] dp
 *	Dentry to be returned
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int
namei_last_nofollow_locked(char *path, struct dentry *ddp, struct dentry **dp);

/**
 * Searches a pathname.
 * This routine returns a locked directory vnode and file name.
 *
 * @param path
 *	Full path to the file
 * @param[out] dpp
 *	Pointer to the dentry for the directory
 * @param[out] name
 *	If non-null, pointer to file name in path
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int lookup(char *path, struct dentry **dpp, char **name);

/**
 * Initializes the vnode buckets of the vnode table.
 * It is called once (from vfscore_init()) in initialization.
 */
void vnode_init(void);

/**
 * Calls dentry_init().
 */
void lookup_init(void);

/**
 * Gets the root directory and mount point for specified path.
 *
 * @param path
 *	Full path
 * @param[out] mp
 *	Pointer to the mount point
 * @param[out] root
 *	Pointer to root directory in path
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int vfs_findroot(const char *path, struct mount **mp, char **root);

/**
 * Copies size bytes from the src string into the destination
 * with bounds-checking.
 *
 * @param dest
 *	Destination string
 * @param src
 *	Source string
 * @param size
 *	Number of bytes to be copied
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int vfs_dname_copy(char *dest, const char *src, size_t size);

/**
 * This function is not used at this point.
 *
 * @return
 *	Always returns 0
 */
int fs_noop(void);

/**
 * Initializes the dentry buckets of the dentry hash table.
 */
void dentry_init(void);

/**
 * Releases the resources associated with the fp.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int vfs_close(struct vfscore_file *fp);

/**
 * Reads data indicated from the descriptor provided by the fp structure into
 * the uio structure. The number of bytes to be read and the buffer for storing
 * the data should be set in the uio structure beforehand.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param[in,out] uio
 *	Pointer to the structure containing information about request
 *	and response
 * @param flags
 *	If the FOF_OFFSET is not set, then the read operation will begin from
 *	fp->f_offset. Otherwise, it will begin from the position specified
 *	at uio->uio_offset.
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int vfs_read(struct vfscore_file *fp, struct uio *uio, int flags);

/**
 * Writes data contained in the uio structure to the descriptor provided
 * by the fp structure. The number of bytes to be written and the buffers
 * containing data should be set in the uio structure beforehand.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param[in,out] uio
 *	Pointer to the structure containing information about request
 *	and response
 * @param flags
 *	If the FOF_OFFSET is not set, then the read operation will begin from
 *	fp->f_offset. Otherwise, it will begin from the position specified
 *	at uio->uio_offset.
 *	If O_APPEND is set, the writing will be at the end of the file.
 *	If O_DSYNC or O_SYNC is set, then this function will return only
 *	when the contents of the file have been written to disk.
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int vfs_write(struct vfscore_file *fp, struct uio *uio, int flags);

/**
 * Modifies device parameters of special files.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param com
 *	Device-dependent request code
 * @param data
 *	Pointer to the data that should be used by call
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int vfs_ioctl(struct vfscore_file *fp, unsigned long com, void *data);

/**
 * Gets information about the file pointed by fp structure.
 *
 * @param fp
 *	Pointer to the vfscore_file structure
 * @param[out] st
 *	Buffer to store information about the file
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int vfs_stat(struct vfscore_file *fp, struct stat *st);

/**
 * Gets the vfscore_file associated with a file descriptor.
 *
 * @param fd
 *	An open file descriptor
 * @param[out] out_fp
 *	Pointer to a vfscore_file structure
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int fget(int fd, struct vfscore_file **out_fp);

/**
 * Allocates a new file descriptor referring the fp structure.
 *
 * @param fp
 *	Pointer to the vfscore_file structure to be referred
 * @param[out] newfd
 *	The new allocated file descriptor pointing to the fp structure
 * @return
 *	- (0):  Completed successfully
 *	- (<0): Negative value with error code
 */
int fdalloc(struct vfscore_file *fp, int *newfd);

#ifdef DEBUG_VFS

/**
 * Outputs a dump for all available vnodes.
 */
void vnode_dump(void);

/**
 * Outputs a dump for all mounted devices.
 */
void vfscore_mount_dump(void);
#endif

#endif /* !_VFS_H */
