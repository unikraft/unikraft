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
 * @nptr:   The start of the string
 * @endptr: A pointer to the end of the parsed string will be placed here
 * @base:   The number base to use
 */
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);

/**
 * Convert a string to an integer
 * @s: The start of the string
 */
int atoi(const char *s);

#if CONFIG_LIBUKALLOC
/* Allocate size bytes of memory. Returns pointer to start of allocated memory,
 * or NULL on failure.
 */
void *malloc(size_t size);
/* Release memory previously allocated by malloc(). ptr must be a pointer
 * previously returned by malloc(), otherwise undefined behavior will occur.
 */
void free(void *ptr);
/* Allocate memory for an array of nmemb elements of size bytes. Returns
 * pointer to start of allocated memory, or NULL on failure.
 */
void *calloc(size_t nmemb, size_t size);
/* Change the size of the memory block pointed to by ptr to size bytes.
 * Returns a pointer to the resized memory area. If the area pointed to was
 * moved, a free(ptr) is done.
 */
void *realloc(void *ptr, size_t size);
/* Allocate size bytes of memory, aligned to align bytes, and return the
 * pointer to that area in *memptr. Returns 0 on success, and a non-zero error
 * value on failure.
 */
int posix_memalign(void **memptr, size_t align, size_t size);
/* Allocate size bytes of memory, aligned to align bytes. Returns pointer to
 * start of allocated memory, or NULL on failure.
 */
void *memalign(size_t align, size_t size);
#endif /* CONFIG_LIBUKALLOC */

void abort(void) __noreturn;

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));

#if CONFIG_LIBPOSIX_PROCESS
int system(const char *command);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H__ */
