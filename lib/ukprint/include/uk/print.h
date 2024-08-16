/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PRINT_H__
#define __UK_PRINT_H__

#include <stdarg.h>

#include <uk/arch/lcpu.h>
#include <uk/bitops.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/libid.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __BASENAME__
#define __STR_BASENAME__ STRINGIFY(__BASENAME__)
#else
#define __STR_BASENAME__ (NULL)
#endif

/* Flags */
#define UK_PRINT_KLVL_DEBUG	5
#define UK_PRINT_KLVL_INFO	4
#define UK_PRINT_KLVL_WARN	3
#define UK_PRINT_KLVL_ERR	2
#define UK_PRINT_KLVL_CRIT	1
#define UK_PRINT_KLVL_NONE	0 /* print always */

#define UK_PRINT_RAW		UK_BIT(16) /* no meta */

#define UK_PRINT_KLVL_MASK	0xff
#define UK_PRINT_RAW_MASK	UK_PRINT_RAW

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
void _uk_vprintk(int flags, __u16 libid, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap);
void _uk_printk(int flags, __u16 libid, const char *srcname,
		unsigned int srcline, const char *fmt, ...) __printf(5, 6);

#if defined UK_DEBUG
#define UK_PRINTK_EN(_flags)						\
	((((_flags) & UK_PRINT_KLVL_MASK) <= UK_PRINT_KLVL_MAX) ||	\
	 (((_flags) & UK_PRINT_KLVL_MASK) == UK_PRINT_KLVL_DEBUG))
#else /* !defined UK_DEBUG */
#define UK_PRINTK_EN(_flags)				\
	(((_flags) & UK_PRINT_KLVL_MASK) <= UK_PRINT_KLVL_MAX)
#endif /* !defined UK_DEBUG */

#define uk_vprintk(flags, fmt, ap)                                             \
	do {                                                                   \
		if (UK_PRINTK_EN(flags))                                       \
			_uk_vprintk((flags), uk_libid_self(), __STR_BASENAME__,\
				    __LINE__, (fmt), ap);                      \
	} while (0)

#define uk_vprintk_once(flags, fmt, ap)                                        \
	do {                                                                   \
		if (UK_PRINTK_EN(flags)) {                                     \
			static int __x;                                        \
			if (unlikely(!__x)) {                                  \
				_uk_vprintk((flags), uk_libid_self(),          \
					    __STR_BASENAME__,                  \
					    __LINE__, (fmt), ap);              \
				__x = 1;                                       \
			}                                                      \
		}                                                              \
	} while (0)

#define uk_printk(flags, fmt, ...)                                             \
	do {                                                                   \
		if (UK_PRINTK_EN(flags))                                       \
			_uk_printk((flags), uk_libid_self(), __STR_BASENAME__, \
				   __LINE__, (fmt), ##__VA_ARGS__);            \
	} while (0)

#define uk_printk_once(flags, fmt, ...)                                        \
	do {                                                                   \
		if (UK_PRINTK_EN(flags)) {                                     \
			static int __x;                                        \
			if (unlikely(!__x)) {                                  \
				_uk_printk((flags), uk_libid_self(),           \
					   __STR_BASENAME__,                   \
					   __LINE__, (fmt), ##__VA_ARGS__);    \
				__x = 1;                                       \
			}                                                      \
		}                                                              \
	} while (0)
#else /* !(CONFIG_LIBUKPRINT_PRINTK) */
static inline void uk_vprintk(int flags __unused, const char *fmt __unused,
			      va_list ap __unused)
{}

static inline void uk_printk(int flags, const char *fmt, ...) __printf(2, 3);
static inline void uk_printk(int flags __unused, const char *fmt __unused, ...)
{}

static inline void uk_vprintk_once(int flags __unused, const char *fmt __unused,
				   va_list ap __unused)
{}

static inline void uk_printk_once(int flags, const char *fmt, ...) __printf(2, 3);
static inline void uk_printk_once(int flags __unused,
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
