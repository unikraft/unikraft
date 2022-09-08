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

#ifndef IFMA_NORM_52X_H
#define IFMA_NORM_52X_H

#define NORMALIZE_52x20(R0,R1,R2) {                                         \
   __m256i T0   = _mm256_srli_epi64(R0, EXP_DIGIT_SIZE_AVX512);             \
   __m256i T0h  = _mm256_srli_epi64(R0 ## h, EXP_DIGIT_SIZE_AVX512);        \
   __m256i T1   = _mm256_srli_epi64(R1, EXP_DIGIT_SIZE_AVX512);             \
   __m256i T1h  = _mm256_srli_epi64(R1 ## h, EXP_DIGIT_SIZE_AVX512);        \
   __m256i T2   = _mm256_srli_epi64(R2, EXP_DIGIT_SIZE_AVX512);             \
   __m256i MASK = _mm256_set1_epi64x(EXP_DIGIT_MASK_AVX512);                \
                                                                            \
   Ipp32u kk0, kk1, kk2;                                                    \
   Ipp32u kk0h, kk1h;                                                       \
   kk0 = kk1 = kk2 = kk0h = kk1h = 0;                                       \
                                                                            \
   R0 = _mm256_and_si256(R0, MASK);                                         \
   R1 = _mm256_and_si256(R1, MASK);                                         \
   R2 = _mm256_and_si256(R2, MASK);                                         \
   R0 ## h = _mm256_and_si256(R0 ## h, MASK);                               \
   R1 ## h = _mm256_and_si256(R1 ## h, MASK);                               \
                                                                            \
   T2  = _mm256_alignr_epi64(T2, T1h, 3);                                   \
   T1h = _mm256_alignr_epi64(T1h, T1, 3);                                   \
   T1  = _mm256_alignr_epi64(T1, T0h, 3);                                   \
   T0h = _mm256_alignr_epi64(T0h, T0, 3);                                   \
   T0  = _mm256_alignr_epi64(T0, _mm256_setzero_si256(), 3);                \
                                                                            \
   R0 = _mm256_add_epi64(R0, T0);                                           \
   R1 = _mm256_add_epi64(R1, T1);                                           \
   R2 = _mm256_add_epi64(R2, T2);                                           \
   R0 ## h = _mm256_add_epi64(R0 ## h, T0h);                                \
   R1 ## h = _mm256_add_epi64(R1 ## h, T1h);                                \
   {                                                                        \
      Ipp32u k,l;                                                           \
      k = l = 0;                                                            \
                                                                            \
      kk0 = _mm256_cmp_epu64_mask(MASK, R0, _MM_CMPINT_LT);                 \
      kk1 = _mm256_cmp_epu64_mask(MASK, R1, _MM_CMPINT_LT);                 \
      kk2 = _mm256_cmp_epu64_mask(MASK, R2, _MM_CMPINT_LT);                 \
      kk0h = _mm256_cmp_epu64_mask(MASK, R0 ## h, _MM_CMPINT_LT);           \
      kk1h = _mm256_cmp_epu64_mask(MASK, R1 ## h, _MM_CMPINT_LT);           \
                                                                            \
      k = (kk2<<16)|(kk1h<<12)|(kk1<<8)|(kk0h<<4)|kk0;                      \
                                                                            \
      kk0  = _mm256_cmp_epu64_mask(MASK, R0, _MM_CMPINT_EQ);                \
      kk1  = _mm256_cmp_epu64_mask(MASK, R1, _MM_CMPINT_EQ);                \
      kk2  = _mm256_cmp_epu64_mask(MASK, R2, _MM_CMPINT_EQ);                \
      kk0h = _mm256_cmp_epu64_mask(MASK, R0 ## h, _MM_CMPINT_EQ);           \
      kk1h = _mm256_cmp_epu64_mask(MASK, R1 ## h, _MM_CMPINT_EQ);           \
                                                                            \
      l = (kk2<<16)|(kk1h<<12)|(kk1<<8)|(kk0h<<4)|kk0;                      \
                                                                            \
      k = l + 2*k;                                                          \
      k ^= l;                                                               \
                                                                            \
      kk0  = k;                                                             \
      kk0h = (k>>4);                                                        \
      kk1  = (k>>8);                                                        \
      kk1h = (k>>12);                                                       \
      kk2  = (k>>16);                                                       \
   }                                                                        \
                                                                            \
   R0 = _mm256_mask_sub_epi64(R0, (__mmask8)kk0, R0, MASK);                 \
   R1 = _mm256_mask_sub_epi64(R1, (__mmask8)kk1, R1, MASK);                 \
   R2 = _mm256_mask_sub_epi64(R2, (__mmask8)kk2, R2, MASK);                 \
   R0 ## h = _mm256_mask_sub_epi64(R0 ## h, (__mmask8)kk0h, R0 ## h, MASK); \
   R1 ## h = _mm256_mask_sub_epi64(R1 ## h, (__mmask8)kk1h, R1 ## h, MASK); \
                                                                            \
   R0      = _mm256_and_si256(R0,      MASK);                               \
   R1      = _mm256_and_si256(R1,      MASK);                               \
   R2      = _mm256_and_si256(R2,      MASK);                               \
   R0 ## h = _mm256_and_si256(R0 ## h, MASK);                               \
   R1 ## h = _mm256_and_si256(R1 ## h, MASK);                               \
}

#define NORMALIZE_52x30(R0,R1,R2,R3) {                                                 \
   __m256i T0   = _mm256_srli_epi64(R0, EXP_DIGIT_SIZE_AVX512);                        \
   __m256i T0h  = _mm256_srli_epi64(R0 ## h, EXP_DIGIT_SIZE_AVX512);                   \
   __m256i T1   = _mm256_srli_epi64(R1, EXP_DIGIT_SIZE_AVX512);                        \
   __m256i T1h  = _mm256_srli_epi64(R1 ## h, EXP_DIGIT_SIZE_AVX512);                   \
   __m256i T2   = _mm256_srli_epi64(R2, EXP_DIGIT_SIZE_AVX512);                        \
   __m256i T2h  = _mm256_srli_epi64(R2 ## h, EXP_DIGIT_SIZE_AVX512);                   \
   __m256i T3   = _mm256_srli_epi64(R3, EXP_DIGIT_SIZE_AVX512);                        \
   __m256i T3h  = _mm256_srli_epi64(R3 ## h, EXP_DIGIT_SIZE_AVX512);                   \
   __m256i MASK = _mm256_set1_epi64x(EXP_DIGIT_MASK_AVX512);                           \
                                                                                       \
   Ipp32u kk0, kk1, kk2, kk3;                                                          \
   Ipp32u kk0h, kk1h, kk2h, kk3h;                                                      \
   kk0 = kk1 = kk2 = kk3 = kk0h = kk1h = kk2h = kk3h = 0;                              \
                                                                                       \
   R0 = _mm256_and_si256(R0, MASK);                                                    \
   R1 = _mm256_and_si256(R1, MASK);                                                    \
   R2 = _mm256_and_si256(R2, MASK);                                                    \
   R3 = _mm256_and_si256(R3, MASK);                                                    \
   R0 ## h = _mm256_and_si256(R0 ## h, MASK);                                          \
   R1 ## h = _mm256_and_si256(R1 ## h, MASK);                                          \
   R2 ## h = _mm256_and_si256(R2 ## h, MASK);                                          \
   R3 ## h = _mm256_and_si256(R3 ## h, MASK);                                          \
                                                                                       \
   T3h = _mm256_alignr_epi64(T3h, T3, 3);                                              \
   T3  = _mm256_alignr_epi64(T3 , T2h, 3);                                             \
   T2h = _mm256_alignr_epi64(T2h, T2, 3);                                              \
   T2  = _mm256_alignr_epi64(T2 , T1h, 3);                                             \
   T1h = _mm256_alignr_epi64(T1h, T1, 3);                                              \
   T1  = _mm256_alignr_epi64(T1 , T0h, 3);                                             \
   T0h = _mm256_alignr_epi64(T0h, T0, 3);                                              \
   T0  = _mm256_alignr_epi64(T0, _mm256_setzero_si256(), 3);                           \
                                                                                       \
   R0 = _mm256_add_epi64(R0, T0);                                                      \
   R1 = _mm256_add_epi64(R1, T1);                                                      \
   R2 = _mm256_add_epi64(R2, T2);                                                      \
   R3 = _mm256_add_epi64(R3, T3);                                                      \
   R0 ## h = _mm256_add_epi64(R0 ## h, T0h);                                           \
   R1 ## h = _mm256_add_epi64(R1 ## h, T1h);                                           \
   R2 ## h = _mm256_add_epi64(R2 ## h, T2h);                                           \
   R3 ## h = _mm256_add_epi64(R3 ## h, T3h);                                           \
                                                                                       \
   {                                                                                   \
      Ipp32u k, l;                                                                     \
      k = l = 0;                                                                       \
                                                                                       \
      kk0 = _mm256_cmp_epu64_mask(MASK, R0, _MM_CMPINT_LT);                            \
      kk1 = _mm256_cmp_epu64_mask(MASK, R1, _MM_CMPINT_LT);                            \
      kk2 = _mm256_cmp_epu64_mask(MASK, R2, _MM_CMPINT_LT);                            \
      kk3 = _mm256_cmp_epu64_mask(MASK, R3, _MM_CMPINT_LT);                            \
      kk0h = _mm256_cmp_epu64_mask(MASK, R0 ## h, _MM_CMPINT_LT);                      \
      kk1h = _mm256_cmp_epu64_mask(MASK, R1 ## h, _MM_CMPINT_LT);                      \
      kk2h = _mm256_cmp_epu64_mask(MASK, R2 ## h, _MM_CMPINT_LT);                      \
      kk3h = _mm256_cmp_epu64_mask(MASK, R3 ## h, _MM_CMPINT_LT);                      \
                                                                                       \
      k = (kk3h<<28)|(kk3<<24)|(kk2h<<20)|(kk2<<16)|(kk1h<<12)|(kk1<<8)|(kk0h<<4)|kk0; \
                                                                                       \
      kk0  = _mm256_cmp_epu64_mask(MASK, R0, _MM_CMPINT_EQ);                           \
      kk1  = _mm256_cmp_epu64_mask(MASK, R1, _MM_CMPINT_EQ);                           \
      kk2  = _mm256_cmp_epu64_mask(MASK, R2, _MM_CMPINT_EQ);                           \
      kk3  = _mm256_cmp_epu64_mask(MASK, R3, _MM_CMPINT_EQ);                           \
      kk0h = _mm256_cmp_epu64_mask(MASK, R0 ## h, _MM_CMPINT_EQ);                      \
      kk1h = _mm256_cmp_epu64_mask(MASK, R1 ## h, _MM_CMPINT_EQ);                      \
      kk2h = _mm256_cmp_epu64_mask(MASK, R2 ## h, _MM_CMPINT_EQ);                      \
      kk3h = _mm256_cmp_epu64_mask(MASK, R3 ## h, _MM_CMPINT_EQ);                      \
                                                                                       \
      l = (kk3h<<28)|(kk3<<24)|(kk2h<<20)|(kk2<<16)|(kk1h<<12)|(kk1<<8)|(kk0h<<4)|kk0; \
                                                                                       \
      k = l + 2*k;                                                                     \
      k ^= l;                                                                          \
                                                                                       \
      kk0  = k;                                                                        \
      kk0h = (k>>4);                                                                   \
      kk1  = (k>>8);                                                                   \
      kk1h = (k>>12);                                                                  \
      kk2  = (k>>16);                                                                  \
      kk2h = (k>>20);                                                                  \
      kk3  = (k>>24);                                                                  \
      kk3h = (k>>28);                                                                  \
   }                                                                                   \
                                                                                       \
   R0 = _mm256_mask_sub_epi64(R0, (__mmask8)kk0, R0, MASK);                            \
   R1 = _mm256_mask_sub_epi64(R1, (__mmask8)kk1, R1, MASK);                            \
   R2 = _mm256_mask_sub_epi64(R2, (__mmask8)kk2, R2, MASK);                            \
   R3 = _mm256_mask_sub_epi64(R3, (__mmask8)kk3, R3, MASK);                            \
   R0 ## h = _mm256_mask_sub_epi64(R0 ## h, (__mmask8)kk0h, R0 ## h, MASK);            \
   R1 ## h = _mm256_mask_sub_epi64(R1 ## h, (__mmask8)kk1h, R1 ## h, MASK);            \
   R2 ## h = _mm256_mask_sub_epi64(R2 ## h, (__mmask8)kk2h, R2 ## h, MASK);            \
   R3 ## h = _mm256_mask_sub_epi64(R3 ## h, (__mmask8)kk3h, R3 ## h, MASK);            \
                                                                                       \
   R0      = _mm256_and_si256(R0,      MASK);                                          \
   R1      = _mm256_and_si256(R1,      MASK);                                          \
   R2      = _mm256_and_si256(R2,      MASK);                                          \
   R3      = _mm256_and_si256(R3,      MASK);                                          \
   R0 ## h = _mm256_and_si256(R0 ## h, MASK);                                          \
   R1 ## h = _mm256_and_si256(R1 ## h, MASK);                                          \
   R2 ## h = _mm256_and_si256(R2 ## h, MASK);                                          \
   R3 ## h = _mm256_and_si256(R3 ## h, MASK);                                          \
}

#define NORMALIZE_52x40(R0,R1,R2,R3,R4) {                                   \
   __m256i T0   = _mm256_srli_epi64(R0, EXP_DIGIT_SIZE_AVX512);             \
   __m256i T0h  = _mm256_srli_epi64(R0 ## h, EXP_DIGIT_SIZE_AVX512);        \
   __m256i T1   = _mm256_srli_epi64(R1, EXP_DIGIT_SIZE_AVX512);             \
   __m256i T1h  = _mm256_srli_epi64(R1 ## h, EXP_DIGIT_SIZE_AVX512);        \
   __m256i T2   = _mm256_srli_epi64(R2, EXP_DIGIT_SIZE_AVX512);             \
   __m256i T2h  = _mm256_srli_epi64(R2 ## h, EXP_DIGIT_SIZE_AVX512);        \
   __m256i T3   = _mm256_srli_epi64(R3, EXP_DIGIT_SIZE_AVX512);             \
   __m256i T3h  = _mm256_srli_epi64(R3 ## h, EXP_DIGIT_SIZE_AVX512);        \
   __m256i T4   = _mm256_srli_epi64(R4, EXP_DIGIT_SIZE_AVX512);             \
   __m256i T4h  = _mm256_srli_epi64(R4 ## h, EXP_DIGIT_SIZE_AVX512);        \
   __m256i MASK = _mm256_set1_epi64x(EXP_DIGIT_MASK_AVX512);                \
                                                                            \
   Ipp64u kk0, kk1, kk2, kk3, kk4;                                          \
   Ipp64u kk0h, kk1h, kk2h, kk3h, kk4h;                                     \
   kk0 = kk1 = kk2 = kk3 = kk4 = kk0h = kk1h = kk2h = kk3h = kk4h = 0;      \
                                                                            \
   R0 = _mm256_and_si256(R0, MASK);                                         \
   R1 = _mm256_and_si256(R1, MASK);                                         \
   R2 = _mm256_and_si256(R2, MASK);                                         \
   R3 = _mm256_and_si256(R3, MASK);                                         \
   R4 = _mm256_and_si256(R4, MASK);                                         \
   R0 ## h = _mm256_and_si256(R0 ## h, MASK);                               \
   R1 ## h = _mm256_and_si256(R1 ## h, MASK);                               \
   R2 ## h = _mm256_and_si256(R2 ## h, MASK);                               \
   R3 ## h = _mm256_and_si256(R3 ## h, MASK);                               \
   R4 ## h = _mm256_and_si256(R4 ## h, MASK);                               \
                                                                            \
   T4h = _mm256_alignr_epi64(T4h, T4, 3);                                   \
   T4  = _mm256_alignr_epi64(T4 , T3h, 3);                                  \
   T3h = _mm256_alignr_epi64(T3h, T3, 3);                                   \
   T3  = _mm256_alignr_epi64(T3 , T2h, 3);                                  \
   T2h = _mm256_alignr_epi64(T2h, T2, 3);                                   \
   T2  = _mm256_alignr_epi64(T2 , T1h, 3);                                  \
   T1h = _mm256_alignr_epi64(T1h, T1, 3);                                   \
   T1  = _mm256_alignr_epi64(T1 , T0h, 3);                                  \
   T0h = _mm256_alignr_epi64(T0h, T0, 3);                                   \
   T0  = _mm256_alignr_epi64(T0, _mm256_setzero_si256(), 3);                \
                                                                            \
   R0 = _mm256_add_epi64(R0, T0);                                           \
   R1 = _mm256_add_epi64(R1, T1);                                           \
   R2 = _mm256_add_epi64(R2, T2);                                           \
   R3 = _mm256_add_epi64(R3, T3);                                           \
   R4 = _mm256_add_epi64(R4, T4);                                           \
   R0 ## h = _mm256_add_epi64(R0 ## h, T0h);                                \
   R1 ## h = _mm256_add_epi64(R1 ## h, T1h);                                \
   R2 ## h = _mm256_add_epi64(R2 ## h, T2h);                                \
   R3 ## h = _mm256_add_epi64(R3 ## h, T3h);                                \
   R4 ## h = _mm256_add_epi64(R4 ## h, T4h);                                \
                                                                            \
   {                                                                        \
      Ipp64u k, l;                                                          \
      k = l = 0;                                                            \
                                                                            \
      kk0 = _mm256_cmp_epu64_mask(MASK, R0, _MM_CMPINT_LT);                 \
      kk1 = _mm256_cmp_epu64_mask(MASK, R1, _MM_CMPINT_LT);                 \
      kk2 = _mm256_cmp_epu64_mask(MASK, R2, _MM_CMPINT_LT);                 \
      kk3 = _mm256_cmp_epu64_mask(MASK, R3, _MM_CMPINT_LT);                 \
      kk4 = _mm256_cmp_epu64_mask(MASK, R4, _MM_CMPINT_LT);                 \
      kk0h = _mm256_cmp_epu64_mask(MASK, R0 ## h, _MM_CMPINT_LT);           \
      kk1h = _mm256_cmp_epu64_mask(MASK, R1 ## h, _MM_CMPINT_LT);           \
      kk2h = _mm256_cmp_epu64_mask(MASK, R2 ## h, _MM_CMPINT_LT);           \
      kk3h = _mm256_cmp_epu64_mask(MASK, R3 ## h, _MM_CMPINT_LT);           \
      kk4h = _mm256_cmp_epu64_mask(MASK, R4 ## h, _MM_CMPINT_LT);           \
                                                                            \
      k = (kk4h<<36)|(kk4<<32)|(kk3h<<28)|(kk3<<24)|(kk2h<<20)|             \
           (kk2<<16)|(kk1h<<12)|(kk1<<8)|(kk0h<<4)|kk0;                     \
                                                                            \
      kk0  = _mm256_cmp_epu64_mask(MASK, R0, _MM_CMPINT_EQ);                \
      kk1  = _mm256_cmp_epu64_mask(MASK, R1, _MM_CMPINT_EQ);                \
      kk2  = _mm256_cmp_epu64_mask(MASK, R2, _MM_CMPINT_EQ);                \
      kk3  = _mm256_cmp_epu64_mask(MASK, R3, _MM_CMPINT_EQ);                \
      kk4  = _mm256_cmp_epu64_mask(MASK, R4, _MM_CMPINT_EQ);                \
      kk0h = _mm256_cmp_epu64_mask(MASK, R0 ## h, _MM_CMPINT_EQ);           \
      kk1h = _mm256_cmp_epu64_mask(MASK, R1 ## h, _MM_CMPINT_EQ);           \
      kk2h = _mm256_cmp_epu64_mask(MASK, R2 ## h, _MM_CMPINT_EQ);           \
      kk3h = _mm256_cmp_epu64_mask(MASK, R3 ## h, _MM_CMPINT_EQ);           \
      kk4h = _mm256_cmp_epu64_mask(MASK, R4 ## h, _MM_CMPINT_EQ);           \
                                                                            \
      l = (kk4h<<36)|(kk4<<32)|(kk3h<<28)|(kk3<<24)|(kk2h<<20)|             \
           (kk2<<16)|(kk1h<<12)|(kk1<<8)|(kk0h<<4)|kk0;                     \
                                                                            \
      k = l + 2*k;                                                          \
      k ^= l;                                                               \
                                                                            \
      kk0  = k;                                                             \
      kk0h = (k>>4);                                                        \
      kk1  = (k>>8);                                                        \
      kk1h = (k>>12);                                                       \
      kk2  = (k>>16);                                                       \
      kk2h = (k>>20);                                                       \
      kk3  = (k>>24);                                                       \
      kk3h = (k>>28);                                                       \
      kk4  = (k>>32);                                                       \
      kk4h = (k>>36);                                                       \
   }                                                                        \
                                                                            \
   R0 = _mm256_mask_sub_epi64(R0, (__mmask8)kk0, R0, MASK);                 \
   R1 = _mm256_mask_sub_epi64(R1, (__mmask8)kk1, R1, MASK);                 \
   R2 = _mm256_mask_sub_epi64(R2, (__mmask8)kk2, R2, MASK);                 \
   R3 = _mm256_mask_sub_epi64(R3, (__mmask8)kk3, R3, MASK);                 \
   R4 = _mm256_mask_sub_epi64(R4, (__mmask8)kk4, R4, MASK);                 \
   R0 ## h = _mm256_mask_sub_epi64(R0 ## h, (__mmask8)kk0h, R0 ## h, MASK); \
   R1 ## h = _mm256_mask_sub_epi64(R1 ## h, (__mmask8)kk1h, R1 ## h, MASK); \
   R2 ## h = _mm256_mask_sub_epi64(R2 ## h, (__mmask8)kk2h, R2 ## h, MASK); \
   R3 ## h = _mm256_mask_sub_epi64(R3 ## h, (__mmask8)kk3h, R3 ## h, MASK); \
   R4 ## h = _mm256_mask_sub_epi64(R4 ## h, (__mmask8)kk4h, R4 ## h, MASK); \
                                                                            \
   R0      = _mm256_and_si256(R0,      MASK);                               \
   R1      = _mm256_and_si256(R1,      MASK);                               \
   R2      = _mm256_and_si256(R2,      MASK);                               \
   R3      = _mm256_and_si256(R3,      MASK);                               \
   R4      = _mm256_and_si256(R4,      MASK);                               \
   R0 ## h = _mm256_and_si256(R0 ## h, MASK);                               \
   R1 ## h = _mm256_and_si256(R1 ## h, MASK);                               \
   R2 ## h = _mm256_and_si256(R2 ## h, MASK);                               \
   R3 ## h = _mm256_and_si256(R3 ## h, MASK);                               \
   R4 ## h = _mm256_and_si256(R4 ## h, MASK);                               \
}

#endif  // IFMA_NORM_52X_H
