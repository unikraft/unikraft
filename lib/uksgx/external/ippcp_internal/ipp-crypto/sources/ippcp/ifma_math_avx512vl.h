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

#include "owncp.h"

#ifndef IFMA_MATH_AVX512VL_H
#define IFMA_MATH_AVX512VL_H

/*
 * This header provides low-level abstraction for 256-bit AVX512VL
 * ISA instructions.
 *
 */

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) // for MSVC
    #pragma warning(disable:4101)
    #pragma warning(disable:4127) // warning: conditional expression is constant (see fma52x8lo_mem_len())
#elif defined (__INTEL_COMPILER)
    #pragma warning(disable:177)
#endif

#include <immintrin.h>

#ifndef SIMD_LEN
#define SIMD_LEN 256
#endif

#define SIMD_TYPE(LEN) typedef __m ## LEN ## i U64;

#if (SIMD_LEN == 256)
  SIMD_TYPE(256)
  #define SIMD_BYTES  (SIMD_LEN/8)
  #define SIMD_QWORDS (SIMD_LEN/64)

  __INLINE U64 loadu64(const void *p) {
    return _mm256_loadu_si256((U64*)p);
  }

  __INLINE void storeu64(const void *p, U64 v) {
    _mm256_storeu_si256((U64*)p, v);
  }

  #define set64 _mm256_set1_epi64x

  #ifdef __GNUC__
      static U64 fma52lo(U64 a, U64 b, U64 c)
      {
        __asm__ ( "vpmadd52luq %2, %1, %0" : "+x" (a): "x" (b), "x" (c) );
        return a;
      }

      static U64 fma52hi(U64 a, U64 b, U64 c)
      {
        __asm__ ( "vpmadd52huq %2, %1, %0" : "+x" (a): "x" (b), "x" (c) );
        return a;
      }

      #define _mm_madd52lo_epu64_(r, a, b, c, o) \
      { \
          r=a; \
          __asm__ ( "vpmadd52luq " #o "(%2), %1, %0" : "+x" (r): "x" (b), "r" (c) ); \
      }

      #define _mm_madd52hi_epu64_(r, a, b, c, o) \
      { \
          r=a; \
          __asm__ ( "vpmadd52huq " #o "(%2), %1, %0" : "+x" (r): "x" (b), "r" (c) ); \
      }
  #else
      /* Use IFMA instrinsics for all other compilers */
      static U64 fma52lo(U64 a, U64 b, U64 c)
      {
        return _mm256_madd52lo_epu64(a, b, c);
      }

      static U64 fma52hi(U64 a, U64 b, U64 c)
      {
        return _mm256_madd52hi_epu64(a, b, c);
      }

      #define _mm_madd52lo_epu64_(r, a, b, c, o) \
      { \
          r=fma52lo(a, b, loadu64((U64*)(((char*)c)+o))); \
      }

      #define _mm_madd52hi_epu64_(r, a, b, c, o) \
      { \
          r=fma52hi(a, b, loadu64((U64*)(((char*)c)+o))); \
      }
  #endif

  __INLINE U64 mul52lo(U64 b, U64 c)
  {
    return fma52lo(_mm256_setzero_si256(), b, c);
  }

  #define fma52lo_mem(r, a, b, c, o) _mm_madd52lo_epu64_(r, a, b, c, o)
  #define fma52hi_mem(r, a, b, c, o) _mm_madd52hi_epu64_(r, a, b, c, o)

  __INLINE U64 add64(U64 a, U64 b)
  {
    return _mm256_add_epi64(a, b);
  }

  __INLINE U64 sub64(U64 a, U64 b)
  {
    return _mm256_sub_epi64(a, b);
  }

  __INLINE U64 get_zero64()
  {
    return _mm256_setzero_si256();
  }

  __INLINE void set_zero64(U64 *a)
  {
    *a = _mm256_xor_si256(*a, *a);
  }

  __INLINE U64 set1(unsigned long long a)
  {
    return _mm256_set1_epi64x((long long)a);
  }

  __INLINE U64 srli64(U64 a, int s)
  {
    return _mm256_srli_epi64(a, s);
  }

  #define slli64 _mm256_slli_epi64

  __INLINE U64 and64_const(U64 a, unsigned long long mask)
  {
    return _mm256_and_si256(a, _mm256_set1_epi64x((long long)mask));
  }

  __INLINE U64 and64(U64 a, U64 mask)
  {
    return _mm256_and_si256(a, mask);
  }

  #define or64 _mm256_or_si256
  #define xor64 _mm256_xor_si256

  static Ipp64u get64(U64 v, int idx) {
    long long int res;
    switch (idx) {
      case 1: res = _mm256_extract_epi64(v, 1); break;
      case 2: res = _mm256_extract_epi64(v, 2); break;
      case 3: res = _mm256_extract_epi64(v, 3); break;
      default: res = _mm256_extract_epi64(v, 0);
    }
    return (Ipp64u)res;
  }

  #define fma52x8lo_mem(r, a, b, c, o)             \
      fma52lo_mem(r, a, b, c, o);                  \
      fma52lo_mem(r ## h, a ## h, b, c, (o) + 32);

  #define fma52x8hi_mem(r, a, b, c, o)             \
      fma52hi_mem(r, a, b, c, o);                  \
      fma52hi_mem(r ## h, a ## h, b, c, (o) + 32);

  #define fma52x8lo_mem_len(r, a, b, c, o, l) \
      fma52lo_mem(r, a, b, c, o);             \
      if (l > 4) { fma52lo_mem(r ## h, a ## h, b, c, (o) + 32); }

  #define fma52x8hi_mem_len(r, a, b, c, o, l) \
      fma52hi_mem(r, a, b, c, o);             \
      if (l > 4) { fma52hi_mem(r ## h, a ## h, b, c, (o) + 32); }

  #define fma52x8lo_mask_mem(r, m, a, b, c, o)     \
      fma52lo_mem(r, a, b, c, o);                  \
      fma52lo_mem(r ## h, a ## h, b, c, (o) + 32);

  #define fma52x8hi_mask_mem(r, m, a, b, c, o)     \
      fma52hi_mem(r, a, b, c, o);                  \
      fma52hi_mem(r ## h, a ## h, b, c, (o) + 32);

  #define shift64(R0, R1) { \
      R0 = R0 ## h;         \
      R0 ## h = R1; }

  #define shift64_imm(R0, R1, imm) \
      R0 = _mm256_alignr_epi64(R1, R0, imm);

  #define blend64(a, b, m) \
      _mm256_blend_epi32(a, b, (int)(0x3<<((m-1)<<1)));

#else
  #error "Incorrect SIMD length"
#endif  // SIMD_LEN

#endif  // IFMA_MATH_AVX512VL_H
