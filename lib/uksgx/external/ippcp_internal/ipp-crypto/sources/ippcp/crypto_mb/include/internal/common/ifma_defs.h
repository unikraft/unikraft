/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef IFMA_DEFS_H
#define IFMA_DEFS_H

#include <crypto_mb/defs.h>

/* define DLL_EXPORT */
#if defined(__GNUC__) || defined(__CLANG__)
   #define DLL_PUBLIC __attribute__ ((visibility ("default")))
   #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
   #define DLL_PUBLIC
   #define DLL_LOCAL
#endif

/* define SIMD_LEN if not set (Default is 512 bit AVX) */
#ifndef SIMD_LEN
  #define SIMD_LEN 512
#endif

#if ((SIMD_LEN != 512) && (SIMD_LEN != 256))
  #error "Incorrect SIMD length"
#endif

/* internal function names */
#if (SIMD_LEN == 512)
    #define FUNC_SUFFIX mb8
    #define MB_FUNC_NAME(name) name ## mb8
#else
    #define FUNC_SUFFIX mb4
    #define MB_FUNC_NAME(name) name ## mb4
#endif

#define SIMD_TYPE(LEN) typedef __m ## LEN ## i U64;

/* max internal data bitsize */
#define IFMA_MAX_BITSIZE   (4096)

/* internal radix definition */
#define DIGIT_SIZE (52)
#define DIGIT_BASE ((int64u)1<<DIGIT_SIZE)
#define DIGIT_MASK ((int64u)0xFFFFFFFFFFFFF)

/* num of digit in "digsize" retresentation of "bitsize" value */
#define NUMBER_OF_DIGITS(bitsize, digsize)   (((bitsize) + (digsize)-1)/(digsize))
/* mask of most significant digit wrt "digsize" retresentation */
#define MS_DIGIT_MASK(bitsize, digsize)      (((int64u)1 <<((bitsize) %digsize)) -1)

/* pointer alignment */
#define IFMA_UINT_PTR( ptr ) ( (int64u)(ptr) )
#define IFMA_BYTES_TO_ALIGN(ptr, align) ((~(IFMA_UINT_PTR(ptr)&((align)-1))+1)&((align)-1))
#define IFMA_ALIGNED_PTR(ptr, align) (void*)( (unsigned char*)(ptr) + (IFMA_BYTES_TO_ALIGN( ptr, align )) )

/* repetitions */
#define  REP2_DECL(a)   a, a
#define  REP4_DECL(a)   REP2_DECL(a), REP2_DECL(a)
#define  REP8_DECL(a)   REP4_DECL(a), REP4_DECL(a)

#endif /* IFMA_DEFS_H */
