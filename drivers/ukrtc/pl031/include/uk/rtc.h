/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Jianyong Wu <Jianyong.Wu@arm.com>
 *
 * Copyright (c) 2019, Arm Ltd. All rights reserved.
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

#ifndef __UK_RTC_RTC_H__
#define __UK_RTC_RTC_H__

#include <uk/arch/types.h>

/* Store time in structured format. */
struct rtc_time {
	int year;
	int mon;
	int day;
	int hour;
	int min;
	int sec;
};

/**
 * Convert raw RTC data (read from RTC register) to a time structure.
 *
 * @param raw Raw RTC value
 * @param [inout] tm Pointer to time structure storing the result
 */
void rtc_raw_to_tm(__u32 raw, struct rtc_time *tm);

/**
 * Convert structured time to raw RTC data (to be written to RTC register).
 *
 * @param tm Pointer to time structure
 *
 * @return The raw RTC data
 */
__u32 rtc_tm_to_raw(struct rtc_time *tm);

#endif /* __UK_RTC_RTC_H__ */
