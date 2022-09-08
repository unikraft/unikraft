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
//     AES authentication (GCM mode)
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

/* AES-GCM authentication function. It calculates GHASH of the source input */
IPP_OWN_DEFN (void, AesGcmAuth_vaes, (Ipp8u* pGHash, const Ipp8u* pSrc, int len, const Ipp8u* pHKey, const void* pParam))
{
   IPP_UNREFERENCED_PARAMETER(pParam);

   __m512i* pSrc512  = (__m512i*)pSrc;
   __m512i* pHKey512 = (__m512i*)pHKey;

   /* Load hKeys vectors */
   __m512i hKeys0 = _mm512_loadu_si512(pHKey512);
   __m512i hKeys1 = _mm512_loadu_si512(pHKey512 + 1);
   __m512i hKeys2 = _mm512_loadu_si512(pHKey512 + 2);
   __m512i hKeys3 = _mm512_loadu_si512(pHKey512 + 3);

   /* Load precomputed multipliers for Karatsuba multiplication */
   __m512i hKeysKaratsuba0 = _mm512_loadu_si512(pHKey512 + 4);
   __m512i hKeysKaratsuba1 = _mm512_loadu_si512(pHKey512 + 5);
   __m512i hKeysKaratsuba2 = _mm512_loadu_si512(pHKey512 + 6);
   __m512i hKeysKaratsuba3 = _mm512_loadu_si512(pHKey512 + 7);

   /* Current GHASH value */
   __m128i ghash128;
   __m512i ghash512 = _mm512_maskz_loadu_epi64(0x03, pGHash);
   ghash512 = _mm512_shuffle_epi8(ghash512, M512(swapBytes));

   int blocks;
   for (blocks = len / MBS_RIJ128; blocks >= (4 * 4); blocks -= (4 * 4)) {
      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pSrc512 + 3);

      /* Reflect inputs */
      blk0 = _mm512_shuffle_epi8(blk0, M512(swapBytes));
      blk1 = _mm512_shuffle_epi8(blk1, M512(swapBytes));
      blk2 = _mm512_shuffle_epi8(blk2, M512(swapBytes));
      blk3 = _mm512_shuffle_epi8(blk3, M512(swapBytes));

      /* Add current GHASH to src[0] */
      blk0 = _mm512_mask_xor_epi64(blk0, 0x03, blk0, ghash512);

      /* Karatsuba multiplication for 16 blocks with postponed aggregation */
      __m512i H, M, L, tmp1, tmp2, tmp3;
      AesGcmKaratsubaMul4(&blk0, &hKeys3, &hKeysKaratsuba3, &H, &M, &L);
      AesGcmKaratsubaMul4(&blk1, &hKeys2, &hKeysKaratsuba2, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);
      AesGcmKaratsubaMul4(&blk2, &hKeys1, &hKeysKaratsuba1, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);
      AesGcmKaratsubaMul4(&blk3, &hKeys0, &hKeysKaratsuba0, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);

      AggregateKaratsubaPartialProducts(&H, &M, &L, &ghash128);

      /* save current ghash value */
      ghash512 = _mm512_castsi128_si512(ghash128);

      pSrc512 += 4;
   }

   if ((3 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512+1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512+2);

      /* Reflect inputs */
      blk0 = _mm512_shuffle_epi8(blk0, M512(swapBytes));
      blk1 = _mm512_shuffle_epi8(blk1, M512(swapBytes));
      blk2 = _mm512_shuffle_epi8(blk2, M512(swapBytes));

      /* Add current GHASH to src[0] */
      blk0 = _mm512_mask_xor_epi64(blk0, 0x03, blk0, ghash512);

      __m512i H, M, L, tmp1, tmp2, tmp3;
      AesGcmKaratsubaMul4(&blk0, &hKeys2, &hKeysKaratsuba2, &H, &M, &L);
      AesGcmKaratsubaMul4(&blk1, &hKeys1, &hKeysKaratsuba1, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);
      AesGcmKaratsubaMul4(&blk2, &hKeys0, &hKeysKaratsuba0, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);

      AggregateKaratsubaPartialProducts(&H, &M, &L, &ghash128);

      /* save current ghash value */
      ghash512 = _mm512_castsi128_si512(ghash128);

      pSrc512 += 3;
   }

   if ((4 * 2) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512+1);

      /* Reflect inputs */
      blk0 = _mm512_shuffle_epi8(blk0, M512(swapBytes));
      blk1 = _mm512_shuffle_epi8(blk1, M512(swapBytes));

      /* Add current GHASH to src[0] */
      blk0 = _mm512_mask_xor_epi64(blk0, 0x03, blk0, ghash512);

      __m512i H, M, L, tmp1, tmp2, tmp3;
      AesGcmKaratsubaMul4(&blk0, &hKeys1, &hKeysKaratsuba1, &H, &M, &L);
      AesGcmKaratsubaMul4(&blk1, &hKeys0, &hKeysKaratsuba0, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);

      AggregateKaratsubaPartialProducts(&H, &M, &L, &ghash128);

      /* save current ghash value */
      ghash512 = _mm512_castsi128_si512(ghash128);

      pSrc512 += 2;
   }

   for (; blocks >= 4; blocks -= 4) {
      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      blk0 = _mm512_shuffle_epi8(blk0, M512(swapBytes));

      /* Add current GHASH to src[0] */
      blk0 = _mm512_mask_xor_epi64(blk0, 0x03, blk0, ghash512);

      __m512i H, M, L;
      AesGcmKaratsubaMul4(&blk0, &hKeys0, &hKeysKaratsuba0, &H, &M, &L);

      AggregateKaratsubaPartialProducts(&H, &M, &L, &ghash128);

      /* save current ghash value */
      ghash512 = _mm512_castsi128_si512(ghash128);

      pSrc512 += 1;
   }

   // at least one block left (max 3 blocks)
   if (blocks) {
      __mmask8 k8   = (__mmask8)((1 << (blocks + blocks)) - 1);   // 64-bit chunks

      __m512i blk0 = _mm512_maskz_loadu_epi64(k8, pSrc512);
      blk0 = _mm512_shuffle_epi8(blk0, M512(swapBytes));

      /* Add current GHASH to src[0] */
      blk0 = _mm512_mask_xor_epi64(blk0, 0x03, blk0, ghash512);

      __m512i H = _mm512_setzero_si512();
      __m512i M = _mm512_setzero_si512();
      __m512i L = _mm512_setzero_si512();

      // NB: we need immediate parameter in alignr function
      switch (blocks) {
         case 1: {
            hKeys0 = _mm512_alignr_epi64(_mm512_setzero_si512(), hKeys0, 6);
            hKeysKaratsuba0 = _mm512_alignr_epi64(_mm512_setzero_si512(), hKeysKaratsuba0, 6);
            break;
         }
         case 2: {
            hKeys0 = _mm512_alignr_epi64(_mm512_setzero_si512(), hKeys0, 4);
            hKeysKaratsuba0 = _mm512_alignr_epi64(_mm512_setzero_si512(), hKeysKaratsuba0, 4);
            break;
         }
         default: {
            hKeys0 = _mm512_alignr_epi64(_mm512_setzero_si512(), hKeys0, 2);
            hKeysKaratsuba0 = _mm512_alignr_epi64(_mm512_setzero_si512(), hKeysKaratsuba0, 2);
         }
      }
      AesGcmKaratsubaMul4(&blk0, &hKeys0, &hKeysKaratsuba0, &H, &M, &L);

      AggregateKaratsubaPartialProducts(&H, &M, &L, &ghash128);

      /* save current ghash value */
      ghash512 = _mm512_castsi128_si512(ghash128);
   }

   ghash512 = _mm512_shuffle_epi8(ghash512, M512(swapBytes));
   _mm512_mask_storeu_epi64(pGHash, 0x03, ghash512);
}

#endif /* #if (_IPP32E>=_IPP32E_K1) */

#endif
