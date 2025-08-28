#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

long uk_syscall_r_getcwd(char *buf, size_t sz);

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
	long ret = uk_syscall_r_getcwd(p, size);
	if (ret < 0)
		return 0;
	if (ret == 0 || p[0] != '/') {
		errno = ENOENT;
		return 0;
	}
	return buf ? buf : strdup(tmp);
}
