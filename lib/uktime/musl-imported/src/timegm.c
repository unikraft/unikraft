#define _GNU_SOURCE
#include <uk/config.h>
#include "time_impl.h"
#include <errno.h>

const char __utc[] = "UTC";

time_t timegm(struct tm *tm)
{
	struct tm new;
	long long t = __tm_to_secs(tm);

	if (__secs_to_tm(t, &new) < 0) {
		errno = EOVERFLOW;
		return -1;
	}
	*tm = new;
	tm->tm_isdst = 0;
#ifndef CONFIG_LIBNEWLIBC
	tm->__tm_gmtoff = 0;
	tm->__tm_zone = __utc;
#endif
	return t;
}
