#ifndef	_UTIME_H
#define	_UTIME_H

#include <uk/config.h>

#if CONFIG_LIBVFSCORE || CONFIG_LIBPOSIX_VFS

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_time_t

#include <sys/types.h>

struct utimbuf {
	time_t actime;
	time_t modtime;
};

#if CONFIG_LIBVFSCORE

int utime (const char *, const struct utimbuf *);

#endif /* CONFIG_LIBVFSCORE */

#endif /* CONFIG_LIBVFSCORE || CONFIG_LIBPOSIX_VFS */

#ifdef __cplusplus
}
#endif

#endif
