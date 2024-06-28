/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Debug printing routines
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
#if CONFIG_LIBUKDEBUG_PRINT_THREAD
#include <uk/thread.h>
#endif

#if CONFIG_LIBUKCONSOLE
#include <uk/console.h>
#include <errno.h>
#endif /* CONFIG_LIBUKCONSOLE */

#if CONFIG_LIBUKDEBUG_ANSI_COLOR
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
#endif /* !CONFIG_LIBUKDEBUG_ANSI_COLOR */

#define BUFLEN 192
/* special level for printk redirection, used internally only */
#define KLVL_DEBUG (-1)

typedef __ssz (*cout_func)(const char *buf, __sz len);

struct _vprint_console {
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
static inline __ssz _console_out(const char *buf, __sz len)
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
#if CONFIG_LIBUKDEBUG_REDIR_PRINTD || CONFIG_LIBUKDEBUG_PRINTK
static struct _vprint_console kern  = {
#if CONFIG_LIBUKCONSOLE
	.cout = _console_out,
#else
	.cout = NULL,
#endif /* CONFIG_LIBUKCONSOLE */
	.newline = 1,
	.prevlvl = INT_MIN
};
#endif

/* Console state for debug output */
#if !CONFIG_LIBUKDEBUG_REDIR_PRINTD
static struct _vprint_console debug = {
#if CONFIG_LIBUKCONSOLE
	.cout = _console_out,
#else
	.cout = NULL,
#endif /* CONFIG_LIBUKCONSOLE */
	.newline = 1,
	.prevlvl = INT_MIN
};
#endif

static inline void _vprint_cout(struct _vprint_console *cons,
				const char *buf, __sz len)
{
	if (cons->cout)
		cons->cout(buf, len);
}

#if CONFIG_LIBUKDEBUG_PRINT_TIME
static void _print_timestamp(struct _vprint_console *cons)
{
	char buf[BUFLEN];
	int len;
	__nsec nansec =  ukplat_monotonic_clock();
	__nsec sec = ukarch_time_nsec_to_sec(nansec);
	__nsec rem_usec = ukarch_time_subsec(nansec);

	rem_usec = ukarch_time_nsec_to_usec(rem_usec);
	len = __uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_TS
			    "[%5" __PRInsec ".%06" __PRInsec "] ",
			    sec, rem_usec);
	_vprint_cout(cons, (char *)buf, len);
}
#endif

#if CONFIG_LIBUKDEBUG_PRINT_THREAD
static void _print_thread(struct _vprint_console *cons)
{
	struct uk_thread *t = uk_thread_current();
	char buf[BUFLEN];
	int len;

	if (t) {
		if (t->name) {
			len = __uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_THREAD
					    "<%s> ", t->name);
		} else {
			len = __uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_THREAD
					    "<%p> ", (void *) t);
		}
	} else {
		len = __uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_THREAD
				    "<<n/a>> ");
	}
	_vprint_cout(cons, (char *)buf, len);
}
#endif /* CONFIG_LIBUKDEBUG_PRINT_THREAD */

#if CONFIG_LIBUKDEBUG_PRINT_CALLER
static void _print_caller(struct _vprint_console *cons, __uptr ra, __uptr fa)
{
	char buf[BUFLEN];
	int len;

	len = __uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_CALLER
			    "{r:%p,f:%p} ", (void *) ra, (void *) fa);
	_vprint_cout(cons, (char *)buf, len);
}
#endif /* CONFIG_LIBUKDEBUG_PRINT_CALLER */

static void _vprint(struct _vprint_console *cons,
		    int lvl, __u16 libid,
#if CONFIG_LIBUKDEBUG_PRINT_SRCNAME
		    const char *srcname,
		    unsigned int srcline,
#endif /* CONFIG_LIBUKDEBUG_PRINT_SRCNAME */
#if CONFIG_LIBUKDEBUG_PRINT_CALLER
		    __uptr retaddr,
		    __uptr frameaddr,
#endif /* CONFIG_LIBUKDEBUG_PRINT_CALLER */
		    const char *fmt, va_list ap)
{
	char lbuf[BUFLEN];
	int len, llen;
	const char *msghdr = NULL;
	const char *lptr = NULL;
	const char *nlptr = NULL;
	const char *libname = uk_libname(libid);

	/*
	 * Note: We reset the console colors earlier in order to exclude
	 *       background colors for trailing white spaces.
	 */
	switch (lvl) {
	case KLVL_DEBUG:
		msghdr = LVLC_RESET LVLC_DEBUG "dbg:" LVLC_RESET "  ";
		break;
	case KLVL_CRIT:
		msghdr = LVLC_RESET LVLC_CRIT  "CRIT:" LVLC_RESET " ";
		break;
	case KLVL_ERR:
		msghdr = LVLC_RESET LVLC_ERROR "ERR:" LVLC_RESET "  ";
		break;
	case KLVL_WARN:
		msghdr = LVLC_RESET LVLC_WARN  "Warn:" LVLC_RESET " ";
		break;
	case KLVL_INFO:
		msghdr = LVLC_RESET LVLC_INFO  "Info:" LVLC_RESET " ";
		break;
	default:
		/* unknown type: ignore */
		return;
	}

	if (lvl != cons->prevlvl) {
		/* level changed from previous call */
		if (cons->prevlvl != INT_MIN && !cons->newline) {
			/* level changed without closing with '\n',
			 * enforce printing '\n', before the new message header
			 */
			_vprint_cout(cons, "\n", 1);
		}
		cons->prevlvl = lvl;
		cons->newline = 1; /* enforce printing the message header */
	}

	len = __uk_vsnprintf(lbuf, BUFLEN, fmt, ap);
	lptr = lbuf;
	while (len > 0) {
		if (cons->newline) {
#if CONFIG_LIBUKDEBUG_PRINT_TIME
			_print_timestamp(cons);
#endif
			_vprint_cout(cons, DECONST(char *, msghdr),
				     strlen(msghdr));
#if CONFIG_LIBUKDEBUG_PRINT_THREAD
			_print_thread(cons);
#endif
#if CONFIG_LIBUKDEBUG_PRINT_CALLER
			_print_caller(cons, retaddr, frameaddr);
#endif
			if (libname) {
				_vprint_cout(cons, LVLC_RESET LVLC_LIBNAME "[",
					     strlen(LVLC_RESET LVLC_LIBNAME) + 1);
				_vprint_cout(cons, DECONST(char *, libname),
					     strlen(libname));
				_vprint_cout(cons, "] ", 2);
			}
#if CONFIG_LIBUKDEBUG_PRINT_SRCNAME
			if (srcname) {
				char lnobuf[6];

				_vprint_cout(cons, LVLC_RESET LVLC_SRCNAME "<",
					     strlen(LVLC_RESET LVLC_SRCNAME) + 1);
				_vprint_cout(cons, DECONST(char *, srcname),
					     strlen(srcname));
				_vprint_cout(cons, " @ ", 3);
				_vprint_cout(cons, lnobuf,
					     __uk_snprintf(lnobuf,
							   sizeof(lnobuf),
							   "%4u", srcline));
				_vprint_cout(cons, "> ", 2);
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
		case KLVL_CRIT:
			_vprint_cout(cons, LVLC_RESET LVLC_CRIT_MSG,
				     strlen(LVLC_RESET LVLC_CRIT_MSG));
			break;
		case KLVL_ERR:
			_vprint_cout(cons, LVLC_RESET LVLC_ERROR_MSG,
				     strlen(LVLC_RESET LVLC_ERROR_MSG));
			break;
		default:
			_vprint_cout(cons, LVLC_RESET, strlen(LVLC_RESET));
		}
		_vprint_cout(cons, (char *)lptr, llen);
		_vprint_cout(cons, LVLC_RESET, strlen(LVLC_RESET));

		len -= llen;
		lptr = nlptr + 1;
	}
}

/*
 * DEBUG PRINTING ENTRY
 *  uk_printd() and uk_vprintd are always compiled in.
 *  We rely on OPTIMIZE_DEADELIM: These symbols are automatically
 *  removed from the final image when there was no usage.
 */
#if CONFIG_LIBUKDEBUG_PRINT_SRCNAME
#define _VPRINT_ARGS_SRCNAME(srcname, srcline)	\
	(srcname), (srcline),
#else
#define _VPRINT_ARGS_SRCNAME(srcname, srcline)
#endif /* CONFIG_LIBUKDEBUG_PRINT_SRCNAME */

#if CONFIG_LIBUKDEBUG_PRINT_CALLER
#define _VPRINT_ARGS_CALLER()			\
	__return_addr(0),			\
	__frame_addr(0),
#else
#define _VPRINT_ARGS_CALLER()
#endif /* CONFIG_LIBUKDEBUG_PRINT_CALLER */

void _uk_vprintd(__u16 libid, const char *srcname __maybe_unused,
		 unsigned int srcline __maybe_unused, const char *fmt,
		 va_list ap)
{

#if CONFIG_LIBUKDEBUG_REDIR_PRINTD
	_vprint(&kern,  KLVL_DEBUG, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#else
	_vprint(&debug, KLVL_DEBUG, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#endif /* !CONFIG_LIBUKDEBUG_REDIR_PRINTD */
}

void _uk_printd(__u16 libid, const char *srcname __maybe_unused,
		unsigned int srcline __maybe_unused, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#if CONFIG_LIBUKDEBUG_REDIR_PRINTD
	_vprint(&kern,  KLVL_DEBUG, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#else
	_vprint(&debug, KLVL_DEBUG, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#endif /* !CONFIG_LIBUKDEBUG_REDIR_PRINTD */
	va_end(ap);
}

/*
 * KERNEL PRINT ENTRY
 *  Different to uk_printd(), we have a global switch that disables kernel
 *  messages. We compile these entry points only in when the kernel console is
 *  enabled.
 */
#if CONFIG_LIBUKDEBUG_PRINTK
void _uk_vprintk(int lvl, __u16 libid,
		 const char *srcname __maybe_unused,
		 unsigned int srcline __maybe_unused,
		 const char *fmt, va_list ap)
{
#if CONFIG_LIBUKDEBUG_REDIR_PRINTK
	_vprint(&debug, lvl, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#else
	_vprint(&kern,  lvl, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#endif /* !CONFIG_LIBUKDEBUG_REDIR_PRINTK */
}

void _uk_printk(int lvl, __u16 libid,
		const char *srcname __maybe_unused,
		unsigned int srcline __maybe_unused,
		const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#if CONFIG_LIBUKDEBUG_REDIR_PRINTK
	_vprint(&debug, lvl, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#else
	_vprint(&kern,  lvl, libid,
		_VPRINT_ARGS_SRCNAME(srcname, srcline)
		_VPRINT_ARGS_CALLER()
		fmt, ap);
#endif /* !CONFIG_LIBUKDEBUG_REDIR_PRINTK */
	va_end(ap);
}
#endif /* CONFIG_LIBUKDEBUG_PRINTD */
