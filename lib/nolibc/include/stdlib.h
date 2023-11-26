/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009 Citrix Systems, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <uk/config.h>
#include <uk/essentials.h>
#if CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_NULL
#define __NEED_size_t
#include <nolibc-internal/shareddefs.h>

/**
 * Convert a string to an unsigned long integer.
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 *
 * @param nptr The start of the string
 * @param endptr A pointer to the end of the parsed string will be placed here
 * @param base The number base to use
 */
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);

/**
 * Convert a string to an integer
 * @param s The start of the string
 */
int atoi(const char *s);
long atol(const char *s);

#if CONFIG_LIBUKALLOC

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
int posix_memalign(void **memptr, size_t align, size_t size);
void *memalign(size_t align, size_t size);

#endif /* CONFIG_LIBUKALLOC */
void abort(void) __noreturn;

void exit(int status) __noreturn;

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));

#if CONFIG_LIBPOSIX_ENVIRON
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int clearenv(void);
int putenv(char *string);
char *getenv(const char *name);
#endif /* CONFIG_LIBPOSIX_ENVIRON */

#if CONFIG_LIBPOSIX_PROCESS
int system(const char *command);
#endif

char *setstate(char *state);
char *initstate(unsigned int seed, char *state, size_t size);
void srandom(unsigned int seed);
long random(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H__ */
