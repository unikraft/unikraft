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

#ifndef __STRING_H__
#define __STRING_H__

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_NULL
#define __NEED_size_t
#include <nolibc-internal/shareddefs.h>

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *ptr, int val, size_t len);
void *memchr(const void *ptr, int val, size_t len);
void *memrchr(const void *m, int c, size_t n);
int memcmp(const void *ptr1, const void *ptr2, size_t len);
void *memmove(void *dst, const void *src, size_t len);

char *strncpy(char *dst, const char *src, size_t len);
char *strcpy(char *dst, const char *src);
size_t strlcpy(char *d, const char *s, size_t n);
size_t strlcat(char *d, const char *s, size_t n);
size_t strnlen(const char *str, size_t maxlen);
size_t strlen(const char *str);
char *strchrnul(const char *s, int c);
char *strchr(const char *str, int c);
char *strrchr(const char *s, int c);
int strncmp(const char *str1, const char *str2, size_t len);
int strcmp(const char *str1, const char *str2);
size_t strcspn(const char *s, const char *c);
size_t strspn(const char *s, const char *c);
char *strtok(char *restrict s, const char *restrict sep);
char *strndup(const char *str, size_t len);
char *strdup(const char *str);

char *strerror_r(int errnum, char *buf, size_t buflen);
char *strerror(int errnum);

#ifdef __cplusplus
}
#endif

#endif /* __STRING_H__ */
