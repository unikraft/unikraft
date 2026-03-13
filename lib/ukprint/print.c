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
#if !__INTERRUPTSAFE__
void _uk_vprintk(int flags, __u16 libid,
		 const char *srcname __maybe_unused,
		 unsigned int srcline __maybe_unused,
		 const char *fmt, va_list ap)
#else /* __INTERRUPTSAFE__ */
void _uk_vprintk_isr(int flags, __u16 libid,
		     const char *srcname __maybe_unused,
		     unsigned int srcline __maybe_unused,
		     const char *fmt, va_list ap)
#endif /* __INTERRUPTSAFE__ */
{
	struct uk_print_msg msg;

	UK_PRINT_MSG_ALLOC(msg, flags, libid, srcname, srcline, fmt, ap);

#if CONFIG_LIBUKPRINT_LOGBUF
#if !__INTERRUPTSAFE__
	uk_logbuf_write(&uk_logbuf_default, &msg);
#else /* __INTERRUPTSAFE__ */
	uk_logbuf_write_isr(&uk_logbuf_default, &msg);
#endif /* __INTERRUPTSAFE__ */
#endif /* CONFIG_LIBUKPRINT_LOGBUF */

#if CONFIG_LIBUKCONSOLE
#if !__INTERRUPTSAFE__
	uk_print_console_write(&msg);
#else /* __INTERRUPTSAFE__ */
	uk_print_console_write_isr(&msg);
#endif /* __INTERRUPTSAFE__ */
#endif /* CONFIG_LIBUKCONSOLE */

	UK_PRINT_MSG_FREE(msg);
}

#if !__INTERRUPTSAFE__
void _uk_printk(int flags, __u16 libid,
		const char *srcname __maybe_unused,
		unsigned int srcline __maybe_unused,
		const char *fmt, ...)
#else /* __INTERRUPTSAFE__ */
void _uk_printk_isr(int flags, __u16 libid,
		    const char *srcname __maybe_unused,
		    unsigned int srcline __maybe_unused,
		    const char *fmt, ...)
#endif /* __INTERRUPTSAFE__ */
{
	struct uk_print_msg msg;
	va_list ap;

	va_start(ap, fmt);
	UK_PRINT_MSG_ALLOC(msg, flags, libid, srcname, srcline, fmt, ap);

#if CONFIG_LIBUKPRINT_LOGBUF
#if !__INTERRUPTSAFE__
	uk_logbuf_write(&uk_logbuf_default, &msg);
#else /* __INTERRUPTSAFE__ */
	uk_logbuf_write_isr(&uk_logbuf_default, &msg);
#endif /* __INTERRUPTSAFE__ */
#endif /* CONFIG_LIBUKPRINT_LOGBUF */

#if CONFIG_LIBUKCONSOLE
#if !__INTERRUPTSAFE__
	uk_print_console_write(&msg);
#else /* __INTERRUPTSAFE__ */
	uk_print_console_write_isr(&msg);
#endif /* __INTERRUPTSAFE__ */
#endif /* CONFIG_LIBUKCONSOLE */

	UK_PRINT_MSG_FREE(msg);
	va_end(ap);
}
#endif /* CONFIG_LIBUKPRINT_PRINTK */
