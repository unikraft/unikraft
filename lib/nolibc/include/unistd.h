/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *
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

#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_NULL
#define __NEED_pid_t
#define __NEED_size_t
#define __NEED_ssize_t
#define __NEED_off_t
#define __NEED_useconds_t

#include <nolibc-internal/shareddefs.h>

/*
 * Sysconf name values
 */
#if CONFIG_LIBPOSIX_SYSINFO
#define _SC_OPEN_MAX          4
#define _SC_PAGE_SIZE        30
#define _SC_PAGESIZE         _SC_PAGE_SIZE
#define _SC_GETPW_R_SIZE_MAX 70
#define _SC_PHYS_PAGES       85
#define _SC_AVPHYS_PAGES     86
#define _SC_NPROCESSORS_CONF 83
#define _SC_NPROCESSORS_ONLN 84

long sysconf(int);
int gethostname(char *name, size_t len);
int sethostname(const char *name, size_t len);
size_t confstr(int name, char *buf, size_t len);
long fpathconf(int fd, int name);
long pathconf(const char *path, int name);
#endif

#if CONFIG_HAVE_TIME
unsigned int sleep(unsigned int seconds);
#endif

#if CONFIG_LIBPOSIX_PROCESS
int execl(const char *path, const char *arg, ...
		/* (char  *) NULL */);
int execlp(const char *file, const char *arg, ...
		/* (char  *) NULL */);
int execle(const char *path, const char *arg, ...
		/*, (char *) NULL, char * const envp[] */);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[],
		char *const envp[]);
int execve(const char *filename, char *const argv[],
		char *const envp[]);
pid_t vfork(void);
pid_t getpgid(pid_t pid);
pid_t getpgrp(void);
pid_t getsid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);
pid_t setsid(void);
pid_t getpid(void);
pid_t getppid(void);
#if UK_LIBC_SYSCALLS
int setpgrp(void);
int nice(int inc);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int df, pid_t pgrp);
#endif /* UK_LIBC_SYSCALLS */

#endif /* CONFIG_LIBPOSIX_PROCESS */

#if CONFIG_LIBVFSCORE
int close(int fd);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
ssize_t read(int fd, void *buf, size_t count);
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
void sync(void);
int fsync(int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);
int unlink(const char *pathname);
off_t lseek(int fd, off_t offset, int whence);
int chdir(const char *path);
int fchdir(int fd);
int rmdir(const char *pathname);
int access(const char *pathname, int mode);
int chown(const char *path, uid_t owner, gid_t group);
int faccessat(int dirfd, const char *pathname, int mode, int flags);
int lchown(const char *path, uid_t owner, gid_t group);
int link(const char *oldpath, const char *newpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsize);
ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsize);
int unlinkat(int dirfd, const char *pathname, int flags);

#else /* !CONFIG_LIBVFSCORE */

#if CONFIG_LIBPOSIX_FDTAB
int close(int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);

#if CONFIG_LIBPOSIX_FDIO
ssize_t write(int fd, const void *buf, size_t count);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
ssize_t read(int fd, void *buf, size_t count);
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
int fsync(int fd);
int fdatasync(int fd);
off_t lseek(int fd, off_t offset, int whence);
int ftruncate(int fd, off_t length);
int fchown(int fd, uid_t owner, gid_t group);
#endif /* CONFIG_LIBPOSIX_FDIO */

#if CONFIG_LIBPOSIX_VFS_SYSCALLS
void sync(void);
int unlink(const char *pathname);
int chdir(const char *path);
int fchdir(int fd);
int chroot(const char *path);
int rmdir(const char *pathname);
char *getcwd(char *buf, size_t size);
int symlink(const char *path, const char *linkpath);
int truncate(const char *path, off_t length);
int fchownat(int dfd, const char *path, uid_t owner, gid_t group, int flags);
int linkat(int oldfd, const char *oldname,
	   int newfd, const char *newname, int flags);
int symlinkat(const char *path, int dfd, const char *linkname);
#endif /* CONFIG_LIBPOSIX_VFS_SYSCALLS */

#if CONFIG_LIBPOSIX_PIPE
int pipe(int *pipefd);
#endif
#endif /* CONFIG_LIBPOSIX_FDTAB */

#endif /* !CONFIG_LIBVFSCORE */

#if CONFIG_LIBUKSIGNAL
unsigned int alarm(unsigned int seconds);
int pause(void);
#endif /* CONFIG_LIBUKSIGNAL */

#if CONFIG_LIBPOSIX_USER
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int size, gid_t *list);
char *getlogin(void);
int getlogin_r(char *buf, size_t bufsize);
int setegid(gid_t egid);
int setregid(gid_t rgid, gid_t egid);
int setgid(gid_t gid);
uid_t getuid(void);
int seteuid(uid_t euid);
int setreuid(uid_t ruid, uid_t euid);
int setuid(uid_t uid);
#endif /* CONFIG_LIBPOSIX_USER */

#define STDIN_FILENO	0	/* standard input file descriptor */
#define STDOUT_FILENO	1	/* standard output file descriptor */
#define STDERR_FILENO	2	/* standard error file descriptor */

#ifdef __cplusplus
}
#endif
#endif /* __UNISTD_H__ */
