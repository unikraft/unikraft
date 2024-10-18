/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/print.h>

#if CONFIG_LIBUKPRINT_LOGBUF
#include "logbuf.h"
#endif /* CONFIG_LIBUKPRINT_LOGBUF */
#include "print.h"
#include "snprintf.h"

#if CONFIG_LIBUKPRINT_PRINTK
void _uk_vprintk(int flags, __u16 libid,
		 const char *srcname __maybe_unused,
		 unsigned int srcline __maybe_unused,
		 const char *fmt, va_list ap)
{
	struct uk_print_msg msg;

	UK_PRINT_MSG_ALLOC(msg, flags, libid, srcname, srcline, fmt, ap);

#if CONFIG_LIBUKPRINT_LOGBUF
	uk_logbuf_write(&uk_logbuf_default, &msg);
#endif /* CONFIG_LIBUKPRINT_LOGBUF */

#if CONFIG_LIBUKCONSOLE
	uk_print_console_write(&msg);
#endif /* CONFIG_LIBUKCONSOLE */

	UK_PRINT_MSG_FREE(msg);
}

void _uk_printk(int flags, __u16 libid,
		const char *srcname __maybe_unused,
		unsigned int srcline __maybe_unused,
		const char *fmt, ...)
{
	struct uk_print_msg msg;
	va_list ap;

	va_start(ap, fmt);
	UK_PRINT_MSG_ALLOC(msg, flags, libid, srcname, srcline, fmt, ap);

#if CONFIG_LIBUKPRINT_LOGBUF
	uk_logbuf_write(&uk_logbuf_default, &msg);
#endif /* CONFIG_LIBUKPRINT_LOGBUF */

#if CONFIG_LIBUKCONSOLE
	uk_print_console_write(&msg);
#endif /* CONFIG_LIBUKCONSOLE */

	UK_PRINT_MSG_FREE(msg);
	va_end(ap);
}
#endif /* CONFIG_LIBUKPRINT_PRINTK */
