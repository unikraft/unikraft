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

#ifndef __UKDEBUG_PRINT_H__
#define __UKDEBUG_PRINT_H__

#include <stdarg.h>
#include <uk/essentials.h>
#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __LIBNAME__
#define __STR_LIBNAME__ STRINGIFY(__LIBNAME__)
#else
#define __STR_LIBNAME__ (NULL)
#endif

#ifdef __BASENAME__
#define __STR_BASENAME__ STRINGIFY(__BASENAME__)
#else
#define __STR_BASENAME__ (NULL)
#endif

/*
 * DEBUG PRINTING
 */
#ifdef __IN_LIBUKDEBUG__
/*
 * This redefinition of CONFIG_LIBUKDEBUG_PRINTD is doing the trick to avoid
 * multiple declarations of uk_{v}printd() when we are compiling this library
 * and have the global debug switch CONFIG_LIBUKDEBUG_PRINTD not enabled.
 */
#if !defined CONFIG_LIBUKDEBUG_PRINTD || !CONFIG_LIBUKDEBUG_PRINTD
#undef CONFIG_LIBUKDEBUG_PRINTD
#define CONFIG_LIBUKDEBUG_PRINTD 1
#endif
#endif /* __IN_LIBUKDEBUG__ */

#if defined UK_DEBUG || CONFIG_LIBUKDEBUG_PRINTD
/* please use the uk_printd(), uk_vprintd() macros because
 * they compile in the function calls only if debugging
 * is enabled
 */
void _uk_vprintd(const char *libname, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap);
void _uk_printd(const char *libname, const char *srcname,
		unsigned int srcline, const char *fmt, ...) __printf(4, 5);

#define uk_vprintd(fmt, ap)						\
	do {								\
		_uk_vprintd(__STR_LIBNAME__, __STR_BASENAME__,		\
			    __LINE__, (fmt), ap);			\
	} while (0)

#define uk_printd(fmt, ...)						\
	do {								\
		_uk_printd(__STR_LIBNAME__, __STR_BASENAME__,		\
			   __LINE__, (fmt), ##__VA_ARGS__);		\
	} while (0)
#else
static inline void uk_vprintd(const char *fmt __unused, va_list ap __unused)
{}

static inline void uk_printd(const char *fmt, ...) __printf(1, 2);
static inline void uk_printd(const char *fmt __unused, ...)
{}
#endif

/*
 * KERNEL CONSOLE
 */
#define KLVL_INFO  (3)
#define KLVL_WARN  (2)
#define KLVL_ERR   (1)
#define KLVL_CRIT  (0)

#if CONFIG_LIBUKDEBUG_PRINTK_CRIT
#define KLVL_MAX KLVL_CRIT
#elif CONFIG_LIBUKDEBUG_PRINTK_ERR
#define KLVL_MAX KLVL_ERR
#elif CONFIG_LIBUKDEBUG_PRINTK_WARN
#define KLVL_MAX KLVL_WARN
#elif CONFIG_LIBUKDEBUG_PRINTK_INFO
#define KLVL_MAX KLVL_INFO
#else
#define KLVL_MAX KLVL_ERR /* default level */
#endif

#if CONFIG_LIBUKDEBUG_PRINTK
/* please use the uk_printd(), uk_vprintd() macros because
 * they compile in the function calls only if the configured
 * debug level requires it
 */
void _uk_vprintk(int lvl, const char *libname, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap);
void _uk_printk(int lvl, const char *libname, const char *srcname,
		unsigned int srcline, const char *fmt, ...) __printf(5, 6);

#define uk_vprintk(lvl, fmt, ap)                                               \
	do {                                                                   \
		if ((lvl) <= KLVL_MAX)                                         \
			_uk_vprintk((lvl), __STR_LIBNAME__, __STR_BASENAME__,  \
				    __LINE__, (fmt), ap);                      \
	} while (0)

#define uk_printk(lvl, fmt, ...)                                               \
	do {                                                                   \
		if ((lvl) <= KLVL_MAX)                                         \
			_uk_printk((lvl), __STR_LIBNAME__, __STR_BASENAME__,   \
				   __LINE__, (fmt), ##__VA_ARGS__);            \
	} while (0)
#else
static inline void uk_vprintk(int lvl __unused, const char *fmt __unused,
				va_list ap __unused)
{}

static inline void uk_printk(int lvl, const char *fmt, ...) __printf(2, 3);
static inline void uk_printk(int lvl __unused, const char *fmt __unused, ...)
{}
#endif /* CONFIG_LIBUKDEBUG_PRINTK */

/*
 * Convenience wrapper for uk_printk() and uk_printd()
 * This is similar to the pr_* variants that you find in the Linux kernel
 */
#define uk_pr_debug(fmt, ...) uk_printd((fmt), ##__VA_ARGS__)
#define uk_pr_info(fmt, ...)  uk_printk(KLVL_INFO,  (fmt), ##__VA_ARGS__)
#define uk_pr_warn(fmt, ...)  uk_printk(KLVL_WARN,  (fmt), ##__VA_ARGS__)
#define uk_pr_err(fmt, ...)   uk_printk(KLVL_ERR,   (fmt), ##__VA_ARGS__)
#define uk_pr_crit(fmt, ...)  uk_printk(KLVL_CRIT,  (fmt), ##__VA_ARGS__)

/* NOTE: borrowed from OSv */
#define WARN_STUBBED_ONCE(thing) do { \
	static int _x; \
	if (!_x) { \
		_x = 1; \
		thing; \
	} \
} while (0)

/* Warning for stubbed functions */
#define WARN_STUBBED() \
	WARN_STUBBED_ONCE(uk_pr_warn("%s() stubbed\n", __func__))

#ifdef __cplusplus
}
#endif

#endif /* __UKDEBUG_PRINT_H__ */
