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

#include <uk/plat/console.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/arch/lcpu.h>

#if CONFIG_LIBUKDEBUG_ANSI_COLOR
#define LVLC_RESET	UK_ANSI_MOD_RESET
#define LVLC_TS		UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_GREEN)
#define LVLC_SP		UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLUE)
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
#define LVLC_SP		""
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

typedef int (*_ukplat_cout_t)(const char *, unsigned int);

struct _vprint_console {
	_ukplat_cout_t cout;
	int newline;
	int prevlvl;
};

/* Console state for kernel output */
#if CONFIG_LIBUKDEBUG_REDIR_PRINTD || CONFIG_LIBUKDEBUG_PRINTK
static struct _vprint_console kern  = { .cout = ukplat_coutk,
					.newline = 1,
					.prevlvl = INT_MIN };
#endif

/* Console state for debug output */
#if !CONFIG_LIBUKDEBUG_REDIR_PRINTD
static struct _vprint_console debug = { .cout = ukplat_coutd,
					.newline = 1,
					.prevlvl = INT_MIN };
#endif

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
	cons->cout((char *)buf, len);
}
#endif

#if CONFIG_LIBUKDEBUG_PRINT_STACK
static void _print_stack(struct _vprint_console *cons)
{
	unsigned long stackb;
	char buf[BUFLEN];
	int len;

	stackb = (ukarch_read_sp() & STACK_MASK_TOP) + __STACK_SIZE;

	len = __uk_snprintf(buf, BUFLEN, LVLC_RESET LVLC_SP
			    "<%p> ", (void *) stackb);
	cons->cout((char *)buf, len);
}
#endif

static void _vprint(struct _vprint_console *cons,
		    int lvl, const char *libname,
		    const char *srcname __maybe_unused,
		    unsigned int srcline __maybe_unused,
		    const char *fmt, va_list ap)
{
	char lbuf[BUFLEN];
	int len, llen;
	const char *msghdr = NULL;
	const char *lptr = NULL;
	const char *nlptr = NULL;

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
			cons->cout("\n", 1);
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
			cons->cout(DECONST(char *, msghdr), strlen(msghdr));
#if CONFIG_LIBUKDEBUG_PRINT_STACK
			_print_stack(cons);
#endif
			if (libname) {
				cons->cout(LVLC_RESET LVLC_LIBNAME "[",
					   strlen(LVLC_RESET LVLC_LIBNAME) + 1);
				cons->cout(DECONST(char *, libname),
					   strlen(libname));
				cons->cout("] ", 2);
			}
#if CONFIG_LIBUKDEBUG_PRINT_SRCNAME
			if (srcname) {
				char lnobuf[6];

				cons->cout(LVLC_RESET LVLC_SRCNAME "<",
					   strlen(LVLC_RESET LVLC_SRCNAME) + 1);
				cons->cout(DECONST(char *, srcname),
					   strlen(srcname));
				cons->cout(" @ ", 3);
				cons->cout(lnobuf,
					   __uk_snprintf(lnobuf, sizeof(lnobuf),
							 "%4u", srcline));
				cons->cout("> ", 2);
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
			cons->cout(LVLC_RESET LVLC_CRIT_MSG,
				   strlen(LVLC_RESET LVLC_CRIT_MSG));
			break;
		case KLVL_ERR:
			cons->cout(LVLC_RESET LVLC_ERROR_MSG,
				   strlen(LVLC_RESET LVLC_ERROR_MSG));
			break;
		default:
			cons->cout(LVLC_RESET, strlen(LVLC_RESET));
		}
		cons->cout((char *)lptr, llen);
		cons->cout(LVLC_RESET, strlen(LVLC_RESET));

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
void _uk_vprintd(const char *libname, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap)
{

#if CONFIG_LIBUKDEBUG_REDIR_PRINTD
	_vprint(&kern,  KLVL_DEBUG, libname, srcname, srcline, fmt, ap);
#else
	_vprint(&debug, KLVL_DEBUG, libname, srcname, srcline, fmt, ap);
#endif
}

void _uk_printd(const char *libname, const char *srcname,
		unsigned int srcline, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_uk_vprintd(libname, srcname, srcline, fmt, ap);
	va_end(ap);
}

/*
 * KERNEL PRINT ENTRY
 *  Different to uk_printd(), we have a global switch that disables kernel
 *  messages. We compile these entry points only in when the kernel console is
 *  enabled.
 */
#if CONFIG_LIBUKDEBUG_PRINTK
void _uk_vprintk(int lvl, const char *libname, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap)
{
#if CONFIG_LIBUKDEBUG_REDIR_PRINTK
	_vprint(&debug, lvl, libname, srcname, srcline, fmt, ap);
#else
	_vprint(&kern,  lvl, libname, srcname, srcline, fmt, ap);
#endif
}

void _uk_printk(int lvl, const char *libname, const char *srcname,
		unsigned int srcline, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_uk_vprintk(lvl, libname, srcname, srcline, fmt, ap);
	va_end(ap);
}
#endif /* CONFIG_LIBUKDEBUG_PRINTD */
