/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PRINT_H__
#define __UK_PRINT_H__

#include <stdarg.h>
#include <uk/libid.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>
#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __BASENAME__
#define __STR_BASENAME__ STRINGIFY(__BASENAME__)
#else
#define __STR_BASENAME__ (NULL)
#endif

/*
 * KERNEL CONSOLE
 */
#define UK_PRINT_KLVL_DEBUG	4
#define UK_PRINT_KLVL_INFO	3
#define UK_PRINT_KLVL_WARN	2
#define UK_PRINT_KLVL_ERR	1
#define UK_PRINT_KLVL_CRIT	0

#if CONFIG_LIBUKPRINT_PRINTK_DEBUG
#define UK_PRINT_KLVL_MAX UK_PRINT_KLVL_DEBUG
#elif CONFIG_LIBUKPRINT_PRINTK_CRIT
#define UK_PRINT_KLVL_MAX UK_PRINT_KLVL_CRIT
#elif CONFIG_LIBUKPRINT_PRINTK_ERR
#define UK_PRINT_KLVL_MAX UK_PRINT_KLVL_ERR
#elif CONFIG_LIBUKPRINT_PRINTK_WARN
#define UK_PRINT_KLVL_MAX UK_PRINT_KLVL_WARN
#elif CONFIG_LIBUKPRINT_PRINTK_INFO
#define UK_PRINT_KLVL_MAX UK_PRINT_KLVL_INFO
#else
#define UK_PRINT_KLVL_MAX UK_PRINT_KLVL_ERR /* default level */
#endif

#if CONFIG_LIBUKPRINT
/* please use the uk_{v}printk() macros because they compile
 * in the function calls only if the configured debug level
 * requires it
 */
void _uk_vprintk(int lvl, __u16 libid, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap);
void _uk_printk(int lvl, __u16 libid, const char *srcname,
		unsigned int srcline, const char *fmt, ...) __printf(5, 6);

#if defined UK_DEBUG
#define UK_PRINTK_EN(_lvl)				\
	(((_lvl) <= UK_PRINT_KLVL_MAX) ||		\
	 ((_lvl) == UK_PRINT_KLVL_DEBUG))
#else /* !defined UK_DEBUG */
#define UK_PRINTK_EN(_lvl)				\
	(((_lvl) <= UK_PRINT_KLVL_MAX))
#endif /* !defined UK_DEBUG */

#define uk_vprintk(lvl, fmt, ap)                                               \
	do {                                                                   \
		if (UK_PRINTK_EN(lvl))                                         \
			_uk_vprintk((lvl), uk_libid_self(), __STR_BASENAME__,  \
				    __LINE__, (fmt), ap);                      \
	} while (0)

#define uk_vprintk_once(lvl, fmt, ap)                                          \
	do {                                                                   \
		if (UK_PRINTK_EN(lvl)) {                                       \
			static int __x;                                        \
			if (unlikely(!__x)) {                                  \
				_uk_vprintk((lvl), uk_libid_self(),            \
					    __STR_BASENAME__,                  \
					    __LINE__, (fmt), ap);              \
				__x = 1;                                       \
			}                                                      \
		}                                                              \
	} while (0)

#define uk_printk(lvl, fmt, ...)                                               \
	do {                                                                   \
		if (UK_PRINTK_EN(lvl))                                         \
			_uk_printk((lvl), uk_libid_self(), __STR_BASENAME__,   \
				   __LINE__, (fmt), ##__VA_ARGS__);            \
	} while (0)

#define uk_printk_once(lvl, fmt, ...)                                          \
	do {                                                                   \
		if (UK_PRINTK_EN(lvl)) {                                       \
			static int __x;                                        \
			if (unlikely(!__x)) {                                  \
				_uk_printk((lvl), uk_libid_self(),             \
					   __STR_BASENAME__,                   \
					   __LINE__, (fmt), ##__VA_ARGS__);    \
				__x = 1;                                       \
			}                                                      \
		}                                                              \
	} while (0)
#else /* !(CONFIG_LIBUKPRINT_PRINTK) */
static inline void uk_vprintk(int lvl __unused, const char *fmt __unused,
			      va_list ap __unused)
{}

static inline void uk_printk(int lvl, const char *fmt, ...) __printf(2, 3);
static inline void uk_printk(int lvl __unused, const char *fmt __unused, ...)
{}

static inline void uk_vprintk_once(int lvl __unused, const char *fmt __unused,
				   va_list ap __unused)
{}

static inline void uk_printk_once(int lvl, const char *fmt, ...) __printf(2, 3);
static inline void uk_printk_once(int lvl __unused,
				  const char *fmt __unused, ...)
{}
#endif /* !(CONFIG_LIBUKPRINT_PRINTK) */

/*
 * Convenience wrappers for uk_printk(). This is similar to the
 * pr_* variants that you find in the Linux kernel
 */
#define uk_pr_debug(fmt, ...)					\
	uk_printk(UK_PRINT_KLVL_DEBUG, (fmt), ##__VA_ARGS__)

#define uk_pr_debug_once(fmt, ...)				\
	uk_printk_once(UK_PRINT_KLVL_DEBUG, (fmt), ##__VA_ARGS__)

#define uk_pr_info(fmt, ...)					\
	uk_printk(UK_PRINT_KLVL_INFO, (fmt), ##__VA_ARGS__)

#define uk_pr_info_once(fmt, ...)				\
	uk_printk_once(UK_PRINT_KLVL_INFO, (fmt), ##__VA_ARGS__)

#define uk_pr_warn(fmt, ...)					\
	uk_printk(UK_PRINT_KLVL_WARN, (fmt), ##__VA_ARGS__)

#define uk_pr_warn_once(fmt, ...)				\
	uk_printk_once(UK_PRINT_KLVL_WARN, (fmt), ##__VA_ARGS__)

#define uk_pr_err(fmt, ...)					\
	uk_printk(UK_PRINT_KLVL_ERR, (fmt), ##__VA_ARGS__)

#define uk_pr_err_once(fmt, ...)				\
	uk_printk_once(UK_PRINT_KLVL_ERR, (fmt), ##__VA_ARGS__)

#define uk_pr_crit(fmt, ...)					\
	uk_printk(UK_PRINT_KLVL_CRIT, (fmt), ##__VA_ARGS__)

#define uk_pr_crit_once(fmt, ...)				\
	uk_printk_once(UK_PRINT_KLVL_CRIT, (fmt), ##__VA_ARGS__)

/* Warning for stubbed functions */
#define UK_WARN_STUBBED() \
	uk_pr_warn_once("%s() stubbed\n", __func__)

/* DEPRECATED: Please use UK_WARN_STUBBED instead */
#ifndef WARN_STUBBED
#define WARN_STUBBED() \
	UK_WARN_STUBBED()
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UK_PRINT_H__ */
