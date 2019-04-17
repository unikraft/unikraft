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

#ifndef __LIMITS_H__
#define __LIMITS_H__

#include <uk/arch/limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHAR_BITS    __C_BITS
#define CHAR_MIN      __C_MIN
#define CHAR_MAX      __C_MAX
#define UCHAR_MAX    __UC_MAX

#define SHRT_MIN      __S_MIN
#define SHRT_MAX      __S_MAX
#define USHRT_MAX    __US_MAX

#define INT_MIN       __I_MIN
#define INT_MAX       __I_MAX
#define UINT_MAX     __UI_MAX

#define LONG_MIN      __L_MIN
#define LONG_MAX      __L_MAX
#define ULONG_MAX    __UL_MAX

#define LLONG_MIN    __LL_MIN
#define LLONG_MAX    __LL_MAX
#define ULLONG_MAX  __ULL_MAX

#define PATH_MAX 4096
#define NAME_MAX 255

#ifdef __cplusplus
}
#endif

#endif /* __LIMITS_H__ */
