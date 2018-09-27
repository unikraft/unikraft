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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>

#include <uk/plat/console.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/arch/lcpu.h>

/*
 * Note: Console redirection is implemented in this file. All pre-compiled code
 *       (even with a different configuration) will end up calling uk_printk()
 *       or _uk_printd() depending on the message type. The behavior of the
 *       final image adopts automatically to the current configuration of this
 *       library.
 */

#define BUFLEN 192
/* special level for printk redirection, used internally only */
#define KLVL_DEBUG (-1)

#if !CONFIG_LIBUKDEBUG_REDIR_PRINTD
static inline void _vprintd(const char *fmt, va_list ap)
{
	char lbuf[BUFLEN];
	int len;

	len = vsnprintf(lbuf, BUFLEN, fmt, ap);
	if (likely(len > 0))
		ukplat_coutk(lbuf, len);
}
#endif

#if CONFIG_LIBUKDEBUG_REDIR_PRINTK
#define _ukplat_coutk(lbuf, len) ukplat_coutd((lbuf), (len))
#else
#define _ukplat_coutk(lbuf, len) ukplat_coutk((lbuf), (len))
#endif

#if CONFIG_LIBUKDEBUG_PRINTK_TIME
static void _printk_timestamp(void)
{
	char buf[BUFLEN];
	int len;
	__nsec nansec =  ukplat_monotonic_clock();
	__nsec sec = ukarch_time_nsec_to_sec(nansec);
	__nsec rem_usec = ukarch_time_subsec(nansec);

	rem_usec = ukarch_time_nsec_to_usec(rem_usec);
	len = snprintf(buf, BUFLEN, "[%5" __PRInsec ".%06" __PRInsec "] ",
			sec, rem_usec);
	_ukplat_coutk((char *)buf, len);
}
#endif

#if CONFIG_LIBUKDEBUG_PRINTK_STACK
static void _printk_stack(void)
{
	unsigned long stackb;
	char buf[BUFLEN];
	int len;

	stackb = (ukarch_read_sp() & ~(__STACK_SIZE - 1)) + __STACK_SIZE;

	len = snprintf(buf, BUFLEN, "<%p> ", (void *) stackb);
	_ukplat_coutk((char *)buf, len);
}
#endif

#if CONFIG_LIBUKDEBUG_REDIR_PRINTD || CONFIG_LIBUKDEBUG_PRINTK
static void _vprintk(int lvl, const char *libname, const char *srcname,
		     unsigned int srcline, const char *fmt, va_list ap)
{
	static int newline = 1;
	static int prevlvl = INT_MIN;

	char lbuf[BUFLEN];
	int len, llen;
	const char *msghdr = NULL;
	const char *lptr = NULL;
	const char *nlptr = NULL;

	switch (lvl) {
#if CONFIG_LIBUKDEBUG_REDIR_PRINTD
	case KLVL_DEBUG:
		msghdr = "dbg:  ";
		break;
#endif
	case KLVL_CRIT:
		msghdr = "CRIT: ";
		break;
	case KLVL_ERR:
		msghdr = "ERR:  ";
		break;
	case KLVL_WARN:
		msghdr = "Warn: ";
		break;
	case KLVL_INFO:
		msghdr = "Info: ";
		break;
	default:
		/* unknown type: ignore */
		return;
	}

	if (lvl != prevlvl) {
		/* level changed from previous call */
		if (prevlvl != INT_MIN && !newline) {
			/* level changed without closing with '\n',
			 * enforce printing '\n', before the new message header
			 */
			_ukplat_coutk("\n", 1);
		}
		prevlvl = lvl;
		newline = 1; /* enforce printing the message header */
	}

	len = vsnprintf(lbuf, BUFLEN, fmt, ap);
	lptr = lbuf;
	while (len > 0) {
		if (newline) {
#if CONFIG_LIBUKDEBUG_PRINTK_TIME
			_printk_timestamp();
#endif
			_ukplat_coutk(DECONST(char *, msghdr), 6);
#if CONFIG_LIBUKDEBUG_PRINTK_STACK
			_printk_stack();
#endif
			if (libname) {
				_ukplat_coutk("[", 1);
				_ukplat_coutk(DECONST(char *, libname),
					      strlen(libname));
				_ukplat_coutk("] ", 2);
			}
			if (srcname) {
				char lnobuf[6];

				_ukplat_coutk(DECONST(char *, srcname),
					      strlen(srcname));
				_ukplat_coutk(" @ ", 3);
				_ukplat_coutk(lnobuf,
					      snprintf(lnobuf, sizeof(lnobuf),
						       "%-5u", srcline));
				_ukplat_coutk(": ", 2);
			}
			newline = 0;
		}

		nlptr = memchr(lptr, '\n', len);
		if (nlptr) {
			llen = (int)((uintptr_t)nlptr - (uintptr_t)lbuf) + 1;
			newline = 1;
		} else {
			llen = len;
		}
		_ukplat_coutk((char *)lptr, llen);
		len -= llen;
		lptr = nlptr + 1;
	}
}
#endif /* CONFIG_LIBUKDEBUG_REDIR_PRINTD || CONFIG_LIBUKDEBUG_PRINTK */

/*
 * DEBUG PRINTING ENTRY
 *  uk_printd() and uk_vprintd are always compiled in.
 *  We rely on OPTIMIZE_DEADELIM: These symbols are automatically
 *  removed from the final image when there was no usage.
 */
void uk_vprintd(const char *fmt __maybe_unused, va_list ap __maybe_unused)
{
#if CONFIG_LIBUKDEBUG_REDIR_PRINTD
	_vprintk(KLVL_DEBUG, NULL, NULL, 0, fmt, ap);
#else
	_vprintd(fmt, ap);
#endif
}

void uk_printd(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	uk_vprintd(fmt, ap);
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
	_vprintk(lvl, libname, srcname, srcline, fmt, ap);
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
