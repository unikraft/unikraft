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
//     AES decryption (GCM mode)
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

static __ALIGN64 Ipp32u inc_lo32x4[]  = { 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0 };
static __ALIGN64 Ipp32u inc1_lo32x4[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
static __ALIGN64 Ipp32u inc4_lo32x4[] = { 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0 };

IPP_OWN_DEFN (void, AesGcmDec_vaes, (Ipp8u* pDst, const Ipp8u* pSrc, int length, IppsAES_GCMState* pCtx))
{
   IppsAESSpec* pAES  = AESGCM_CIPHER(pCtx);
   int cipherRounds   = RIJ_NR(pAES) - 1;
   const Ipp8u* pRKey = RIJ_EKEYS(pAES);

   Ipp8u* pCtrValue   = AESGCM_COUNTER(pCtx);
   Ipp8u* pGHash      = AESGCM_GHASH(pCtx);
   const Ipp8u* pHKey = AESGCM_HKEY(pCtx);

   __m128i* pKeys    = (__m128i*)pRKey;
   __m512i* pSrc512  = (__m512i*)pSrc;
   __m512i* pDst512  = (__m512i*)pDst;
   __m512i* pHKey512 = (__m512i*)pHKey;

   __m512i incMsk = M512(inc_lo32x4);

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

   /* Load initial counter */
   __m128i ctr128 = _mm_maskz_loadu_epi64(0x03, pCtrValue);
   __m512i ctr512 = _mm512_broadcast_i64x2(ctr128);

   // convert counter to little-endian
   ctr512 = _mm512_shuffle_epi8(ctr512, M512(swapBytes));

   int blocks;
   for (blocks = length / MBS_RIJ128; blocks >= (4 * 4); blocks -= (4 * 4)) {
      __m512i counter0 = _mm512_add_epi32(incMsk, ctr512);
      __m512i counter1 = _mm512_add_epi32(M512(inc4_lo32x4), counter0);
      __m512i counter2 = _mm512_add_epi32(M512(inc4_lo32x4), counter1);
      __m512i counter3 = _mm512_add_epi32(M512(inc4_lo32x4), counter2);

      incMsk = M512(inc4_lo32x4);
      ctr512 = counter3;

      // convert back to big-endian
      counter0 = _mm512_shuffle_epi8(counter0, M512(swapBytes));
      counter1 = _mm512_shuffle_epi8(counter1, M512(swapBytes));
      counter2 = _mm512_shuffle_epi8(counter2, M512(swapBytes));
      counter3 = _mm512_shuffle_epi8(counter3, M512(swapBytes));

      cpAESEncrypt4_VAES_NI(&counter0, &counter1, &counter2, &counter3, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pSrc512 + 3);

      _mm512_storeu_si512(pDst512, _mm512_xor_si512(blk0, counter0));
      _mm512_storeu_si512(pDst512 + 1, _mm512_xor_si512(blk1, counter1));
      _mm512_storeu_si512(pDst512 + 2, _mm512_xor_si512(blk2, counter2));
      _mm512_storeu_si512(pDst512 + 3, _mm512_xor_si512(blk3, counter3));

      /* Authenticate */
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
      pDst512 += 4;
   }

   if ((3 * 4) <= blocks) {
      __m512i counter0 = _mm512_add_epi32(incMsk, ctr512);
      __m512i counter1 = _mm512_add_epi32(M512(inc4_lo32x4), counter0);
      __m512i counter2 = _mm512_add_epi32(M512(inc4_lo32x4), counter1);

      incMsk = M512(inc4_lo32x4);
      ctr512 = counter2;

      // convert back to big-endian
      counter0 = _mm512_shuffle_epi8(counter0, M512(swapBytes));
      counter1 = _mm512_shuffle_epi8(counter1, M512(swapBytes));
      counter2 = _mm512_shuffle_epi8(counter2, M512(swapBytes));

      cpAESEncrypt3_VAES_NI(&counter0, &counter1, &counter2, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512 + 2);

      _mm512_storeu_si512(pDst512, _mm512_xor_si512(blk0, counter0));
      _mm512_storeu_si512(pDst512 + 1, _mm512_xor_si512(blk1, counter1));
      _mm512_storeu_si512(pDst512 + 2, _mm512_xor_si512(blk2, counter2));

      /* Authenticate */
      /* Reflect inputs */
      blk0 = _mm512_shuffle_epi8(blk0, M512(swapBytes));
      blk1 = _mm512_shuffle_epi8(blk1, M512(swapBytes));
      blk2 = _mm512_shuffle_epi8(blk2, M512(swapBytes));

      /* Add current GHASH to src[0] */
      blk0 = _mm512_mask_xor_epi64(blk0, 0x03, blk0, ghash512);

      /* Karatsuba multiplication for 16 blocks with postponed aggregation */
      __m512i H, M, L, tmp1, tmp2, tmp3;
      AesGcmKaratsubaMul4(&blk0, &hKeys3, &hKeysKaratsuba2, &H, &M, &L);
      AesGcmKaratsubaMul4(&blk1, &hKeys2, &hKeysKaratsuba1, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);
      AesGcmKaratsubaMul4(&blk2, &hKeys1, &hKeysKaratsuba0, &tmp1, &tmp2, &tmp3);
      H = _mm512_xor_si512(H, tmp1);
      M = _mm512_xor_si512(M, tmp2);
      L = _mm512_xor_si512(L, tmp3);

      AggregateKaratsubaPartialProducts(&H, &M, &L, &ghash128);

      /* save current ghash value */
      ghash512 = _mm512_castsi128_si512(ghash128);

      pSrc512 += 3;
      pDst512 += 3;
      blocks -= (3 * 4);
   }

   if ((4 * 2) <= blocks) {
      __m512i counter0 = _mm512_add_epi32(incMsk, ctr512);
      __m512i counter1 = _mm512_add_epi32(M512(inc4_lo32x4), counter0);

      incMsk = M512(inc4_lo32x4);
      ctr512 = counter1;

      // convert back to big-endian
      counter0 = _mm512_shuffle_epi8(counter0, M512(swapBytes));
      counter1 = _mm512_shuffle_epi8(counter1, M512(swapBytes));

      cpAESEncrypt2_VAES_NI(&counter0, &counter1, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);

      _mm512_storeu_si512(pDst512, _mm512_xor_si512(blk0, counter0));
      _mm512_storeu_si512(pDst512 + 1, _mm512_xor_si512(blk1, counter1));

      /* Authenticate */
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
      pDst512 += 2;
      blocks -= (2 * 4);
   }

   for (; blocks >= 4; blocks -= 4) {
      __m512i counter0 = _mm512_add_epi32(incMsk, ctr512);

      incMsk = M512(inc4_lo32x4);
      ctr512 = counter0;

      // convert back to big-endian
      counter0 = _mm512_shuffle_epi8(counter0, M512(swapBytes));

      cpAESEncrypt1_VAES_NI(&counter0, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      _mm512_storeu_si512(pDst512, _mm512_xor_si512(blk0, counter0));

      /* Authenticate */
      blk0 = _mm512_shuffle_epi8(blk0, M512(swapBytes));
      /* Add current GHASH to src[0] */
      blk0 = _mm512_mask_xor_epi64(blk0, 0x03, blk0, ghash512);

      __m512i H, M, L;
      AesGcmKaratsubaMul4(&blk0, &hKeys0, &hKeysKaratsuba0, &H, &M, &L);

      AggregateKaratsubaPartialProducts(&H, &M, &L, &ghash128);

      /* save current ghash value */
      ghash512 = _mm512_castsi128_si512(ghash128);

      pSrc512 += 1;
      pDst512 += 1;
   }

   if (blocks) {
      __mmask8 k8   = (__mmask8)((1 << (blocks + blocks)) - 1);   // 64-bit chunks
      __mmask16 k16 = (__mmask16)((1 << (blocks << 2)) - 1);       // 32-bit chunks
      __mmask64 k64 = (__mmask64)((1LL << (blocks << 4)) - 1);     // 8-bit chunks

      __m512i counter0 = _mm512_maskz_add_epi32(k16, incMsk, ctr512);
      ctr512 = counter0;

      // convert back to big-endian
      counter0 = _mm512_maskz_shuffle_epi8(k64, counter0, M512(swapBytes));

      cpAESEncrypt1_VAES_NI(&counter0, pKeys, cipherRounds);

      __m512i blk0 = _mm512_maskz_loadu_epi64(k8, pSrc512);
      _mm512_mask_storeu_epi64(pDst512, k8, _mm512_maskz_xor_epi64(k8, blk0, counter0));

      /* Authenticate */
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

   // return last counter
   __mmask8  lastCtrK8  = blocks == 0 ? 0xC0 : (__mmask8)((Ipp8u)0x03<<((blocks-1)<<1));
   __mmask16 lastCtrK16 = blocks == 0 ? 0xF000 : (__mmask16)((Ipp16u)0xF<<((blocks-1)<<2));
   __mmask64 lastCtrK64 = blocks == 0 ? 0xFFFF000000000000 : (__mmask64)((Ipp64u)0xFFFF<<((blocks-1)<<4));

   ctr512 = _mm512_maskz_add_epi32(lastCtrK16, M512(inc1_lo32x4), ctr512);
   ctr512 = _mm512_maskz_shuffle_epi8(lastCtrK64, ctr512, M512(swapBytes));

   /* save next counter */
   _mm512_mask_compressstoreu_epi64(pCtrValue, lastCtrK8, ctr512);

   /* save current ghash value */
   ghash512 = _mm512_shuffle_epi8(ghash512, M512(swapBytes));
   _mm512_mask_storeu_epi64(pGHash, 0x03, ghash512);
}

#endif

#endif /* #if (_IPP32E>=_IPP32E_K1) */
