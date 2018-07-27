/* SPDX-License-Identifier: BSD-3-Clause */
/*
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

#ifndef __STDIO_H__
#define __STDIO_H__

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_NULL
#define __NEED_size_t
#define __NEED_ssize_t
#include <nolibc-internal/shareddefs.h>

struct _nolibc_fd;
typedef struct _nolibc_fd FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* stdio.h shall not define va_list if it is included, but it shall
 * declare functions that use va_list.
 */
#ifndef va_list
#define __STDIO_H_DEFINED_va_list
#define va_list __builtin_va_list
#endif

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int  snprintf(char *str, size_t size, const char *fmt, ...) __printf(3, 4);

int vsprintf(char *str, const char *fmt, va_list ap);
int  sprintf(char *str, const char *fmt, ...)               __printf(2, 3);

int vfprintf(FILE *fp, const char *fmt, va_list ap);
int  fprintf(FILE *fp, const char *fmt, ...)                __printf(2, 3);
int   fflush(FILE *fp);

int vprintf(const char *fmt, va_list ap);
int  printf(const char *fmt, ...)                           __printf(1, 2);

int vsscanf(const char *str, const char *fmt, va_list ap);
int  sscanf(const char *str, const char *fmt, ...)          __scanf(2, 3);

int vasprintf(char **str, const char *fmt, va_list ap);
int  asprintf(char **str, const char *fmt, ...)             __printf(2, 3);

#ifdef __STDIO_H_DEFINED_va_list
#undef va_list
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STDIO_H__ */
