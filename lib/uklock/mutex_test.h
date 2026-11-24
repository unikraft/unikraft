/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UKLOCK_MUTEX_TEST_H__
#define __UKLOCK_MUTEX_TEST_H__

#include <uk/config.h>

struct uk_mutex;

#if CONFIG_LIBUKLOCK_MUTEX_ABA_TEST
void uk_mutex_test_after_mark(struct uk_mutex *m);
#else
#define uk_mutex_test_after_mark(m) do { } while (0)
#endif

#endif /* __UKLOCK_MUTEX_TEST_H__ */
