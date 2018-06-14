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
#define DLVL_CONS (-1)

#if !CONFIG_LIBUKDEBUG_REDIR_PRINTK
static inline void _vprintk(const char *fmt, va_list ap)
{
	char lbuf[BUFLEN];
	int len;

	len = vsnprintf(lbuf, BUFLEN, fmt, ap);
	if (likely(len > 0))
		ukplat_coutk(lbuf, len);
}
#endif

#if CONFIG_LIBUKDEBUG_REDIR_PRINTD
#define _ukplat_coutd(lbuf, len) ukplat_coutk((lbuf), (len))
#else
#define _ukplat_coutd(lbuf, len) ukplat_coutd((lbuf), (len))
#endif

#if CONFIG_LIBUKDEBUG_PRINTD_TIME
static void _printd_timestamp(void)
{
	char buf[BUFLEN];
	int len;
	__nsec nansec =  ukplat_monotonic_clock();
	__nsec sec = ukarch_time_nsec_to_sec(nansec);
	__nsec rem_usec = ukarch_time_subsec(nansec);

	rem_usec = ukarch_time_nsec_to_usec(rem_usec);
	len = snprintf(buf, BUFLEN, "[%5" __PRInsec ".%06" __PRInsec "] ",
			sec, rem_usec);
	_ukplat_coutd((char *)buf, len);
}
#endif

#if CONFIG_LIBUKDEBUG_PRINTD_STACK
static void _printd_stack(void)
{
	unsigned long stackb;
	char buf[BUFLEN];
	int len;

	stackb = (ukarch_read_sp() & ~(__STACK_SIZE - 1)) + __STACK_SIZE;

	len = snprintf(buf, BUFLEN, "<%p> ", (void *) stackb);
	_ukplat_coutd((char *)buf, len);
}
#endif

static inline void _vprintd(int lvl, const char *libname, const char *srcname,
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
#if CONFIG_LIBUKDEBUG_REDIR_PRINTK
	case DLVL_CONS:
		msghdr = "Kern: ";
		break;
#endif
	case DLVL_CRIT:
		msghdr = "CRIT: ";
		break;
	case DLVL_ERR:
		msghdr = "ERR:  ";
		break;
	case DLVL_WARN:
		msghdr = "Warn: ";
		break;
	case DLVL_INFO:
		msghdr = "Info: ";
		break;
	case DLVL_EXTRA:
		msghdr = "EInf: ";
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
			_ukplat_coutd("\n", 1);
		}
		prevlvl = lvl;
		newline = 1; /* enforce printing the message header */
	}

	len = vsnprintf(lbuf, BUFLEN, fmt, ap);
	lptr = lbuf;
	while (len > 0) {
		if (newline) {
#if CONFIG_LIBUKDEBUG_PRINTD_TIME
			_printd_timestamp();
#endif
			_ukplat_coutd(DECONST(char *, msghdr), 6);
#if CONFIG_LIBUKDEBUG_PRINTD_STACK
			_printd_stack();
#endif
			if (libname) {
				_ukplat_coutd("[", 1);
				_ukplat_coutd(DECONST(char *, libname),
					      strlen(libname));
				_ukplat_coutd("] ", 2);
			}
			if (srcname) {
				char lnobuf[6];

				_ukplat_coutd(DECONST(char *, srcname),
					      strlen(srcname));
				_ukplat_coutd(" @ ", 3);
				_ukplat_coutd(lnobuf,
					      snprintf(lnobuf, sizeof(lnobuf),
						       "%-5u", srcline));
				_ukplat_coutd(": ", 2);
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
		_ukplat_coutd((char *)lptr, llen);
		len -= llen;
		lptr = nlptr + 1;
	}
}

/* ensures that function is always compiled */
void uk_vprintk(const char *fmt, va_list ap)
{
#if CONFIG_LIBUKDEBUG_PRINTK
#if CONFIG_LIBUKDEBUG_REDIR_PRINTK
	_vprintd(DLVL_CONS, NULL, NULL, 0, fmt, ap);
#else
	_vprintk(fmt, ap);
#endif
#endif /* CONFIG_LIBUKDEBUG_PRINTK */
}

/* ensures that function is always compiled */
void uk_printk(const char *fmt, ...)
{
#if CONFIG_LIBUKDEBUG_PRINTK
	va_list ap;

	va_start(ap, fmt);
	uk_vprintk(fmt, ap);
	va_end(ap);
#endif /* CONFIG_LIBUKDEBUG_PRINTK */
}

/* ensures that function is always compiled */
void _uk_vprintd(int lvl, const char *libname, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap)
{
#if CONFIG_LIBUKDEBUG_PRINTD
	if (likely(lvl > DLVL_MAX))
		return;
	_vprintd(lvl, libname, srcname, srcline, fmt, ap);
#endif /* CONFIG_LIBUKDEBUG_PRINTD */
}

/* ensures that function is always compiled */
void _uk_printd(int lvl, const char *libname, const char *srcname,
		unsigned int srcline, const char *fmt, ...)
{
#if CONFIG_LIBUKDEBUG_PRINTD
	va_list ap;

	if (likely(lvl > DLVL_MAX))
		return;

	va_start(ap, fmt);
	_uk_vprintd(lvl, libname, srcname, srcline, fmt, ap);
	va_end(ap);
#endif /* CONFIG_LIBUKDEBUG_PRINTD */
}
