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

#ifndef __UKARCH_TYPES_H__
#define __UKARCH_TYPES_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <uk/asm/intsizes.h>
#include <uk/asm/types.h>

#ifndef __ASSEMBLY__

#if (defined __C_IS_8)
typedef signed   char      __s8;
typedef unsigned char      __u8;
#define __PRIs8 "d"
#define __PRIu8 "u"
#define __PRIx8 "x"
#define __SCNs8 "hhd"
#define __SCNu8 "hhu"
#define __SCNx8 "hhx"
#define __HAVE_INT8__
#undef __C_IS_8
#endif

#if (defined __S_IS_16)
typedef signed   short     __s16;
typedef unsigned short     __u16;
#define __PRIs16 "d"
#define __PRIu16 "u"
#define __PRIx16 "x"
#define __SCNs16 "hd"
#define __SCNu16 "hu"
#define __SCNx16 "hx"
#define __HAVE_INT16__
#undef __S_IS_16
#endif

#if (defined __I_IS_16)
#ifndef __HAVE_INT16__
typedef signed   int     __s16;
typedef unsigned int     __u16;
#define __PRIs16 "d"
#define __PRIu16 "u"
#define __PRIx16 "x"
#define __SCNs16 "d"
#define __SCNu16 "u"
#define __SCNx16 "x"
#define __HAVE_INT16__
#endif
#undef __I_IS_16
#elif (defined __I_IS_32)
#ifndef __HAVE_INT32__
typedef signed   int     __s32;
typedef unsigned int     __u32;
#define __PRIs32 "d"
#define __PRIu32 "u"
#define __PRIx32 "x"
#define __SCNs32 "d"
#define __SCNu32 "u"
#define __SCNx32 "x"
#define __HAVE_INT32__
#endif
#undef __I_IS_32
#elif (defined __I_IS_64)
#ifndef __HAVE_INT64__
typedef signed   int     __s64;
typedef unsigned int     __u64;
#define __PRIs64 "d"
#define __PRIu64 "u"
#define __PRIx64 "x"
#define __SCNs64 "d"
#define __SCNu64 "u"
#define __SCNx64 "x"
#define __HAVE_INT64__
#endif
#undef __I_IS_64
#endif

#if (defined __L_IS_32)
#ifndef __HAVE_INT32__
typedef signed   long      __s32;
typedef unsigned long      __u32;
#define __PRIs32 "ld"
#define __PRIu32 "lu"
#define __PRIx32 "lx"
#define __SCNs32 "ld"
#define __SCNu32 "lu"
#define __SCNx32 "lx"
#define __HAVE_INT32__
#endif
#undef __L_IS_32
#elif (defined __L_IS_64)
#ifndef __HAVE_INT64__
typedef signed   long      __s64;
typedef unsigned long      __u64;
#define __PRIs64 "ld"
#define __PRIu64 "lu"
#define __PRIx64 "lx"
#define __SCNs64 "ld"
#define __SCNu64 "lu"
#define __SCNx64 "lx"
#define __HAVE_INT64__
#endif
#undef __L_IS_64
#endif

#if (defined __LL_IS_32)
#ifndef __HAVE_INT32__
typedef signed   long long __s32;
typedef unsigned long long __u32;
#define __PRIs32 "lld"
#define __PRIu32 "llu"
#define __PRIx32 "llx"
#define __SCNs32 "lld"
#define __SCNu32 "llu"
#define __SCNx32 "llx"
#define __HAVE_INT32__
#endif
#undef __LL_IS_32
#elif (defined __LL_IS_64)
#ifndef __HAVE_INT64__
typedef signed   long long __s64;
typedef unsigned long long __u64;
#define __PRIs64 "lld"
#define __PRIu64 "llu"
#define __PRIx64 "llx"
#define __SCNs64 "lld"
#define __SCNu64 "llu"
#define __SCNx64 "llx"
#define __HAVE_INT64__
#endif
#undef __LL_IS_64
#endif

#if (defined __PTR_IS_16)
typedef __s16 __sptr;
typedef __u16 __uptr;
#define __PRIuptr __PRIx16
#define __PRIsz   __PRIu16
#define __PRIssz  __PRIs16
#define __PRIoff  __PRIs16
#define __HAVE_PTR__
#elif (defined __PTR_IS_32)
typedef __s32 __sptr;
typedef __u32 __uptr;
#define __PRIuptr __PRIx32
#define __PRIsz   __PRIu32
#define __PRIssz  __PRIs32
#define __PRIoff  __PRIs32
#define __HAVE_PTR__
#elif (defined __PTR_IS_64)
typedef __s64 __sptr;
typedef __u64 __uptr;
#define __PRIuptr __PRIx64
#define __PRIsz   __PRIu64
#define __PRIssz  __PRIs64
#define __PRIoff  __PRIs64
#define __HAVE_PTR__
#endif
typedef __uptr __sz;  /* size_t  equivalent */
typedef __sptr __ssz; /* ssize_t equivalent */
typedef __sptr __off; /* off_t equivalent */

#if (defined __PHY_ADDR_IS_16)
typedef __u16 __vm_offset;
typedef __u16 __phys_addr;
#define __PRIpaddr __PRIx16
#define __HAVE_PHYS_ADDR__
#elif (defined __PHY_ADDR_IS_32)
typedef __u32 __vm_offset;
typedef __u32 __phys_addr;
#define __PRIpaddr __PRIx32
#define __HAVE_PHYS_ADDR__
#elif (defined __PHY_ADDR_IS_64)
typedef __u64 __vm_offset;
typedef __u64 __phys_addr;
#define __PRIpaddr __PRIx64
#define __HAVE_PHYS_ADDR__
#endif

/* Sanity check */
#ifndef __HAVE_INT8__
#error Missing 8-bit integer definitions
#else
#undef __HAVE_INT8__
#endif
#ifndef __HAVE_INT16__
#error Missing 16-bit integer definitions
#else
#undef __HAVE_INT16__
#endif
#ifndef __HAVE_INT32__
#error Missing 32-bit integer definitions
#else
#undef __HAVE_INT32__
#endif
#ifndef __HAVE_INT64__
#error Missing 64-bit integer definitions
#else
#undef __HAVE_INT64__
#endif
#ifndef __HAVE_PTR__
#error Missing pointer integer definitions
#else
#undef __HAVE_PTR__
#endif
#ifndef __HAVE_PHYS_ADDR__
#error Missing physical address definitions
#else
#undef __HAVE_PHYS_ADDR__
#endif

#ifndef __NULL
#define __NULL ((void *) 0)
#endif

typedef struct {
	__u32 counter;
} __atomic;

#endif /* !__ASSEMBLY__ */

#ifdef	__cplusplus
}
#endif

#endif /* __UKARCH_TYPES_H__ */
