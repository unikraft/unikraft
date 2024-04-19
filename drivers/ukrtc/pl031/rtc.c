/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Jianyong Wu <Jianyong.Wu@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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
 */

#include <uk/essentials.h>
#include <uk/rtc.h>

static const unsigned int days_per_mon[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int is_leap_year(unsigned int year)
{
	return ((!(year % 4) && (year % 100)) || !(year % 400));
}

void rtc_raw_to_tm(__u32 raw, struct rtc_time *rt)
{
	unsigned int hours, days, years;
	unsigned int normal_days, day_in_year;
	unsigned int leap, i, sum = 0;

	/*
	 * Total days for every consecutive 4 years, assuming there is a leap
	 * year every 4 years
	 */
	const unsigned int dy4   = 365 * 3 + 366;
	/* Total days for every consecutive 100 years */
	const unsigned int dy100 = 25 * dy4 - 1;
	/* Total days for every consecutive 400 years */
	const unsigned int dy400 = dy100 * 4 + 1;

	rt->sec = raw % 60;
	rt->min = (raw % 3600) / 60;
	hours = (raw / 60) / 60;
	days = hours / 24;
	rt->hour = hours % 24;

	/* Normalize days by getting rid of the additional day in leap year */
	normal_days = days - days / dy4 + days / dy100 + days / dy400;
	years = normal_days / 365;
	rt->year = 1970 + years;

	leap = is_leap_year(rt->year);
	day_in_year = normal_days - years * 365;

	/*
	 * If the remaining number of days is larger than the sum of the first
	 * two months we should consider 29 days for February.
	 */
	sum += leap * (day_in_year >= (days_per_mon[0] + days_per_mon[1]));
	for (i = 0; i < 12; i++) {
		sum += days_per_mon[i];
		if (day_in_year < sum) {
			rt->mon = i + 1;
			rt->day = day_in_year - (sum - days_per_mon[i]) + 1;
			break;
		}
	}
}

__u32 rtc_tm_to_raw(struct rtc_time *rt)
{
	unsigned int leaps, leap, years, days, sec;
	int i;

	years = rt->year - 1970;
	leaps = years / 4 - years / 100 + years / 400;
	leap = is_leap_year(rt->year);

	days = years * 365 + leaps;
	if (rt->mon == 1) {
		days += days_per_mon[0];
	} else {
		for (i = 0; i < rt->mon - 1; i++)
			days += days_per_mon[i];
	}

	days += rt->day + (rt->mon > 2) * leap - 1;
	sec = days * 3600 * 24 + rt->hour * 3600 + rt->min * 60 + rt->sec;

	return sec;
}
