/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "snprintf.h"
#include <limits.h>
#include <string.h>

#include <uk/essentials.h>
#include <uk/plat/console.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/lcpu.h>
#if CONFIG_LIBUKPRINT_PRINT_THREAD
#include <uk/thread.h>
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */

#if CONFIG_LIBUKCONSOLE
#include <uk/console.h>
#include <uk/console/driver.h>
#endif /* CONFIG_LIBUKCONSOLE */

#include "print.h"

unsigned int uk_print_console_lvl = UK_PRINT_KLVL_MAX;

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
#else /* !CONFIG_LIBUKPRINT_ANSI_COLOR */
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

/* Console state for kernel output */
static struct vprint_console kconsole  = {
#if CONFIG_LIBUKCONSOLE
	/* Kernel messages are diagnostics: they go to the consoles' error
	 * stream, kept apart from a program's standard output where a
	 * console can, and to their logs (see kconsole_cout()).
	 */
	.cout = uk_console_err_onlcr,
#else /* !CONFIG_LIBUKCONSOLE */
	.cout = __NULL,
#endif /* !CONFIG_LIBUKCONSOLE */
	.newline = 1,
	.prevlvl = INT_MIN
};

/* How much of @buf[0..@len) to send so its end does not cut a UTF-8
 * sequence: the host takes the text as a string, and a broken one aborts
 * the guest call.
 */
static __sz utf8_whole(const char *buf, __sz len)
{
	__sz i = len;
	unsigned char lead;
	__sz n;

	/* A sequence is at most 4 bytes: its lead is at most 3 back */
	while (i > 0 && len - i < 3 &&
	       ((unsigned char)buf[i - 1] & 0xC0) == 0x80)
		i--;
	if (i == 0)
		return len;
	lead = (unsigned char)buf[i - 1];
	if (lead < 0xC0)
		return len;
	n = (lead >= 0xF0) ? 4 : (lead >= 0xE0) ? 3 : 2;
	return (len - (i - 1) < n) ? i - 1 : len;
}

#if CONFIG_LIBUKCONSOLE
/* Where the text of a message of level @lvl goes.  A crash is for
 * whoever reads the program's standard error, on every console.  The
 * rest, which a program's own failure can bring on (a missing file it
 * tries to run, say), and what is printed without a level (a syscall
 * trace, test results), is the kernel's log: a console that keeps one
 * took the message whole (kconsole_log()), and only the others print it.
 */
static cout_func kconsole_cout(int lvl)
{
	if (lvl == UK_PRINT_KLVL_CRIT)
		return uk_console_err_onlcr;
	return uk_console_err_unlogged_onlcr;
}

/* Hand the message in @text[0..@len) to the consoles that keep a log */
static void kconsole_log(struct uk_print_msg *msg, int lvl,
			 const char *libname, const char *text, int len)
{
	struct uk_console_log_msg lmsg = {
		.level = lvl,
		.libname = libname,
		.text = text,
	};

	if (len > 0 && text[len - 1] == '\n')
		len--;
	if (len <= 0)
		return;
	lmsg.len = len;
#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
	lmsg.srcname = msg->srcname;
	lmsg.srcline = msg->srcline;
#else /* !CONFIG_LIBUKPRINT_PRINT_SRCNAME */
	(void)msg;
#endif /* !CONFIG_LIBUKPRINT_PRINT_SRCNAME */
	uk_console_log(&lmsg);
}
#endif /* CONFIG_LIBUKCONSOLE */

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
#endif /* CONFIG_LIBUKPRINT_PRINT_TIME */

#if CONFIG_LIBUKPRINT_PRINT_THREAD
static void print_thread(struct vprint_console *cons)
{
	struct uk_thread *t = uk_thread_current();
	char buf[BUFLEN];
	int len;

	if (t) {
		if (t->name) {
			/* A long name is cut, at a whole character, so the
			 * tag still closes before the message
			 */
			len = strnlen(t->name, BUFLEN / 2);
			len = utf8_whole(t->name, len);
			len = uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_THREAD
					  "<%.*s> ", len, t->name);
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

void uk_print_console_write(struct uk_print_msg *msg)
{
	char lbuf[BUFLEN];
	int len, llen;
	const char *msghdr = __NULL;
	const char *lptr = __NULL;
	const char *nlptr = __NULL;
	const char *libname = uk_libname(msg->libid);
	int lvl = msg->flags & UK_PRINT_KLVL_MASK;
	int raw = msg->flags & UK_PRINT_RAW_MASK;
	struct vprint_console *cons = &kconsole;

#if CONFIG_LIBUKPRINT_PRINTK_UKSTORE
	if (lvl > (int)uk_print_console_lvl)
		return;
#endif /* CONFIG_LIBUKPRINT_PRINTK_UKSTORE */

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
#if CONFIG_LIBUKCONSOLE
		/* After the line it closes, on the stream that line is on */
		cons->cout = kconsole_cout(lvl);
#endif /* CONFIG_LIBUKCONSOLE */
		cons->prevlvl = lvl;
		cons->newline = 1; /* enforce printing the message header */
	}

	len = uk_vsnprintf(lbuf, BUFLEN, msg->fmt, msg->ap);
	/* uk_vsnprintf returns the length it wanted, not what fits in lbuf.
	 * A cut message ends at a whole character, and its line ends, or
	 * the next message joins it; a raw piece's caller ends its own.
	 */
	if (len >= BUFLEN) {
		len = utf8_whole(lbuf, raw ? BUFLEN - 1 : BUFLEN - 2);
		if (!raw && (!len || lbuf[len - 1] != '\n'))
			lbuf[len++] = '\n';
	}
#if CONFIG_LIBUKCONSOLE
	if (lvl != UK_PRINT_KLVL_CRIT)
		kconsole_log(msg, lvl, libname, lbuf, len);
#endif /* CONFIG_LIBUKCONSOLE */
	lptr = lbuf;
	while (len > 0) {
		if (cons->newline && !raw) {
#if CONFIG_LIBUKPRINT_PRINT_TIME
			print_timestamp(cons);
#endif /* CONFIG_LIBUKPRINT_PRINT_TIME */
			vprint_cout(cons, DECONST(char *, msghdr),
				    strlen(msghdr));
#if CONFIG_LIBUKPRINT_PRINT_THREAD
			print_thread(cons);
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */
#if CONFIG_LIBUKPRINT_PRINT_CALLER
			print_caller(cons, msg->retaddr, msg->frameaddr);
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */
			if (libname) {
				vprint_cout(cons, LVLC_RESET LVLC_LIBNAME "[",
					    sizeof(LVLC_RESET LVLC_LIBNAME));
				vprint_cout(cons, DECONST(char *, libname),
					    strlen(libname));
				vprint_cout(cons, "] ", 2);
			}
#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
			if (msg->srcname) {
				char lnobuf[6];

				vprint_cout(cons, LVLC_RESET LVLC_SRCNAME "<",
					    sizeof(LVLC_RESET LVLC_SRCNAME));
				vprint_cout(cons, DECONST(char *, msg->srcname),
					    strlen(msg->srcname));
				vprint_cout(cons, " @ ", 3);
				vprint_cout(cons, lnobuf,
					    uk_snprintf(lnobuf, sizeof(lnobuf),
							"%4u", msg->srcline));
				vprint_cout(cons, "> ", 2);
			}
#endif /* CONFIG_LIBUKPRINT_PRINT_SRCNAME */
			cons->newline = 0;
		}

		nlptr = memchr(lptr, '\n', len);
		if (nlptr) {
			llen = (int)((__uptr)nlptr - (__uptr)lptr) + 1;
			cons->newline = 1;
		} else {
			llen = len;
		}

		/* Message body */
		switch (lvl) {
		case UK_PRINT_KLVL_CRIT:
			vprint_cout(cons, LVLC_RESET LVLC_CRIT_MSG,
				    sizeof(LVLC_RESET LVLC_CRIT_MSG) - 1);
			break;
		case UK_PRINT_KLVL_ERR:
			vprint_cout(cons, LVLC_RESET LVLC_ERROR_MSG,
				    sizeof(LVLC_RESET LVLC_ERROR_MSG) - 1);
			break;
		default:
			vprint_cout(cons, LVLC_RESET, sizeof(LVLC_RESET) - 1);
		}
		vprint_cout(cons, (char *)lptr, llen);
		vprint_cout(cons, LVLC_RESET, sizeof(LVLC_RESET) - 1);

		len -= llen;
		lptr = nlptr + 1;
	}
}
