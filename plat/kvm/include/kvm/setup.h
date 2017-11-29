/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Ricardo Koller
 *          Martin Lucina
 *          Wei Chen
 *          Felipe Huici <felipe.huici@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2017 NEC Europe Ltd., NEC Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <inttypes.h>
#include <uk/assert.h>
#include <uk/essentials.h>

/* alignment macros */
#define ALIGN_4K __align(0x1000)
#define ALIGN_64_BIT __align(0x8)

/* convenient macro stringification */
#define STR_EXPAND(y) #y
#define STR(x) STR_EXPAND(x)

#define assert(e) UK_ASSERT(e)
#define PANIC(s) assert(e) //kludge!

/* platform.c: platform includes */
void platform_init(void *arg);
char *platform_cmdline(void);
uint64_t platform_mem_size(void);
void platform_exit(void) __attribute__((noreturn));
