#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

/* Includes for the internal `getcwd` syscall */
#if CONFIG_LIBPOSIX_VFS
#include <uk/posix-vfs.h>
#else /* !CONFIG_LIBPOSIX_VFS */
#error No suitable VFS stack enabled in config
#endif /* !CONFIG_LIBPOSIX_VFS */

char *getcwd(char *buf, size_t size)
{
	char tmp[buf ? 1 : PATH_MAX];
	char *p = buf;
	if (!buf) {
		p = tmp;
		size = sizeof tmp;
	} else if (!size) {
		errno = EINVAL;
		return 0;
	}
	long ret = uk_sys_getcwd(p, size);
	if (ret < 0)
		return 0;
	if (ret == 0 || p[0] != '/') {
		errno = ENOENT;
		return 0;
	}
	return buf ? buf : strdup(tmp);
}
