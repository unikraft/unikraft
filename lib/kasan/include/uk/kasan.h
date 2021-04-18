/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Badoiu Vlad-Andrei <vlad_andrei.badoiu@upb.ro>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
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
 */

#ifndef _SYS_KASAN_H_
#define _SYS_KASAN_H_

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KASAN_CODE_STACK_LEFT 0xF1
#define KASAN_CODE_STACK_MID 0xF2
#define KASAN_CODE_STACK_RIGHT 0xF3

/* Our own redzone codes */
#define KASAN_CODE_GLOBAL_OVERFLOW 0xFA
#define KASAN_CODE_KMEM_FREED 0xFB
#define KASAN_CODE_POOL_OVERFLOW 0xFC
#define KASAN_CODE_POOL_FREED 0xFD
#define KASAN_CODE_KMALLOC_OVERFLOW 0xFE
#define KASAN_CODE_KMALLOC_FREED 0xFF

/* Redzone sizes for instrumented allocators */
#define KASAN_KMALLOC_REDZONE_SIZE 8

/* Shadow mem size */
#define KASAN_MD_SHADOW_SIZE (16 * (1 << 40)) /* 16 TB */

/* Initialize KASAN subsystem. */
void init_kasan(void *base);

/* Mark bytes as valid (in the shadow memory) */
void kasan_mark_valid(const void *addr, size_t size);

/* Mark bytes as invalid (in the shadow memory) */
void kasan_mark_invalid(const void *addr, size_t size, uint8_t code);

/* Mark first 'size' bytes as valid (in the shadow memory), and the remaining
 * (size_with_redzone - size) bytes as invalid with given code.
 */
void kasan_mark(const void *addr, size_t size, size_t size_with_redzone,
		uint8_t code);

#ifdef __cplusplus
}
#endif

#endif /* !_SYS_KASAN_H_ */
