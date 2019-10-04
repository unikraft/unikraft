/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * libnewlib glue code
 *
 * Authors: Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <time.h>
#include <uk/plat/time.h>
#include <errno.h>
#include <uk/config.h>
#if CONFIG_HAVE_SCHED
#include <uk/sched.h>
#else
#include <uk/plat/lcpu.h>
#endif
#include <uk/essentials.h>

#ifndef CONFIG_HAVE_SCHED
/* Workaround until Unikraft changes interface for something more
 * sensible
 */
static void __spin_wait(__nsec nsec)
{
	__nsec until = ukplat_monotonic_clock() + nsec;

	while (until > ukplat_monotonic_clock())
		ukplat_lcpu_halt_to(until);
}
#endif

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	__nsec before, after, diff, nsec;

	if (!req || req->tv_nsec < 0 || req->tv_nsec > 999999999) {
		errno = EINVAL;
		return -1;
	}

	nsec = (__nsec) req->tv_sec * 1000000000L;
	nsec += req->tv_nsec;
	before = ukplat_monotonic_clock();

#if CONFIG_HAVE_SCHED
	uk_sched_thread_sleep(nsec);
#else
	__spin_wait(nsec);
#endif

	after = ukplat_monotonic_clock();
	diff = after - before;

	if (diff < nsec) {
		if (rem) {
			rem->tv_sec = ukarch_time_nsec_to_sec(nsec - diff);
			rem->tv_nsec = ukarch_time_subsec(nsec - diff);
		}
		errno = EINTR;
		return -1;
	}
	return 0;
}

unsigned int sleep(unsigned int seconds)
{
	struct timespec ts;

	ts.tv_sec = seconds;
	ts.tv_nsec = 0;
	if (nanosleep(&ts, &ts))
		return ts.tv_sec;

	return 0;
}

int gettimeofday(struct timeval *tv, void *tz __unused)
{
	__nsec now = ukplat_wall_clock();

	if (!tv) {
		errno = EINVAL;
		return -1;
	}

	tv->tv_sec = ukarch_time_nsec_to_sec(now);
	tv->tv_usec = ukarch_time_nsec_to_usec(ukarch_time_subsec(now));
	return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	__nsec now;

	if (!tp) {
		errno = EFAULT;
		return -1;
	}

	switch (clk_id) {
	case CLOCK_MONOTONIC:
		now = ukplat_monotonic_clock();
		break;
	case CLOCK_REALTIME:
		now = ukplat_wall_clock();
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	tp->tv_sec = ukarch_time_nsec_to_sec(now);
	tp->tv_nsec = ukarch_time_subsec(now);
	return 0;
}
