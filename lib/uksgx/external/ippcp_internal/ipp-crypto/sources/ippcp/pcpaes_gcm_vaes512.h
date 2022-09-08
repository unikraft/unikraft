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

/*
//
//  Purpose:
//     Cryptography Primitive.
//     AES-GCM auxilary
//
//  Contents:
//
*/

#if 0 // Not used

#if !defined(_CP_AES_GCM_VAES512_H)
#define _CP_AES_GCM_VAES512_H

#if (_IPP32E>=_IPP32E_K1)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4310) // cast truncates constant value in MSVC
#endif

#define M128(mem)    (*((__m128i*)((Ipp8u*)(mem))))
#define M256(mem)    (*((__m256i*)((Ipp8u*)(mem))))
#define M512(mem)    (*((__m512i*)((Ipp8u*)(mem))))

static const __ALIGN64 Ipp64u POLY2[] = { 0x1, 0xC200000000000000, 0x1, 0xC200000000000000,
                                          0x1, 0xC200000000000000, 0x1, 0xC200000000000000 };

static __ALIGN64 Ipp8u swapBytes[] = {
   15,14,13,12, 11,10, 9, 8, 7, 6, 5, 4,  3, 2, 1, 0,
   31,30,29,28, 27,26,25,24, 23,22,21,20, 19,18,17,16,
   47,46,45,44, 43,42,41,40, 39,38,37,36, 35,34,33,32,
   63,62,61,60, 59,58,57,56, 55,54,53,52, 51,50,49,48,
};

/* The function performs careless Karatsuba multiplication of 4x128-bit blocks stored
 * in 512-bit inputs:
 *    A = [A3 A2 A1 A0], B = [B3 B2 B1 B0]
 *    Ai=[a1i:a0i] and Bi=[b1i:b0i], i=0..3
 * and returns parts of the multiplication Hi, Mi and Li, i=0..3, where
 *    Hi = a1i*b1i
 *    Mi = (a1i^a0i)*(b1i^b0i)
 *    Li = a0i*b0i
 *
 *  NB: make sure unused parts of input registers are zeroed to avoid issues with further horizontal XOR.
 */
__INLINE void AesGcmKaratsubaMul4(const __m512i * const pA,              /* A3 A2 A1 A0                   */
                                const __m512i * const pHKeys,          /* B3 B2 B1 B0                   */
                                const __m512i * const pHKeysKaratsuba, /* precomputed (b1i^b0i)         */
                                __m512i * const pH,
                                __m512i * const pM,
                                __m512i * const pL)
{
   *pL = _mm512_clmulepi64_epi128(*pA, *pHKeys, 0x00); // L = [a0i*b0i]
   *pH = _mm512_clmulepi64_epi128(*pA, *pHKeys, 0x11); // H = [a1i*b1i]

   *pM = _mm512_shuffle_epi32(*pA, 78); // M = [a0i:a1i]
   *pM = _mm512_xor_epi32(*pM, *pA);    // M = [a1i^a0i:a1i^a0i]
   *pM = _mm512_clmulepi64_epi128(*pM, *pHKeysKaratsuba, 0x00); // M = (a1i^a0i)*(b1i^b0i)
}

/* The function performs horizontal XOR for 4 128-bit values in 512-bit register
   128-bit result value saved in the low part of the 512-bit register
 */
__INLINE void HXor4x128(const __m512i * const zmm,
                      __m128i * const xmm)
{
   __m256i ymm;

   ymm = _mm512_extracti64x4_epi64(*zmm, 0x1); // zmm = [3 2 1 0]; ymm = [3 2]
   ymm = _mm256_xor_si256(_mm512_castsi512_si256(*zmm), ymm); // ymm = [1^3 0^2]

   *xmm = _mm256_extracti32x4_epi32(ymm, 0x1); // xmm = [1^3]
   *xmm = _mm_maskz_xor_epi64(0xFF, _mm256_castsi256_si128(ymm), *xmm);
}

/* The function performs Montgomery reduction of 256-bit polynomial to 128-bit one
   with irreducible polynomial
 */
__INLINE void ReducePoly2x128(const __m128i * const pHI,
                            const __m128i * const pLO,
                            __m128i * const result)
{
   __m256i tmp1, tmp2, HI, LO;

    HI = _mm256_set_m128i(_mm_setzero_si128(), *pHI);
    LO = _mm256_set_m128i(_mm_setzero_si128(), *pLO);

    tmp1 = _mm256_clmulepi64_epi128(LO, M256(POLY2), 0x10);
    tmp2 = _mm256_shuffle_epi32(LO, 78); // 78 = 01001110b
    LO = _mm256_xor_si256(tmp1, tmp2);

    tmp1 = _mm256_clmulepi64_epi128(LO, M256(POLY2), 0x10);
    tmp2 = _mm256_shuffle_epi32(LO, 78); // 78 = 01001110b
    LO = _mm256_xor_si256(tmp1, tmp2);

    tmp1 = _mm256_xor_si256(HI, LO);
    *result = _mm256_castsi256_si128(tmp1);
}

/* The function aggregates partial products of Karatsuba multiplication into final ghash value */
__INLINE void AggregateKaratsubaPartialProducts(const __m512i * const pH,
                                     const __m512i * const pM,
                                     const __m512i * const pL,
                                     __m128i * const result)
{
   __m512i H, M, L;
   __m128i H128,  L128;

   /* Aggregation step1 - combine multiplication results H,M,L into Hi and Lo 128-bit parts */
   M = _mm512_xor_si512(*pM, *pH);
   M = _mm512_xor_si512(M, *pL);
   H = _mm512_bsrli_epi128(M, 8);
   H = _mm512_xor_si512(*pH, H);
   L = _mm512_bslli_epi128(M, 8);
   L = _mm512_xor_si512(*pL, L);

   /* Aggregation step2 - horizontal XOR for H, L*/
   HXor4x128(&H, &H128);
   HXor4x128(&L, &L128);

   /* Reduction of 256-bit poly to 128-bit poly using irreducible polynomial */
   ReducePoly2x128(&H128, &L128, result);
}

#endif /* #if (_IPP32E>=_IPP32E_K1) */

#endif /* _CP_AES_GCM_VAES512_H*/

#endif
