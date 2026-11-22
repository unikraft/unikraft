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
#include <uk/isr/string.h>
#include <uk/plat/console.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/errptr.h>
#if CONFIG_LIBUKPRINT_PRINT_THREAD
#include <uk/thread.h>
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */

#if CONFIG_LIBUKCONSOLE
#include <uk/console.h>
#include <uk/spinlock.h>
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

#define LINEBUF_LEN (3 * BUFLEN)

struct vprint_console {
	struct uk_spinlock lock;
	int newline;
	int prevlvl;
};

struct vprint_line {
	char buf[LINEBUF_LEN];
	__sz len;
};

static struct vprint_console kconsole = {
	.lock = UK_SPINLOCK_INITIALIZER(),
	.newline = 1,
	.prevlvl = INT_MIN,
};

static struct vprint_console kconsole_isr = {
	.lock = UK_SPINLOCK_INITIALIZER(),
	.newline = 1,
	.prevlvl = INT_MIN,
};

static __isr void vprint_append(struct vprint_line *line, const char *buf, __sz len)
{
	/* Always leave room for a complete line ending. */
	len = MIN(len, sizeof(line->buf) - line->len - 2);
	memcpy_isr(line->buf + line->len, buf, len);
	line->len += len;
}

static __isr void vprint_end_line(struct vprint_line *line)
{
#if !CONFIG_LIBUKPRINT_NOCARRIAGERETURN
	line->buf[line->len++] = '\r';
#endif
	line->buf[line->len++] = '\n';
}

static void vprint_flush(struct vprint_line *line, __bool newline)
{
	if (newline)
		vprint_end_line(line);
	if (line->len)
		uk_console_out(line->buf, line->len);
	line->len = 0;
}

static __isr void vprint_flush_isr(struct vprint_line *line, __bool newline)
{
	if (newline)
		vprint_end_line(line);
	if (line->len)
		uk_console_emerg_out(line->buf, line->len);
	line->len = 0;
}

#if CONFIG_LIBUKPRINT_PRINT_TIME
static void print_timestamp(struct vprint_line *line)
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
	vprint_append(line, buf, len > 0 ? (__sz)MIN(len, BUFLEN - 1) : 0);
}

__isr static void print_timestamp_isr(struct vprint_line *line)
{
	char buf[BUFLEN];
	int len;
	/* TODO: This is not __isr but in practice it seems to be fine. Add
	 * a proper __isr variant when moving timer/wallclock related
	 * functionalities to their own library.
	 */
	__nsec nansec =  ukplat_monotonic_clock();
	__nsec sec = ukarch_time_nsec_to_sec(nansec);
	__nsec rem_usec = ukarch_time_subsec(nansec);

	rem_usec = ukarch_time_nsec_to_usec(rem_usec);
	len = uk_snprintf_isr(buf, BUFLEN, LVLC_RESET LVLC_TS
			      "[%5" __PRInsec ".%06" __PRInsec "] ",
			      sec, rem_usec);
	vprint_append(line, buf, len > 0 ? (__sz)MIN(len, BUFLEN - 1) : 0);
}
#endif /* CONFIG_LIBUKPRINT_PRINT_TIME */

#if CONFIG_LIBUKPRINT_PRINT_THREAD
static void print_thread(struct vprint_line *line)
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
	vprint_append(line, buf, len > 0 ? (__sz)MIN(len, BUFLEN - 1) : 0);
}

__isr static void print_thread_isr(struct vprint_line *line)
{
	struct uk_thread *t;
	char buf[BUFLEN];
	int len;

	t = uk_pcpuvar_lval(uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx),
			    __uk_sched_thread_current);

	if (t) {
		if (t->name) {
			len = uk_snprintf_isr(buf,
					      BUFLEN, LVLC_RESET LVLC_THREAD
					      "<%s> ", t->name);
		} else {
			len = uk_snprintf_isr(buf,
					      BUFLEN, LVLC_RESET LVLC_THREAD
					      "<%p> ", (void *)t);
		}
	} else {
		len = uk_snprintf_isr(buf, BUFLEN, LVLC_RESET LVLC_THREAD
				      "<<n/a>> ");
	}
	vprint_append(line, buf, len > 0 ? (__sz)MIN(len, BUFLEN - 1) : 0);
}
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */

#if CONFIG_LIBUKPRINT_PRINT_CALLER
static void print_caller(struct vprint_line *line, __uptr ra, __uptr fa)
{
	char buf[BUFLEN];
	int len;

	len = uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_CALLER
			  "{r:%p,f:%p} ", (void *)ra, (void *)fa);
	vprint_append(line, buf, len > 0 ? (__sz)MIN(len, BUFLEN - 1) : 0);
}

__isr static void print_caller_isr(struct vprint_line *line, __uptr ra,
				   __uptr fa)
{
	char buf[BUFLEN];
	int len;

	len = uk_snprintf_isr(buf, BUFLEN, LVLC_RESET LVLC_CALLER
			      "{r:%p,f:%p} ", (void *)ra, (void *)fa);
	vprint_append(line, buf, len > 0 ? (__sz)MIN(len, BUFLEN - 1) : 0);
}
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */

void uk_print_console_write(struct uk_print_msg *msg)
{
	char lbuf[BUFLEN];
	struct vprint_line line;
	unsigned long flags;
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

	len = uk_vsnprintf(lbuf, BUFLEN, msg->fmt, msg->ap);
	len = MIN(len, BUFLEN - 1);
	line.len = 0;
	uk_spin_lock_irqsave(&cons->lock, flags);

	if (lvl != cons->prevlvl) {
		/* level changed from previous call */
		if (cons->prevlvl != INT_MIN && !cons->newline) {
			/* level changed without closing with '\n',
			 * enforce printing '\n', before the new message header
			 */
			vprint_flush(&line, __true);
		}
		cons->prevlvl = lvl;
		cons->newline = 1; /* enforce printing the message header */
	}

	lptr = lbuf;
	while (len > 0) {
		if (cons->newline && !raw) {
#if CONFIG_LIBUKPRINT_PRINT_TIME
			print_timestamp(&line);
#endif /* CONFIG_LIBUKPRINT_PRINT_TIME */
			vprint_append(&line, msghdr, strlen(msghdr));
#if CONFIG_LIBUKPRINT_PRINT_THREAD
			print_thread(&line);
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */
#if CONFIG_LIBUKPRINT_PRINT_CALLER
			print_caller(&line, msg->retaddr, msg->frameaddr);
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */
			if (libname) {
				vprint_append(&line, LVLC_RESET LVLC_LIBNAME "[",
					sizeof(LVLC_RESET LVLC_LIBNAME "[")
						- 1);
				vprint_append(&line, libname, strlen(libname));
				vprint_append(&line, "] ", 2);
			}
#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
			if (msg->srcname) {
				char lnobuf[11];

				vprint_append(&line, LVLC_RESET LVLC_SRCNAME "<",
					sizeof(LVLC_RESET LVLC_SRCNAME "<")
						- 1);
				vprint_append(&line, msg->srcname, strlen(msg->srcname));
				vprint_append(&line, " @ ", 3);
				vprint_append(&line, lnobuf,
					    uk_snprintf(lnobuf, sizeof(lnobuf),
							"%4u", msg->srcline));
				vprint_append(&line, "> ", 2);
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
			vprint_append(&line, LVLC_RESET LVLC_CRIT_MSG,
				    sizeof(LVLC_RESET LVLC_CRIT_MSG) - 1);
			break;
		case UK_PRINT_KLVL_ERR:
			vprint_append(&line, LVLC_RESET LVLC_ERROR_MSG,
				    sizeof(LVLC_RESET LVLC_ERROR_MSG) - 1);
			break;
		default:
			vprint_append(&line, LVLC_RESET, sizeof(LVLC_RESET) - 1);
		}
		/* Write the reset before the trailing '\n' so consoles that
		 * flush on newline (e.g. Xen vpl011) keep the reset on the
		 * same line and colors do not bleed to the next line.
		 * Order: body -> reset -> '\n'.
		 */
		vprint_append(&line, lptr, nlptr ? llen - 1 : llen);
		vprint_append(&line, LVLC_RESET, sizeof(LVLC_RESET) - 1);
		if (nlptr)
			vprint_flush(&line, __true);

		len -= llen;
		lptr = nlptr ? nlptr + 1 : lptr + llen;
	}
	vprint_flush(&line, __false);
	uk_spin_unlock_irqrestore(&cons->lock, flags);
}

__isr void uk_print_console_write_isr(struct uk_print_msg *msg)
{
	char lbuf[BUFLEN];
	struct vprint_line line;
	unsigned long flags;
	int len, llen;
	const char *msghdr = __NULL;
	const char *lptr = __NULL;
	const char *nlptr = __NULL;
	const char *libname = uk_libname(msg->libid);
	int lvl = msg->flags & UK_PRINT_KLVL_MASK;
	int raw = msg->flags & UK_PRINT_RAW_MASK;
	struct vprint_console *cons = &kconsole_isr;

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

	len = uk_vsnprintf_isr(lbuf, BUFLEN, msg->fmt,
			       msg->ap);
	len = MIN(len, BUFLEN - 1);
	line.len = 0;
	uk_spin_lock_irqsave(&cons->lock, flags);

	if (lvl != cons->prevlvl) {
		/* level changed from previous call */
		if (cons->prevlvl != INT_MIN && !cons->newline) {
			/* level changed without closing with '\n',
			 * enforce printing '\n', before the new message header
			 */
			vprint_flush_isr(&line, __true);
		}
		cons->prevlvl = lvl;
		cons->newline = 1; /* enforce printing the message header */
	}

	lptr = lbuf;
	while (len > 0) {
		if (cons->newline && !raw) {
#if CONFIG_LIBUKPRINT_PRINT_TIME
			print_timestamp_isr(&line);
#endif /* CONFIG_LIBUKPRINT_PRINT_TIME */
			vprint_append(&line, msghdr, strlen_isr(msghdr));
#if CONFIG_LIBUKPRINT_PRINT_THREAD
			print_thread_isr(&line);
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */
#if CONFIG_LIBUKPRINT_PRINT_CALLER
			print_caller_isr(&line, msg->retaddr, msg->frameaddr);
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */
			if (libname) {
				vprint_append(&line,
						LVLC_RESET LVLC_LIBNAME "[",
						sizeof(LVLC_RESET
						       LVLC_LIBNAME "[") - 1);
				vprint_append(&line, libname, strlen_isr(libname));
				vprint_append(&line, "] ", 2);
			}
#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
			if (msg->srcname) {
				char lnobuf[11];

				vprint_append(&line,
						LVLC_RESET LVLC_SRCNAME "<",
						sizeof(LVLC_RESET
						       LVLC_SRCNAME "<") - 1);
				vprint_append(&line, msg->srcname, strlen_isr(msg->srcname));
				vprint_append(&line, " @ ", 3);
				vprint_append(&line, lnobuf,
						uk_snprintf_isr(lnobuf,
								sizeof(lnobuf),
								"%4u",
								msg->srcline));
				vprint_append(&line, "> ", 2);
			}
#endif /* CONFIG_LIBUKPRINT_PRINT_SRCNAME */
			cons->newline = 0;
		}

		nlptr = memchr_isr(lptr, '\n', len);
		if (nlptr) {
			llen = (int)((__uptr)nlptr - (__uptr)lptr) + 1;
			cons->newline = 1;
		} else {
			llen = len;
		}

		/* Message body */
		switch (lvl) {
		case UK_PRINT_KLVL_CRIT:
			vprint_append(&line, LVLC_RESET LVLC_CRIT_MSG,
					sizeof(LVLC_RESET LVLC_CRIT_MSG) - 1);
			break;
		case UK_PRINT_KLVL_ERR:
			vprint_append(&line, LVLC_RESET LVLC_ERROR_MSG,
					sizeof(LVLC_RESET LVLC_ERROR_MSG) - 1);
			break;
		default:
			vprint_append(&line, LVLC_RESET,
					sizeof(LVLC_RESET) - 1);
		}
		/* Write the reset before the trailing '\n' (see non-ISR). */
		vprint_append(&line, lptr, nlptr ? llen - 1 : llen);
		vprint_append(&line, LVLC_RESET, sizeof(LVLC_RESET) - 1);
		if (nlptr)
			vprint_flush_isr(&line, __true);

		len -= llen;
		lptr = nlptr ? nlptr + 1 : lptr + llen;
	}
	vprint_flush_isr(&line, __false);
	uk_spin_unlock_irqrestore(&cons->lock, flags);
}
