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

#ifndef __INTTYPES_H__
#define __INTTYPES_H__

#include <stdint.h>
#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRId8  __PRIs8
#define PRId16 __PRIs16
#define PRId32 __PRIs32
#define PRId64 __PRIs64
#define PRIi8  __PRIs8
#define PRIi16 __PRIs16
#define PRIi32 __PRIs32
#define PRIi64 __PRIs64
#define PRIu8  __PRIu8
#define PRIu16 __PRIu16
#define PRIu32 __PRIu32
#define PRIu64 __PRIu64
#define PRIx8  __PRIx8
#define PRIx16 __PRIx16
#define PRIx32 __PRIx32
#define PRIx64 __PRIx64

#define SCNd8  __SCNs8
#define SCNd16 __SCNs16
#define SCNd32 __SCNs32
#define SCNd64 __SCNs64
#define SCNi8  __SCNs8
#define SCNi16 __SCNs16
#define SCNi32 __SCNs32
#define SCNi64 __SCNs64
#define SCNu8  __SCNu8
#define SCNu16 __SCNu16
#define SCNu32 __SCNu32
#define SCNu64 __SCNu64
#define SCNx8  __SCNx8
#define SCNx16 __SCNx16
#define SCNx32 __SCNx32
#define SCNx64 __SCNx64

typedef signed   long long intmax_t;
typedef unsigned long long uintmax_t;

#ifdef __cplusplus
}
#endif

#endif /* __INTTYPES_H__ */
