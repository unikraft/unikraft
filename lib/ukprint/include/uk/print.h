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
 * DEBUG PRINTING
 */
void _uk_vprintd(__u16 libid, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap);
void _uk_printd(__u16 libid, const char *srcname,
		unsigned int srcline, const char *fmt, ...) __printf(4, 5);

#ifdef __IN_LIBUKPRINT__
/*
 * This redefinition of CONFIG_LIBUKPRINT_PRINTD is doing the trick to avoid
 * multiple declarations of uk_{v}printd() when we are compiling this library
 * and have the global debug switch CONFIG_LIBUKPRINT_PRINTD not enabled.
 */
#if !defined CONFIG_LIBUKPRINT_PRINTD || !CONFIG_LIBUKPRINT_PRINTD
#undef CONFIG_LIBUKPRINT_PRINTD
#define CONFIG_LIBUKPRINT_PRINTD 1
#endif
#endif /* __IN_LIBUKPRINT__ */

#if defined UK_DEBUG || CONFIG_LIBUKPRINT_PRINTD
#define uk_vprintd(fmt, ap)						\
	do {								\
		_uk_vprintd(uk_libid_self(), __STR_BASENAME__,		\
			    __LINE__, (fmt), ap);			\
	} while (0)

#define uk_vprintd_once(fmt, ap)					\
	do {								\
		static int __x;						\
		if (unlikely(!__x)) {					\
			_uk_vprintd(uk_libid_self(), __STR_BASENAME__,	\
				    __LINE__, (fmt), ap);		\
			__x = 1;					\
		}							\
	} while (0)

#define uk_printd(fmt, ...)						\
	do {								\
		_uk_printd(uk_libid_self(), __STR_BASENAME__,		\
			   __LINE__, (fmt), ##__VA_ARGS__);		\
	} while (0)

#define uk_printd_once(fmt, ...)					\
	do {								\
		static int __x;						\
		if (unlikely(!__x)) {					\
			_uk_printd(uk_libid_self(), __STR_BASENAME__,	\
				   __LINE__, (fmt), ##__VA_ARGS__);	\
			__x = 1;					\
		}							\
	} while (0)
#else
static inline void uk_vprintd(const char *fmt __unused, va_list ap __unused)
{}

static inline void uk_printd(const char *fmt, ...) __printf(1, 2);
static inline void uk_printd(const char *fmt __unused, ...)
{}

static inline void uk_vprintd_once(const char *fmt __unused,
				   va_list ap __unused)
{}

static inline void uk_printd_once(const char *fmt, ...) __printf(1, 2);
static inline void uk_printd_once(const char *fmt __unused, ...)
{}
#endif

/*
 * KERNEL CONSOLE
 */
#define UK_PRINT_KLVL_INFO	3
#define UK_PRINT_KLVL_WARN	2
#define UK_PRINT_KLVL_ERR	1
#define UK_PRINT_KLVL_CRIT	0

#if CONFIG_LIBUKPRINT_PRINTK_CRIT
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

#if CONFIG_LIBUKPRINT_PRINTK
/* please use the uk_printd(), uk_vprintd() macros because
 * they compile in the function calls only if the configured
 * debug level requires it
 */
void _uk_vprintk(int lvl, __u16 libid, const char *srcname,
		 unsigned int srcline, const char *fmt, va_list ap);
void _uk_printk(int lvl, __u16 libid, const char *srcname,
		unsigned int srcline, const char *fmt, ...) __printf(5, 6);

#define uk_vprintk(lvl, fmt, ap)                                               \
	do {                                                                   \
		if ((lvl) <= UK_PRINT_KLVL_MAX)                                \
			_uk_vprintk((lvl), uk_libid_self(), __STR_BASENAME__,  \
				    __LINE__, (fmt), ap);                      \
	} while (0)

#define uk_vprintk_once(lvl, fmt, ap)                                          \
	do {                                                                   \
		if ((lvl) <= UK_PRINT_KLVL_MAX) {                              \
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
		if ((lvl) <= UK_PRINT_KLVL_MAX)                                \
			_uk_printk((lvl), uk_libid_self(), __STR_BASENAME__,   \
				   __LINE__, (fmt), ##__VA_ARGS__);            \
	} while (0)

#define uk_printk_once(lvl, fmt, ...)                                          \
	do {                                                                   \
		if ((lvl) <= UK_PRINT_KLVL_MAX) {                              \
			static int __x;                                        \
			if (unlikely(!__x)) {                                  \
				_uk_printk((lvl), uk_libid_self(),             \
					   __STR_BASENAME__,                   \
					   __LINE__, (fmt), ##__VA_ARGS__);    \
				__x = 1;                                       \
			}                                                      \
		}                                                              \
	} while (0)
#else
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
#endif /* CONFIG_LIBUKPRINT_PRINTK */

/*
 * Convenience wrapper for uk_printk() and uk_printd()
 * This is similar to the pr_* variants that you find in the Linux kernel
 */
#define uk_pr_debug(fmt, ...) uk_printd((fmt), ##__VA_ARGS__)
#define uk_pr_debug_once(fmt, ...) uk_printd_once((fmt), ##__VA_ARGS__)

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
