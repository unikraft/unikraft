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

#ifndef __UKARCH_LIMITS_H__
#define __UKARCH_LIMITS_H__

#include <uk/config.h>
#include <uk/asm/limits.h>

#define STACK_MASK_TOP           (~(__STACK_SIZE - 1))

#ifndef __ASSEMBLY__

#include <uk/asm/intsizes.h>

#if (defined __C_IS_8)
#define __C_MAX             (127)
#define __C_MAX_U          (127U)
#define __C_MIN    (-__C_MAX - 1)
#define __UC_MAX           (255U)
#define __UC_MIN             (0U)
#define	__S8_MAX          __C_MAX
#define	__S8_MAX_U      __C_MAX_U
#define __S8_MIN          __C_MIN
#define	__U8_MAX         __UC_MAX
#define __U8_MIN         __UC_MIN
#define	__C_BITS              (8)
#define __HAVE_INT8__
#undef __C_IS_8__
#endif

#if (defined __S_IS_8)
#define __S_MAX             (127)
#define __S_MAX_U          (127U)
#define __S_MIN    (-__S_MAX - 1)
#define __US_MAX           (255U)
#define __US_MIN             (0U)
#undef __S_IS_8
#elif (defined __S_IS_16)
#define __S_MAX           (32767)
#define __S_MAX_U        (32767U)
#define __S_MIN    (-__S_MAX - 1)
#define __US_MAX         (65535U)
#define __US_MIN             (0U)
#define	__S16_MAX         __S_MAX
#define	__S16_MAX_U     __S_MAX_U
#define __S16_MIN         __S_MIN
#define	__U16_MAX        __US_MAX
#define __U16_MIN        __US_MIN
#define __HAVE_INT16__
#undef __S_IS_16
#endif

#if (defined __I_IS_16)
#define __I_MAX           (32767)
#define __I_MAX_U        (32767U)
#define __I_MIN    (-__I_MAX - 1)
#define __UI_MAX         (65535U)
#define __UI_MIN             (0U)
#ifndef __HAVE_INT16__
#define	__S16_MAX         __I_MAX
#define	__S16_MAX_U     __I_MAX_U
#define __S16_MIN         __I_MIN
#define	__U16_MAX        __UI_MAX
#define __U16_MIN        __UI_MIN
#define __HAVE_INT16__
#endif
#undef __I_IS_16
#elif (defined __I_IS_32)
#define __I_MAX      (2147483647)
#define __I_MAX_U   (2147483647U)
#define __I_MIN    (-__I_MAX - 1)
#define __UI_MAX    (4294967295U)
#define __UI_MIN             (0U)
#ifndef __HAVE_INT32__
#define	__S32_MAX         __I_MAX
#define	__S32_MAX_U     __I_MAX_U
#define __S32_MIN         __I_MIN
#define	__U32_MAX        __UI_MAX
#define __U32_MIN        __UI_MIN
#define __HAVE_INT32__
#endif
#undef __I_IS_32
#elif (defined __I_IS_64)
#define __I_MAX      (9223372036854775807)
#define __I_MAX_U   (9223372036854775807U)
#define __I_MIN             (-__I_MAX - 1)
#define __UI_MAX   (18446744073709551615U)
#define __UI_MIN                      (0U)
#ifndef __HAVE_INT64__
#define	__S64_MAX                  __I_MAX
#define	__S64_MAX_U              __I_MAX_U
#define __S64_MIN                  __I_MIN
#define	__U64_MAX                 __UI_MAX
#define __U64_MIN                 __UI_MIN
#define __HAVE_INT64__
#endif
#undef __I_IS_64
#endif

#if (defined __L_IS_32)
#define __L_MAX     (2147483647L)
#define __L_MAX_U  (2147483647UL)
#define __L_MIN   (-__L_MAX - 1L)
#define __UL_MAX   (4294967295UL)
#define __UL_MIN            (0UL)
#ifndef __HAVE_INT32__
#define	__S32_MAX         __L_MAX
#define	__S32_MAX_U     __L_MAX_U
#define __S32_MIN         __L_MIN
#define	__U32_MAX        __UL_MAX
#define __U32_MIN        __UL_MIN
#define __HAVE_INT32__
#endif
#undef __L_IS_32
#elif (defined __L_IS_64)
#define __L_MAX      (9223372036854775807L)
#define __L_MAX_U   (9223372036854775807UL)
#define __L_MIN             (-__L_MAX - 1L)
#define __UL_MAX   (18446744073709551615UL)
#define __UL_MIN                      (0UL)
#ifndef __HAVE_INT64__
#define	__S64_MAX                   __L_MAX
#define	__S64_MAX_U               __L_MAX_U
#define __S64_MIN                   __L_MIN
#define	__U64_MAX                  __UL_MAX
#define __U64_MIN                  __UL_MIN
#define __HAVE_INT64__
#endif
#undef __L_IS_64
#endif

#if (defined __LL_IS_32)
#define __LL_MAX     (2147483647LL)
#define __LL_MAX_U  (2147483647ULL)
#define __LL_MIN  (-__LL_MAX - 1LL)
#define __ULL_MAX   (4294967295ULL)
#define __ULL_MIN            (0ULL)
#ifndef __HAVE_INT32__
#define	__S32_MAX         __LL_MAX
#define	__S32_MAX_U     __LL_MAX_U
#define __S32_MIN         __LL_MIN
#define	__U32_MAX        __ULL_MAX
#define __U32_MIN        __ULL_MIN
#define __HAVE_INT32__
#endif
#undef __LL_IS_32
#elif (defined __LL_IS_64)
#define __LL_MAX    (9223372036854775807LL)
#define __LL_MAX_U (9223372036854775807ULL)
#define __LL_MIN          (-__LL_MAX - 1LL)
#define __ULL_MAX (18446744073709551615ULL)
#define __ULL_MIN                    (0ULL)
#ifndef __HAVE_INT64__
#define	__S64_MAX                  __LL_MAX
#define	__S64_MAX_U              __LL_MAX_U
#define __S64_MIN                  __LL_MIN
#define	__U64_MAX                 __ULL_MAX
#define __U64_MIN                 __ULL_MIN
#define __HAVE_INT64__
#endif
#undef __LL_IS_64
#endif

#if (defined __PTR_IS_16)
#define __PTR_MAX __U16_MAX
#define __PTR_MIN __U16_MIN
#define __SZ_MAX  __S16_MAX_U
#define __SZ_MIN  __U16_MIN
#define __SSZ_MAX __S16_MAX
#define __SSZ_MIN __S16_MIN
#define __OFF_MAX __S16_MAX
#define __OFF_MIN __S16_MIN
#define __HAVE_PTR__
#elif (defined __PTR_IS_32)
#define __PTR_MAX __U32_MAX
#define __PTR_MIN __U32_MIN
#define __SZ_MAX  __S32_MAX_U
#define __SZ_MIN  __U32_MIN
#define __SSZ_MAX __S32_MAX
#define __SSZ_MIN __S32_MIN
#define __OFF_MAX __S32_MAX
#define __OFF_MIN __S32_MIN
#define __HAVE_PTR__
#elif (defined __PTR_IS_64)
#define __PTR_MAX __U64_MAX
#define __PTR_MIN __U64_MIN
#define __SZ_MAX  __S64_MAX_U
#define __SZ_MIN  __U64_MIN
#define __SSZ_MAX __S64_MAX
#define __SSZ_MIN __S64_MIN
#define __OFF_MAX __S64_MAX
#define __OFF_MIN __S64_MIN
#define __HAVE_PTR__
#endif

/* Clean-up */
#ifdef __HAVE_INT8__
#undef __HAVE_INT8__
#endif
#ifdef __HAVE_INT16__
#undef __HAVE_INT16__
#endif
#ifdef __HAVE_INT32__
#undef __HAVE_INT32__
#endif
#ifdef __HAVE_INT64__
#undef __HAVE_INT64__
#endif
#ifdef __HAVE_PTR__
#undef __HAVE_PTR__
#endif

#endif /* !__ASSEMBLY__ */

#endif /* __UKARCH_LIMITS_H__ */
