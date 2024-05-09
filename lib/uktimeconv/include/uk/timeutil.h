/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_TIMEUTIL_H__
#define __UK_TIMEUTIL_H__

#include <sys/time.h>
#include <time.h>

#include <uk/arch/time.h>


#define uk_time_spec_to_nsec(t) \
	(ukarch_time_sec_to_nsec((t)->tv_sec) + (t)->tv_nsec)

#define uk_time_val_to_nsec(tv) ( \
	ukarch_time_sec_to_nsec((tv)->tv_sec) + \
	ukarch_time_usec_to_nsec((tv)->tv_usec) \
)

#define uk_time_spec_from_nsec(ns) ((struct timespec){ \
	.tv_sec = ukarch_time_nsec_to_sec((ns)), \
	.tv_nsec = ukarch_time_subsec((ns)) \
})

#define uk_time_spec_from_msec(ms) \
	uk_time_spec_from_nsec(ukarch_time_msec_to_nsec((ms)))

#define uk_time_spec_from_val(tv) ((struct timespec){ \
	.tv_sec = (tv)->tv_sec, \
	.tv_nsec = ukarch_time_usec_to_nsec((tv)->tv_usec) \
})

#define uk_time_val_from_spec(ts) ((struct timeval){ \
	.tv_sec = (ts)->tv_sec, \
	.tv_usec = ukarch_time_nsec_to_usec((ts)->tv_nsec) \
})

static inline
__snsec uk_time_spec_nsecdiff(const struct timespec *t1,
			      const struct timespec *t2)
{
	return (t2->tv_sec - t1->tv_sec) * UKARCH_NSEC_PER_SEC +
	       t2->tv_nsec - t1->tv_nsec;
}

static inline
struct timespec uk_time_spec_sum(const struct timespec *t1,
				 const struct timespec *t2)
{
	struct timespec ret;

	ret.tv_sec = t1->tv_sec + t2->tv_sec;
	ret.tv_nsec = t1->tv_nsec + t2->tv_nsec;
	if (ret.tv_nsec > (long)UKARCH_NSEC_PER_SEC) {
		ret.tv_sec += 1;
		ret.tv_nsec -= UKARCH_NSEC_PER_SEC;
	}
	return ret;
}

#endif /*__UK_TIMEUTIL_H__ */
