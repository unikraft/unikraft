/* SPDX-License-Identifier: ISC AND BSD-2-Clause-NetBSD AND BSD-3-Clause */
/*
 * Authors: Martin Lucina
 *          Ricardo Koller
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Taken from solo5 clock_subr.c */

/*	$NetBSD: clock_subr.c,v 1.26 2014/12/22 18:09:20 christos Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */

#include <uk/timeconv.h>
#include <uk/assert.h>

#define SECS_PER_COMMON_YEAR    (__SECS_PER_DAY * DAYS_PER_COMMON_YEAR)
#define SECS_PER_LEAP_YEAR      (__SECS_PER_DAY * DAYS_PER_LEAP_YEAR)

#define FEBRUARY  2

/* Traditional POSIX base year */
#define	POSIX_BASE_YEAR	1970

/* Some handy functions */
static __u8 _days_in_month(__u8 m)
{
	switch (m) {
	case FEBRUARY:
		return 28;
	case 4: case 6: case 9: case 11:
		return 30;
	case 1: case 3: case 5: case 7: case 8: case 10: case 12:
		return 31;
	default:
		/* m out of range */
		return 0;
	}
}

__u8 uktimeconv_days_in_month(__u8 month, int is_leap_year)
{
	__u8 days;

	days = _days_in_month(month);
	if (is_leap_year && month == FEBRUARY)
		++days;

	return days;
}

/*
 * This function avoids some unnecessary modulo
 * operations as compared with the usual macro:
 *   ( ((year % 4) == 0 &&
 *      (year % 100) != 0) ||
 *     ((year % 400) == 0) )
 * It is otherwise equivalent.
 */
int uktimeconv_is_leap_year(__s64 year)
{
	if ((year & 3) != 0)
		return 0;

	if ((year % 100) != 0)
		return 1;

	return (year % 400) == 0;
}


/*
 * Generic routines to convert between a POSIX date
 * (seconds since 1/1/1970) and yr/mo/day/hr/min/sec
 * Derived from arch/hp300/hp300/clock.c
 */

/* for easier alignment:
 * time from the epoch to 2000 (there were 7 leap years):
 */
#define	DAYSTO2000         (365 * 30 + 7)

/* 4 year intervals include 1 leap year */
#define	DAYS4YEARS         (365 * 4 + 1)

/* 100 year intervals include 24 leap years */
#define	DAYS100YEARS       (365 * 100 + 24)

/* 400 year intervals include 97 leap years */
#define	DAYS400YEARS       (365 * 400 + 97)

__nsec uktimeconv_bmkclock_to_nsec(struct uktimeconv_bmkclock *dt)
{
	__u64 secs, year, days, i;

	UK_ASSERT(dt->dt_year >= POSIX_BASE_YEAR);

	/*
	 * Compute days since start of time
	 * First from years, then from months.
	 */
	year = (__u64) dt->dt_year;
	days = 0;
	if (uktimeconv_is_leap_year(year) && dt->dt_mon > FEBRUARY)
		days++;

	if (year < 2000) {
		/* simple way for early years */
		for (i = POSIX_BASE_YEAR; i < year; i++)
			days += uktimeconv_days_per_year(i);

	} else {
		/* years are properly aligned */
		days += DAYSTO2000;
		year -= 2000;

		i = year / 400;
		days += i * DAYS400YEARS;
		year -= i * 400;

		i = year / 100;
		days += i * DAYS100YEARS;
		year -= i * 100;

		i = year / 4;
		days += i * DAYS4YEARS;
		year -= i * 4;

		for (i = dt->dt_year - year; i < (__u64) dt->dt_year; i++)
			days += uktimeconv_days_per_year(i);
	}

	/* Months */
	for (i = 1; i < dt->dt_mon; i++)
		days += _days_in_month(i);
	days += (dt->dt_day - 1);

	/* Add hours, minutes, seconds. */
	secs = ((days
		* 24 + dt->dt_hour)
		* 60 + dt->dt_min)
		* 60 + dt->dt_sec;

	return ukarch_time_sec_to_nsec((__nsec) secs);
}
