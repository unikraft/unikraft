
#include <sys/stat.h>
#include <errno.h>

/*
 * Linux Standard Base (LSB) compatibility functions for stat
 * The 'ver' parameter is used to indicate the version of the stat structure
 * but is currently unused in Unikraft.
 */

#if UK_LIBC_SYSCALLS
 static int __xstat(int ver __unused, const char *pathname,
		    struct stat *st)
 {
    return stat(pathname, st);
 }
 #ifdef __xstat64
 #undef __xstat64
 #endif

 LFS64(__xstat);
 #endif /* UK_LIBC_SYSCALLS */
 #if UK_LIBC_SYSCALLS
 int __fxstatat(int ver __unused, int dirfd, const char *pathname,
		struct stat *st, int flags)
 {
    return fstatat(dirfd, pathname, st, flags);
 }
 #ifdef __fxstatat64
 #undef __fxstatat64
 #endif

 LFS64(__fxstatat);
 #endif /* UK_LIBC_SYSCALLS */
  #if UK_LIBC_SYSCALLS
int __fxstat(int ver __unused, int fd, struct stat *st)
{
    return fstat(fd, st);
}

#ifdef __fxstat64
#undef __fxstat64
#endif

LFS64(__fxstat);
#endif /* UK_LIBC_SYSCALLS */
#if UK_LIBC_SYSCALLS
 int __lxstat(int ver __unused, const char *pathname, struct stat *st)
 {
    return lstat(pathname, st);
 }

 #ifdef __lxstat64
 #undef __lxstat64
 #endif

 LFS64(__lxstat);
 #else
 int __lxstat(int ver, const char *pathname, struct stat *st);
 #endif /* UK_LIBC_SYSCALLS */
  #if UK_LIBC_SYSCALLS
 int __xmknod(int ver, const char *pathname,
	      mode_t mode, dev_t *dev __unused)
 {
    return mknod(pathname, mode, *dev);
 }
 #endif /* UK_LIBC_SYSCALLS */