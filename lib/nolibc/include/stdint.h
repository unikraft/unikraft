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

#ifndef __STDINT_H__
#define __STDINT_H__

#include <uk/arch/types.h>
#include <uk/arch/limits.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef __s8  int8_t;
typedef __s16 int16_t;
typedef __s32 int32_t;
typedef __s64 int64_t;

typedef __u8  uint8_t;
typedef __u16 uint16_t;
typedef __u32 uint32_t;
typedef __u64 uint64_t;

typedef __uptr uintptr_t;
typedef __sptr intptr_t;

#define INT8_MIN     __S8_MIN
#define INT8_MAX     __S8_MAX
#define UINT8_MAX    __U8_MAX

#define INT16_MIN   __S16_MIN
#define INT16_MAX   __S16_MAX
#define UINT16_MAX  __U16_MAX

#define INT32_MIN   __S32_MIN
#define INT32_MAX   __S32_MAX
#define UINT32_MAX  __U32_MAX

#define INT64_MIN   __S64_MIN
#define INT64_MAX   __S64_MAX
#define UINT64_MAX  __U64_MAX

#define SSIZE_MIN   __SSZ_MIN
#define SSIZE_MAX   __SSZ_MAX
#define SIZE_MAX     __SZ_MAX

#define INTMAX_MIN   __LL_MIN
#define INTMAX_MAX   __LL_MAX
#define UINTMAX_MAX __ULL_MAX

#ifdef __cplusplus
}
#endif

#endif /* __STDINT_H__ */
