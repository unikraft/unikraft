/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Costin Lupu <costin.lupu@cs.pub.ro>
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

#include <string.h>
#include <uk/plat/time.h>
#include <uk/plat/irq.h>
#include <uk/assert.h>
#include <linuxu/syscall.h>
#include <linuxu/time.h>

static k_timer_t timerid;


__nsec ukplat_monotonic_clock(void)
{
	struct k_timespec tp;
	__nsec ret;
	int rc;

	rc = sys_clock_gettime(K_CLOCK_MONOTONIC, &tp);
	if (unlikely(rc != 0))
		return 0;

	ret = ukarch_time_sec_to_nsec((__nsec) tp.tv_sec);
	ret += (__nsec) tp.tv_nsec;

	return ret;
}

__nsec ukplat_wall_clock(void)
{
	struct k_timespec tp;
	__nsec ret;
	int rc;

	rc = sys_clock_gettime(K_CLOCK_REALTIME, &tp);
	if (unlikely(rc != 0))
		return 0;

	ret = ukarch_time_sec_to_nsec((__nsec) tp.tv_sec);
	ret += (__nsec) tp.tv_nsec;

	return ret;
}

static int timer_handler(void *arg __unused)
{
	/* We only use the timer interrupt to wake up. As we end up here, the
	 * timer interrupt has already done its job and we can acknowledge
	 * receiving it.
	 */
	return 1;
}

void ukplat_time_init(void)
{
	struct uk_sigevent sigev;
	struct k_itimerspec its;
	int rc;

	ukplat_irq_register(TIMER_SIGNUM, timer_handler, NULL);

	memset(&sigev, 0, sizeof(sigev));
	sigev.sigev_notify = 0;
	sigev.sigev_signo = TIMER_SIGNUM;
	sigev.sigev_value.sival_ptr = &timerid;

	rc = sys_timer_create(K_CLOCK_REALTIME, &sigev, &timerid);
	if (unlikely(rc != 0))
		UK_CRASH("Failed to create timer: %d\n", rc);

	/* Initial expiration */
	its.it_value.tv_sec  = TIMER_INTVAL_NSEC / ukarch_time_sec_to_nsec(1);
	its.it_value.tv_nsec = TIMER_INTVAL_NSEC % ukarch_time_sec_to_nsec(1);
	/* Timer interval */
	its.it_interval = its.it_value;

	rc = sys_timer_settime(timerid, 0, &its, NULL);
	if (unlikely(rc != 0))
		UK_CRASH("Failed to setup timer: %d\n", rc);
}

void ukplat_time_fini(void)
{
	sys_timer_delete(timerid);
}
