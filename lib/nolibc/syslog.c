/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marco Schlumpp <marco.schlumpp@gmail.com>
 *
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
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
 */

#include "syslog.h"

#include <stdio.h>
#include <uk/print.h>

static int log_facility = LOG_USER;

static const int level_map[] = {
	KLVL_CRIT, /* LOG_EMERG */
	KLVL_CRIT, /* LOG_ALERT */
	KLVL_CRIT, /* LOG_CRIT */
	KLVL_ERR,  /* LOG_ERR */
	KLVL_WARN, /* LOG_WARNING */
	KLVL_INFO, /* LOG_NOTICE */
	KLVL_INFO, /* LOG_INFO */

	/* This one maps to a different macro on unikraft */
	/* KLVL_INFO, */ /* LOG_DEBUG */
};

static const char *facility_to_str(int facility)
{
	switch (facility) {
	case LOG_AUTH:
		return "AUTH";
	case LOG_AUTHPRIV:
		return "AUTHPRIV";
	case LOG_CRON:
		return "CRON";
	case LOG_DAEMON:
		return "DAEMON";
	case LOG_FTP:
		return "FTP";
	case LOG_KERN:
		return "KERN";
	case LOG_LOCAL0:
		return "LOCAL0";
	case LOG_LOCAL1:
		return "LOCAL1";
	case LOG_LOCAL2:
		return "LOCAL2";
	case LOG_LOCAL3:
		return "LOCAL3";
	case LOG_LOCAL4:
		return "LOCAL4";
	case LOG_LOCAL5:
		return "LOCAL5";
	case LOG_LOCAL6:
		return "LOCAL6";
	case LOG_LOCAL7:
		return "LOCAL7";
	case LOG_LPR:
		return "LPR";
	case LOG_MAIL:
		return "MAIL";
	case LOG_NEWS:
		return "NEWS";
	case LOG_SYSLOG:
		return "SYSLOG";
	case LOG_USER:
		return "USER";
	case LOG_UUCP:
		return "UUCP";
	default:
		return "???";
	}
}

void openlog(const char *ident __unused, int option __unused, int facility)
{
	log_facility = facility;
}

void syslog(int priority, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsyslog(priority, format, ap);
	va_end(ap);
}

void closelog(void)
{}

void vsyslog(int priority, const char *format, va_list ap)
{
	int facility;

	/* Prepare arguments */
	facility = priority & ~LOG_PRIMASK;
	if (facility == 0)
		facility = log_facility;
	priority &= LOG_PRIMASK;

	/* Forward call */
	if (priority == LOG_DEBUG) {
		uk_printd("[%s] ", facility_to_str(facility));
		uk_printd(format, ap);
	} else {
		priority = MAX(priority, 0);
		priority = MIN(priority, (int)ARRAY_SIZE(level_map));
		uk_printk(level_map[priority], "[%s] ",
			  facility_to_str(facility));
		uk_printk(level_map[priority], format, ap);
	}
}
