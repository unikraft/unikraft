/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __SYSLOG_H__
#define __SYSLOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdarg.h"

#define LOG_CONS	0x01
#define LOG_NDELAY	0x02
#define LOG_NOWAIT	0x04
#define LOG_ODELAY	0x08
#define LOG_PERROR	0x10
#define LOG_PID		0x20

#define LOG_AUTH	(1 << 3)
#define LOG_AUTHPRIV	(2 << 3)
#define LOG_CRON	(3 << 3)
#define LOG_DAEMON	(4 << 3)
#define LOG_FTP		(5 << 3)
#define LOG_KERN	(6 << 3)
#define LOG_LOCAL0	(7 << 3)
#define LOG_LOCAL1	(8 << 3)
#define LOG_LOCAL2	(9 << 3)
#define LOG_LOCAL3	(10 << 3)
#define LOG_LOCAL4	(11 << 3)
#define LOG_LOCAL5	(12 << 3)
#define LOG_LOCAL6	(13 << 3)
#define LOG_LOCAL7	(14 << 3)
#define LOG_LPR		(15 << 3)
#define LOG_MAIL	(16 << 3)
#define LOG_NEWS	(17 << 3)
#define LOG_SYSLOG	(18 << 3)
#define LOG_USER	(19 << 3)
#define LOG_UUCP	(20 << 3)

#define LOG_EMERG	0
#define LOG_ALERT	1
#define LOG_CRIT	2
#define LOG_ERR		3
#define LOG_WARNING	4
#define LOG_NOTICE	5
#define LOG_INFO	6
#define LOG_DEBUG	7

#define LOG_PRIMASK	0x7

#define LOG_MAKEPRI(FAC, PRI) ((FAC) | (PRI))

void openlog(const char *ident, int option, int facility);

void syslog(int priority, const char *format, ...);

void closelog(void);

void vsyslog(int priority, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* __SYSLOG_H__ */
