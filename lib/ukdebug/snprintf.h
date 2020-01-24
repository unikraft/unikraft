/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKDEBUG_INTERNAL_SNPRINTF_H__
#define __UKDEBUG_INTERNAL_SNPRINTF_H__

#include <uk/config.h>

/*
 * Point __uk_(v)snprintf to library-internal implementation as soon as
 * we do not use lib/nolibc as libc.
 */
#if CONFIG_LIBNOLIBC
#include <stdio.h>

#define __uk_vsnprintf(...) vsnprintf(__VA_ARGS__)
#define  __uk_snprintf(...)  snprintf(__VA_ARGS__)

#else /* CONFIG_LIBNOLIBC */
#include <stddef.h>
#include <stdarg.h>
#include <uk/essentials.h>

int __uk_vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int  __uk_snprintf(char *str, size_t size, const char *fmt, ...) __printf(3, 4);

#endif /* CONFIG_LIBNOLIBC */

#endif /* __UKDEBUG_INTERNAL_SNPRINTF_H__ */
