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

#ifdef __IN_LIBUKDEBUG__
/*
 * These defines are doing the trick to compile the functions
 * in print.c always in although printing was disabled
 * in the configuration. This is required for linking with
 * pre-compiled objects that built by using a different configuration.
 */
#if !CONFIG_LIBUKDEBUG_PRINTK
#undef CONFIG_LIBUKDEBUG_PRINTK
#define CONFIG_LIBUKDEBUG_PRINTK 1
#endif
#if !CONFIG_LIBUKDEBUG_PRINTD
#undef CONFIG_LIBUKDEBUG_PRINTD
#define CONFIG_LIBUKDEBUG_PRINTD 1
#endif
#endif /* __IN_LIBUKDEBUG__ */

/*
 * KERNEL CONSOLE
 */
#if CONFIG_LIBUKDEBUG_PRINTK
void uk_vprintk(const char *fmt, va_list ap);
void uk_printk(const char *fmt, ...) __printf(1, 2);
#else
static inline void uk_vprintk(const char *fmt, va_list ap)
{
}
static inline void uk_printk(const char *fmt, ...) __printf(1, 2);
static inline void uk_printk(const char *fmt, ...)
{
}
#endif

/*
 * DEBUG CONSOLE
 */
#define DLVL_EXTRA (4)
#define DLVL_INFO  (3)
#define DLVL_WARN  (2)
#define DLVL_ERR   (1)
#define DLVL_CRIT  (0)

#if CONFIG_LIBUKDEBUG_PRINTD_CRIT
#define DLVL_MAX DLVL_CRIT
#elif CONFIG_LIBUKDEBUG_PRINTD_ERR
#define DLVL_MAX DLVL_ERR
#elif CONFIG_LIBUKDEBUG_PRINTD_WARN
#define DLVL_MAX DLVL_WARN
#elif CONFIG_LIBUKDEBUG_PRINTD_INFO
#define DLVL_MAX DLVL_INFO
#elif CONFIG_LIBUKDEBUG_PRINTD_EXTRA
#define DLVL_MAX DLVL_EXTRA
#else
#define DLVL_MAX DLVL_ERR /* default level */
#endif

#if CONFIG_LIBUKDEBUG_PRINTD
/* please use the uk_printd(), uk_vprintd() macros because
 * they compile in the function calls only if the configured
 * debug level requires it
 */
void _uk_vprintd(int lvl, const char *libname, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap);
void _uk_printd(int lvl, const char *libname, const char *srcname,
		unsigned int srcline, const char *fmt, ...) __printf(5, 6);

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

#define uk_vprintd(lvl, fmt, ap)                                               \
	do {                                                                   \
		if ((lvl) <= DLVL_MAX)                                         \
			_uk_vprintd((lvl), __STR_LIBNAME__, __STR_BASENAME__,  \
				    __LINE__, (fmt), ap);                      \
	} while (0)

#define uk_printd(lvl, fmt, ...)                                               \
	do {                                                                   \
		if ((lvl) <= DLVL_MAX)                                         \
			_uk_printd((lvl), __STR_LIBNAME__, __STR_BASENAME__,   \
				   __LINE__, (fmt), ##__VA_ARGS__);            \
	} while (0)
#else
static inline void uk_vprintd(int lvl __unused, const char *fmt __unused,
				va_list ap __unused)
{
}
static inline void uk_printd(int lvl, const char *fmt, ...) __printf(2, 3);
static inline void uk_printd(int lvl __unused, const char *fmt __unused, ...)
{
}
#endif /* CONFIG_LIBUKDEBUG_PRINTD */

/* Print a message on both: Kernel console and Debug console */
#define uk_printkd(dlvl, fmt, ...)                                             \
	do {                                                                   \
		uk_printk((fmt), ##__VA_ARGS__);                               \
		uk_printd((dlvl), (fmt), ##__VA_ARGS__);                       \
	} while (0)

/*
 * Convenience wrapper for uk_printd()
 * This is similar to the pr_* variants that you find in the Linux kernel
 */
#define uk_pr_debug(fmt, ...) uk_printd(DLVL_EXTRA, (fmt), ##__VA_ARGS__)
#define uk_pr_info(fmt, ...)  uk_printd(DLVL_INFO,  (fmt), ##__VA_ARGS__)
#define uk_pr_warn(fmt, ...)  uk_printd(DLVL_WARN,  (fmt), ##__VA_ARGS__)
#define uk_pr_err(fmt, ...)   uk_printd(DLVL_ERR,   (fmt), ##__VA_ARGS__)
#define uk_pr_crit(fmt, ...)  uk_printd(DLVL_CRIT,  (fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __UKDEBUG_PRINT_H__ */
