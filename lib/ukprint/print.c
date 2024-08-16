/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "snprintf.h"
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>

#include <uk/essentials.h>
#include <uk/plat/console.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/arch/lcpu.h>
#if CONFIG_LIBUKPRINT_PRINT_THREAD
#include <uk/thread.h>
#endif

#if CONFIG_LIBUKCONSOLE
#include <uk/console.h>
#include <errno.h>
#endif /* CONFIG_LIBUKCONSOLE */

#if CONFIG_LIBUKPRINT_ANSI_COLOR
#define LVLC_RESET	UK_ANSI_MOD_RESET
#define LVLC_TS		UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_GREEN)
#define LVLC_CALLER	UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define LVLC_THREAD	UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLUE)
#define LVLC_LIBNAME	UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_YELLOW)
#define LVLC_SRCNAME	UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_CYAN)
#define LVLC_DEBUG	UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE)
#define LVLC_KERN	UK_ANSI_MOD_BOLD \
			UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLUE)
#define LVLC_INFO	UK_ANSI_MOD_BOLD \
			UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_GREEN)
#define LVLC_WARN	UK_ANSI_MOD_BOLD \
			UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_YELLOW)
#define LVLC_ERROR	UK_ANSI_MOD_BOLD \
			UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define LVLC_ERROR_MSG	UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define LVLC_CRIT	UK_ANSI_MOD_BOLD \
			UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE) \
			UK_ANSI_MOD_COLORBG(UK_ANSI_COLOR_RED)

#define LVLC_CRIT_MSG	UK_ANSI_MOD_BOLD \
			UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#else
#define LVLC_RESET	""
#define LVLC_TS		""
#define LVLC_CALLER	""
#define LVLC_THREAD	""
#define LVLC_LIBNAME	""
#define LVLC_SRCNAME	""
#define LVLC_DEBUG	""
#define LVLC_KERN	""
#define LVLC_INFO	""
#define LVLC_WARN	""
#define LVLC_ERROR	""
#define LVLC_ERROR_MSG	""
#define LVLC_CRIT	""
#define LVLC_CRIT_MSG	""
#endif /* !CONFIG_LIBUKPRINT_ANSI_COLOR */

#define BUFLEN 192

typedef __ssz (*cout_func)(const char *buf, __sz len);

struct vprint_console {
	cout_func cout;
	int newline;
	int prevlvl;
};

#if CONFIG_LIBUKCONSOLE
/* TODO: Some consoles require both a newline and a carriage return to
 * go to the start of the next line. This kind of behavior should be in
 * a single place in posix-tty. We keep this workaround until we have feature
 * in posix-tty that handles newline characters correctly.
 */
static inline __ssz console_out(const char *buf, __sz len)
{
	const char *next_nl = NULL;
	__sz l = len;
	__sz off = 0;
	__ssz rc = 0;

	if (unlikely(!len))
		return 0;
	if (unlikely(!buf))
		return -EINVAL;

	while (l > 0) {
		next_nl = memchr(buf, '\n', l);
		if (next_nl) {
			off = next_nl - buf;
			if ((rc = uk_console_out(buf, off)) < 0)
				return rc;
			if ((rc = uk_console_out("\r\n", 2)) < 0)
				return rc;
			buf = next_nl + 1;
			l -= off + 1;
		} else {
			if ((rc = uk_console_out(buf, l)) < 0)
				return rc;
			break;
		}
	}

	return len;
}
#endif /* CONFIG_LIBUKCONSOLE */

/* Console state for kernel output */
static struct vprint_console kern  = {
#if CONFIG_LIBUKCONSOLE
	.cout = console_out,
#else
	.cout = NULL,
#endif /* CONFIG_LIBUKCONSOLE */
	.newline = 1,
	.prevlvl = INT_MIN
};

static inline void vprint_cout(struct vprint_console *cons,
			       const char *buf, __sz len)
{
	if (cons->cout)
		cons->cout(buf, len);
}

#if CONFIG_LIBUKPRINT_PRINT_TIME
static void print_timestamp(struct vprint_console *cons)
{
	char buf[BUFLEN];
	int len;
	__nsec nansec =  ukplat_monotonic_clock();
	__nsec sec = ukarch_time_nsec_to_sec(nansec);
	__nsec rem_usec = ukarch_time_subsec(nansec);

	rem_usec = ukarch_time_nsec_to_usec(rem_usec);
	len = uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_TS
			  "[%5" __PRInsec ".%06" __PRInsec "] ",
			  sec, rem_usec);
	vprint_cout(cons, (char *)buf, len);
}
#endif

#if CONFIG_LIBUKPRINT_PRINT_THREAD
static void print_thread(struct vprint_console *cons)
{
	struct uk_thread *t = uk_thread_current();
	char buf[BUFLEN];
	int len;

	if (t) {
		if (t->name) {
			len = uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_THREAD
					  "<%s> ", t->name);
		} else {
			len = uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_THREAD
					  "<%p> ", (void *)t);
		}
	} else {
		len = uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_THREAD
				  "<<n/a>> ");
	}
	vprint_cout(cons, (char *)buf, len);
}
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */

#if CONFIG_LIBUKPRINT_PRINT_CALLER
static void print_caller(struct vprint_console *cons, __uptr ra, __uptr fa)
{
	char buf[BUFLEN];
	int len;

	len = uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_CALLER
			  "{r:%p,f:%p} ", (void *)ra, (void *)fa);
	vprint_cout(cons, (char *)buf, len);
}
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */

static void vprint(struct vprint_console *cons,
		   int flags, __u16 libid,
#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
		   const char *srcname,
		   unsigned int srcline,
#endif /* CONFIG_LIBUKPRINT_PRINT_SRCNAME */
#if CONFIG_LIBUKPRINT_PRINT_CALLER
		   __uptr retaddr,
		   __uptr frameaddr,
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */
		   const char *fmt, va_list ap)
{
	char lbuf[BUFLEN];
	int len, llen;
	const char *msghdr = NULL;
	const char *lptr = NULL;
	const char *nlptr = NULL;
	const char *libname = uk_libname(libid);
	int lvl = flags & UK_PRINT_KLVL_MASK;
	int raw = flags & UK_PRINT_RAW_MASK;

	/*
	 * Note: We reset the console colors earlier in order to exclude
	 *       background colors for trailing white spaces.
	 */
	switch (lvl) {
	case UK_PRINT_KLVL_DEBUG:
		msghdr = LVLC_RESET LVLC_DEBUG "dbg:" LVLC_RESET "  ";
		break;
	case UK_PRINT_KLVL_CRIT:
		msghdr = LVLC_RESET LVLC_CRIT  "CRIT:" LVLC_RESET " ";
		break;
	case UK_PRINT_KLVL_ERR:
		msghdr = LVLC_RESET LVLC_ERROR "ERR:" LVLC_RESET "  ";
		break;
	case UK_PRINT_KLVL_WARN:
		msghdr = LVLC_RESET LVLC_WARN  "Warn:" LVLC_RESET " ";
		break;
	case UK_PRINT_KLVL_INFO:
		msghdr = LVLC_RESET LVLC_INFO  "Info:" LVLC_RESET " ";
		break;
	default: /* KLVL_NONE: no header */
		msghdr = "";
		break;
	}

	if (lvl != cons->prevlvl) {
		/* level changed from previous call */
		if (cons->prevlvl != INT_MIN && !cons->newline) {
			/* level changed without closing with '\n',
			 * enforce printing '\n', before the new message header
			 */
			vprint_cout(cons, "\n", 1);
		}
		cons->prevlvl = lvl;
		cons->newline = 1; /* enforce printing the message header */
	}

	len = uk_vsnprintf(lbuf, BUFLEN, fmt, ap);
	lptr = lbuf;
	while (len > 0) {
		if (cons->newline && !raw) {
#if CONFIG_LIBUKPRINT_PRINT_TIME
			print_timestamp(cons);
#endif
			vprint_cout(cons, DECONST(char *, msghdr),
				    strlen(msghdr));
#if CONFIG_LIBUKPRINT_PRINT_THREAD
			print_thread(cons);
#endif
#if CONFIG_LIBUKPRINT_PRINT_CALLER
			print_caller(cons, retaddr, frameaddr);
#endif
			if (libname) {
				vprint_cout(cons, LVLC_RESET LVLC_LIBNAME "[",
					    strlen(LVLC_RESET LVLC_LIBNAME) + 1);
				vprint_cout(cons, DECONST(char *, libname),
					    strlen(libname));
				vprint_cout(cons, "] ", 2);
			}
#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
			if (srcname) {
				char lnobuf[6];

				vprint_cout(cons, LVLC_RESET LVLC_SRCNAME "<",
					    strlen(LVLC_RESET LVLC_SRCNAME) + 1);
				vprint_cout(cons, DECONST(char *, srcname),
					    strlen(srcname));
				vprint_cout(cons, " @ ", 3);
				vprint_cout(cons, lnobuf,
					    uk_snprintf(lnobuf, sizeof(lnobuf),
							"%4u", srcline));
				vprint_cout(cons, "> ", 2);
			}
#endif
			cons->newline = 0;
		}

		nlptr = memchr(lptr, '\n', len);
		if (nlptr) {
			llen = (int)((uintptr_t)nlptr - (uintptr_t)lptr) + 1;
			cons->newline = 1;
		} else {
			llen = len;
		}

		/* Message body */
		switch (lvl) {
		case UK_PRINT_KLVL_CRIT:
			vprint_cout(cons, LVLC_RESET LVLC_CRIT_MSG,
				    strlen(LVLC_RESET LVLC_CRIT_MSG));
			break;
		case UK_PRINT_KLVL_ERR:
			vprint_cout(cons, LVLC_RESET LVLC_ERROR_MSG,
				    strlen(LVLC_RESET LVLC_ERROR_MSG));
			break;
		default:
			vprint_cout(cons, LVLC_RESET, strlen(LVLC_RESET));
		}
		vprint_cout(cons, (char *)lptr, llen);
		vprint_cout(cons, LVLC_RESET, strlen(LVLC_RESET));

		len -= llen;
		lptr = nlptr + 1;
	}
}

#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
#define _VPRINT_ARGS_SRCNAME(srcname, srcline)	\
	(srcname), (srcline),
#else
#define _VPRINT_ARGS_SRCNAME(srcname, srcline)
#endif /* CONFIG_LIBUKPRINT_PRINT_SRCNAME */

#if CONFIG_LIBUKPRINT_PRINT_CALLER
#define _VPRINT_ARGS_CALLER()			\
	__return_addr(0),			\
	__frame_addr(0),
#else
#define _VPRINT_ARGS_CALLER()
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */

#if CONFIG_LIBUKPRINT_PRINTK
void _uk_vprintk(int lvl, __u16 libid,
		 const char *srcname __maybe_unused,
		 unsigned int srcline __maybe_unused,
		 const char *fmt, va_list ap)
{
	vprint(&kern,  lvl, libid,
	       _VPRINT_ARGS_SRCNAME(srcname, srcline)
	       _VPRINT_ARGS_CALLER()
	       fmt, ap);
}

void _uk_printk(int lvl, __u16 libid,
		const char *srcname __maybe_unused,
		unsigned int srcline __maybe_unused,
		const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	vprint(&kern,  lvl, libid,
	       _VPRINT_ARGS_SRCNAME(srcname, srcline)
	       _VPRINT_ARGS_CALLER()
	       fmt, ap);

	va_end(ap);
}
#endif /* CONFIG_LIBUKPRINT_PRINTK */
