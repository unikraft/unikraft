/* SPDX-License-Identifier: ISC AND BSD-2-Clause-NetBSD */
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
/* Taken from solo5 clock_subr.h */

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

#ifndef __UKTIMECONV_H__
#define __UKTIMECONV_H__

#include <uk/arch/time.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* Some handy constants. */
#define __SECS_PER_MINUTE        60
#define __SECS_PER_HOUR          3600
#define __SECS_PER_DAY           86400
#define __DAYS_PER_COMMON_YEAR   365
#define __DAYS_PER_LEAP_YEAR     366

struct uktimeconv_bmkclock {
	__s64 dt_year;
	__u8  dt_mon;
	__u8  dt_day;
	__u8  dt_hour;
	__u8  dt_min;
	__u8  dt_sec;
};

int uktimeconv_is_leap_year(__s64 year);
__u8 uktimeconv_days_in_month(__u8 month, int is_leap_year);

static inline __u16 uktimeconv_days_per_year(__s64 year)
{
	return uktimeconv_is_leap_year(year)
		? __DAYS_PER_LEAP_YEAR
		: __DAYS_PER_COMMON_YEAR;
}

/*
 * "POSIX time" from "YY/MM/DD/hh/mm/ss"
 */
__nsec uktimeconv_bmkclock_to_nsec(struct uktimeconv_bmkclock *dt);

/*
 * BCD to binary.
 */
static inline unsigned int uktimeconv_bcdtobin(unsigned int bcd)
{
	return ((bcd >> 4) & 0x0f) * 10 + (bcd & 0x0f);
}

#ifdef	__cplusplus
}
#endif

#endif /* __UKTIMECONV_H__ */
