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
//     AES GFMUL and GHASH (GCM mode)
//
//  Contents:
//
*/

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4206) // empty translation unit in MSVC
#endif

#if 0 // Not used

#include "owncp.h"
#include "pcpaesm.h"
#include "pcpaes_encrypt_vaes512.h"
#include "pcpaes_gcm_vaes512.h"
#include "pcpaesauthgcm.h"

#if (_IPP32E>=_IPP32E_K1)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4310) // cast truncates constant value in MSVC
#endif

/* The function calculates multiplication of two 2x128-bit polynomials C = A*B with
   polynomial reduction. 2 polynomials can be processed at one call.
   The inputs are bit-reflected. The result is bit-reflected.
 */
__INLINE void AesGcmGhash2(const __m256i* const src1,
                         const __m256i* const src2,
                         __m256i * const result)
{
   __m256i tmp1, tmp2, tmp3, tmp4;

#if 0
   /* School-book multiplication */
   tmp1 = _mm256_clmulepi64_epi128(src2, src1, 0x00); // [C1:C0]
   tmp2 = _mm256_clmulepi64_epi128(src2, src1, 0x11); // [D1:D0]
   tmp3 = _mm256_clmulepi64_epi128(src2, src1, 0x10); // [F1:F0]
   tmp4 = _mm256_clmulepi64_epi128(src2, src1, 0x01); // [E1:E0]

   tmp3 = _mm256_xor_si256(tmp4, tmp3);

   tmp4 = _mm256_bslli_epi128(tmp3, 8);
   tmp3 = _mm256_bsrli_epi128(tmp3, 8);
   tmp1 = _mm256_xor_si256(tmp4, tmp1);
   tmp4 = _mm256_xor_si256(tmp3, tmp2);
#endif

   /* Karatsuba multiplication */
   tmp1 = _mm256_clmulepi64_epi128(*src2, *src1, 0x00);
   tmp4 = _mm256_clmulepi64_epi128(*src2, *src1, 0x11);

   tmp2 = _mm256_shuffle_epi32(*src2, 78); // 78 = 01001110b
   tmp3 = _mm256_shuffle_epi32(*src1, 78); // 78 = 01001110b
   tmp2 = _mm256_xor_si256(tmp2, *src2);
   tmp3 = _mm256_xor_si256(tmp3, *src1);

   tmp2 = _mm256_clmulepi64_epi128(tmp2, tmp3, 0x00);
   tmp2 = _mm256_xor_si256(tmp2, tmp1);
   tmp2 = _mm256_xor_si256(tmp2, tmp4);

   tmp3 = _mm256_bslli_epi128(tmp2, 8);
   tmp2 = _mm256_bsrli_epi128(tmp2, 8);

   tmp1 = _mm256_xor_si256(tmp1, tmp3);
   tmp4 = _mm256_xor_si256(tmp4, tmp2);

   /* Montgomery reduction */
   tmp2 = _mm256_clmulepi64_epi128(tmp1, M256(POLY2), 0x10);
   tmp3 = _mm256_shuffle_epi32(tmp1, 78); // 78 = 01001110b
   tmp1 = _mm256_xor_si256(tmp2, tmp3);

   tmp2 = _mm256_clmulepi64_epi128(tmp1, M256(POLY2), 0x10);
   tmp3 = _mm256_shuffle_epi32(tmp1, 78); // 78 = 01001110b
   tmp1 = _mm256_xor_si256(tmp2, tmp3);

   *result = _mm256_xor_si256(tmp1, tmp4);
}

/* The function calculates multiplication of two 128-bit polynomials C = A*B with
   polynomial reduction.
   The inputs are bit-reflected. The result is bit-reflected.
 */
__INLINE void AesGcmGhash(const __m128i* const a,
                        const __m128i* const b,
                        __m128i * const result)
{
   __m256i res256;

   __m256i src1 = _mm256_set_m128i(_mm_setzero_si128(), *a);
   __m256i src2 = _mm256_set_m128i(_mm_setzero_si128(), *b);

   AesGcmGhash2(&src1, &src2, &res256);

   *result = _mm256_castsi256_si128(res256);
}

/* The function calculates multiplication of 128-bit polynomials C = A*B with
   polynomial reduction. 4 polynomials can be processed at one call.
   The inputs are bit-reflected. The result is bit-reflected.
 */
__INLINE void AesGcmGhash4(const __m512i* const src1,
                         const __m512i* const src2,
                         __m512i * const result)
{
   __m512i tmp1, tmp2, tmp3, tmp4;

   /* Karatsuba multiplication */
    tmp1 = _mm512_clmulepi64_epi128(*src2, *src1, 0x00);
    tmp4 = _mm512_clmulepi64_epi128(*src2, *src1, 0x11);

    tmp2 = _mm512_shuffle_epi32(*src2, 78); // 78 = 01001110b
    tmp3 = _mm512_shuffle_epi32(*src1, 78); // 78 = 01001110b
    tmp2 = _mm512_xor_si512(tmp2, *src2);
    tmp3 = _mm512_xor_si512(tmp3, *src1);

    tmp2 = _mm512_clmulepi64_epi128(tmp2, tmp3, 0x00);
    tmp2 = _mm512_xor_si512(tmp2, tmp1);
    tmp2 = _mm512_xor_si512(tmp2, tmp4);

    tmp3 = _mm512_bslli_epi128(tmp2, 8);
    tmp2 = _mm512_bsrli_epi128(tmp2, 8);

    tmp1 = _mm512_xor_si512(tmp1, tmp3);
    tmp4 = _mm512_xor_si512(tmp4, tmp2);

    /* Montgomery reduction */
    tmp2 = _mm512_clmulepi64_epi128(tmp1, M512(POLY2), 0x10);
    tmp3 = _mm512_shuffle_epi32(tmp1, 78); // 78 = 01001110b
    tmp1 = _mm512_xor_si512(tmp2, tmp3);

    tmp2 = _mm512_clmulepi64_epi128(tmp1, M512(POLY2), 0x10);
    tmp3 = _mm512_shuffle_epi32(tmp1, 78); // 78 = 01001110b
    tmp1 = _mm512_xor_si512(tmp2, tmp3);

    *result = _mm512_xor_si512(tmp1, tmp4);
}

/* The function performs single A*(Hash<<1 mod poly) multiplication */
IPP_OWN_DEFN (void, AesGcmMulGcm_vaes, (Ipp8u* pGHash, const Ipp8u* pHKey, const void * pParam))
{
   IPP_UNREFERENCED_PARAMETER(pParam);

   __m128i ghash = _mm_maskz_loadu_epi64(0x03, pGHash);
   __m128i hkey  = _mm_maskz_loadu_epi64(0x03, pHKey + 16*3); // NB: hKey is at index 3 in the array

   ghash = _mm_maskz_shuffle_epi8((Ipp16u)(-1), ghash, M128(swapBytes));
   AesGcmGhash(&ghash, &hkey, &ghash);
   ghash = _mm_maskz_shuffle_epi8((Ipp16u)(-1), ghash, M128(swapBytes));

   _mm_mask_storeu_epi64(pGHash, (Ipp8u)(-1), ghash);
}

/* The function computes reflected hKey<<1, hKey^2<<1, hKey^3<<1, ..., hKey^16<<1 - all mod poly
 * to use in batch GHASH calculation.
 * It also pre-computes parts of Karatsuba multipliers that are fixed and derived from the hKey.
 */
IPP_OWN_DEFN (void, AesGcmPrecompute_vaes, (Ipp8u* const pHtbl, const Ipp8u* const hKey))
{
   /* Initial hKey = E_ctr(key, 0^16) */
   __m128i* pDst = (__m128i*)pHtbl;

   __m128i xmm0, xmm1;
   __m256i ymm0, ymm1, ymm3, ymm4, ymm5;
   __m512i zmm0, zmm1, zmm2;

   /* Load initial hKey = E_ctr(key,0^16) */
   ymm0 = _mm256_maskz_loadu_epi64(0x03, hKey);
   /* Reflect hKey */
   ymm0 = _mm256_shuffle_epi8(ymm0, M256(swapBytes));

   /* Compute reflected hKey<<1 mod poly */
   ymm3 = _mm256_srai_epi32(ymm0, 31);
   ymm3 = _mm256_shuffle_epi32(ymm3, 0xFF);
   ymm5 = _mm256_and_si256(ymm3, M256(POLY2));
   ymm3 = _mm256_srli_epi32(ymm0, 31);
   ymm4 = _mm256_slli_epi32(ymm0, 1);
   ymm3 = _mm256_bslli_epi128(ymm3, 4);
   ymm0 = _mm256_xor_si256(ymm4, ymm3);
   ymm0 = _mm256_xor_si256(ymm0,ymm5);

   xmm0 = _mm256_castsi256_si128(ymm0);

   AesGcmGhash(&xmm0, &xmm0, &xmm1);          /* xmm1 = hKey^2                      */

   ymm0 = _mm256_broadcast_i64x2(xmm1);       /* ymm0 = hKey^2 hKey^2               */
   ymm1 = _mm256_set_m128i(xmm0, xmm1);       /* ymm1 = hKey   hKey^2               */
   AesGcmGhash2(&ymm0, &ymm1, &ymm0);         /* ymm0 = hKey^3 hKey^4               */

   xmm1 = _mm256_extracti64x2_epi64(ymm0, 0); /* xmm1 = hKey^4                      */

   zmm0 = _mm512_setzero_si512();
   zmm0 = _mm512_inserti64x4(zmm0, ymm1, 1);  /* zmm0 = hKey   hKey^2 0      0      */
   zmm0 = _mm512_inserti64x4(zmm0, ymm0, 0);  /* zmm0 = hKey   hKey^2 hKey^3 hKey^4 */
   zmm1 = _mm512_broadcast_i64x2(xmm1);       /* zmm1 = hKey^4 hKey^4 hKey^4 hKey^4 */

   /* Store 4xhKey<<1 mod poly degrees in precompute table */
   _mm512_storeu_si512(pDst, zmm0);

   /* Prepare constant multipliers for Karatsuba (Bh^Bl) */
   zmm2 = _mm512_shuffle_epi32(zmm0, 78);
   zmm2 = _mm512_xor_si512(zmm0, zmm2);
   _mm512_storeu_si512(pDst + 16, zmm2);

   for (int i = 1; i < 4; i++)
   {
      AesGcmGhash4(&zmm0, &zmm1, &zmm0);

     /* Store 4xhKey<<1 mod poly degrees in precompute table */
      _mm512_storeu_si512(pDst + 4*i, zmm0);

      /* Prepare constant multipliers for Karatsuba (Bh^Bl) */
      zmm2 = _mm512_shuffle_epi32(zmm0, 78);
      zmm2 = _mm512_xor_si512(zmm0, zmm2);
      _mm512_storeu_si512(pDst + 4*i + 16, zmm2);
   }
}

#endif /* #if (_IPP32E>=_IPP32E_K1) */

#endif
