/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, The Unikraft Authors. All rights reserved. */
#ifndef __UK_PROCESS_ITIMER_H__
#define __UK_PROCESS_ITIMER_H__

#include <sys/time.h>

/* User-pointer entry points; copy and validate outside process locks. */
int uk_process_setitimer(int which, const struct itimerval *value,
			 struct itimerval *old);
int uk_process_getitimer(int which, struct itimerval *value);
unsigned int uk_process_alarm(unsigned int seconds);

#endif /* __UK_PROCESS_ITIMER_H__ */
