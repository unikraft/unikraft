/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PRINT_SNPRINTF_H__
#define __UK_PRINT_SNPRINTF_H__

#include <uk/config.h>

/*
 * Point uk_(v)snprintf to library-internal implementation as soon as
 * we do not use lib/nolibc as libc.
 */
#if CONFIG_LIBNOLIBC
#include <stdio.h>

#define uk_vsnprintf(...) vsnprintf(__VA_ARGS__)
#define uk_snprintf(...)  snprintf(__VA_ARGS__)

#else /* !CONFIG_LIBNOLIBC */
#include <stddef.h>
#include <stdarg.h>
#include <uk/essentials.h>

int uk_vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int uk_snprintf(char *str, size_t size, const char *fmt, ...) __printf(3, 4);

#endif /* !CONFIG_LIBNOLIBC */

#endif /* __UK_PRINT_SNPRINTF_H__ */
