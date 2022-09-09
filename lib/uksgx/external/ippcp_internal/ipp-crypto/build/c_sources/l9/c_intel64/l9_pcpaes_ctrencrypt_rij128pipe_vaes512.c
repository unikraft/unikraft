/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     AES encryption/decryption (CTR mode)
//
//  Contents:
//     EncryptCTR_RIJ128pipe_VAES_NI
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "pcpaes_encrypt_vaes512.h"

#if (_IPP32E>=_IPP32E_K1)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4310) // cast truncates constant value in MSVC
#endif

#define M512(mem)    (*((__m512i*)((Ipp8u*)(mem))))

/* Mask to convert 64-bit parts of four 128-bit numbers stored in one 512-bit register
 * from Little-Endian to Big-Endian */
static __ALIGN32 Ipp8u swapBytes[] = {
   7, 6, 5, 4,  3, 2, 1, 0,  15,14,13,12, 11,10, 9, 8,
   23,22,21,20, 19,18,17,16, 31,30,29,28, 27,26,25,24,
   39,38,37,36, 35,34,33,32, 47,46,45,44, 43,42,41,40,
   55,54,53,52, 51,50,49,48, 63,62,61,60, 59,58,57,56
};

/* Increment masks for Hi-Lo 64-bit parts of 128-bit numbers in 512-bit register */
static __ALIGN64 Ipp64u startIncLoMask[] = { 0x0, 0x0, 0x0, 0x1, 0x0, 0x2, 0x0, 0x3 };
static __ALIGN64 Ipp64u nextIncLoMask[]  = { 0x0, 0x4, 0x0, 0x4, 0x0, 0x4, 0x0, 0x4 };
static __ALIGN64 Ipp64u incLoByOneMask[] = { 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1 };
static __ALIGN64 Ipp64u incHiByOneMask[] = { 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0 };

__INLINE __m512i adcLo_epi64(__m512i a, __m512i b)
{
   a = _mm512_add_epi64(a, b);
   // check overflow in each low 64-bit of 128-bit numbers
   __mmask8 overMsk = _mm512_cmplt_epu64_mask(a, b);
   // get mask of each high 64-bit that need to be increased
   overMsk <<= 1;
   a = _mm512_mask_add_epi64(a, overMsk, a, M512(incHiByOneMask));
   return a;
}

__INLINE __m512i applyNonce(__m512i a, __m512i ctrBitMask, __m512i templateCtr)
{
   a = _mm512_shuffle_epi8(a, M512(swapBytes));
   a = _mm512_and_epi64(a, ctrBitMask);
   a = _mm512_or_epi64(a, templateCtr);

   return a;
}

////////////////////////////////////////////////////////////////////////////////

IPP_OWN_DEFN (void, EncryptCTR_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc,
                                                   Ipp8u* pDst,
                                                   int nr,
                                                   const Ipp8u* pRKey,
                                                   int length,         /* message length in bytes   */
                                                   Ipp8u* pCtrValue,
                                                   const Ipp8u* pCtrBitMask))
{
   int cipherRounds = nr - 1;

   __m128i* pKeys   = (__m128i*)pRKey;
   __m512i* pSrc512 = (__m512i*)pSrc;
   __m512i* pDst512 = (__m512i*)pDst;

   Ipp64u* pCtr64    = (Ipp64u*)pCtrValue;

   // Initial counter
   __m128i initialCtr128  = _mm_maskz_loadu_epi64(0x03, pCtrValue);
   __m128i ctrBitMask128  = _mm_maskz_loadu_epi64(0x03, pCtrBitMask);
   // Unchanged counter part
   __m128i templateCtr128 = _mm_maskz_andnot_epi64(0x03 /* all 128-bits */, ctrBitMask128, initialCtr128);

   __m512i ctrBitMask512  = _mm512_broadcast_i64x2(ctrBitMask128);
   __m512i templateCtr512 = _mm512_broadcast_i64x2(templateCtr128);

   Ipp64u ctr64_h = ENDIANNESS64(pCtr64[0]); // high 64-bit of BE counter converted to LE
   Ipp64u ctr64_l = ENDIANNESS64(pCtr64[1]); // low 64-bit of BE counter converted to LE

   __m512i ctr512 = _mm512_set4_epi64((Ipp64s)ctr64_l, (Ipp64s)ctr64_h, (Ipp64s)ctr64_l , (Ipp64s)ctr64_h);

   int blocks;
   __m512i incMsk = M512(startIncLoMask);
   for (blocks = length / MBS_RIJ128; blocks >= (4 * 4); blocks -= (4 * 4)) {
      __m512i counter0 = adcLo_epi64(incMsk, ctr512);
      __m512i counter1 = adcLo_epi64(M512(nextIncLoMask), counter0);
      __m512i counter2 = adcLo_epi64(M512(nextIncLoMask), counter1);
      __m512i counter3 = adcLo_epi64(M512(nextIncLoMask), counter2);

      incMsk = M512(nextIncLoMask);
      ctr512 = counter3;

      // convert back to BE and add nonce
      counter0 = applyNonce(counter0, ctrBitMask512, templateCtr512);
      counter1 = applyNonce(counter1, ctrBitMask512, templateCtr512);
      counter2 = applyNonce(counter2, ctrBitMask512, templateCtr512);
      counter3 = applyNonce(counter3, ctrBitMask512, templateCtr512);

      cpAESEncrypt4_VAES_NI(&counter0, &counter1, &counter2, &counter3, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pSrc512 + 3);

      blk0 = _mm512_xor_si512(blk0, counter0);
      blk1 = _mm512_xor_si512(blk1, counter1);
      blk2 = _mm512_xor_si512(blk2, counter2);
      blk3 = _mm512_xor_si512(blk3, counter3);

      _mm512_storeu_si512(pDst512, blk0);
      _mm512_storeu_si512(pDst512 + 1, blk1);
      _mm512_storeu_si512(pDst512 + 2, blk2);
      _mm512_storeu_si512(pDst512 + 3, blk3);

      pSrc512 += 4;
      pDst512 += 4;
   }

   if ((3 * 4) <= blocks) {
      __m512i counter0 = adcLo_epi64(incMsk, ctr512);
      __m512i counter1 = adcLo_epi64(M512(nextIncLoMask), counter0);
      __m512i counter2 = adcLo_epi64(M512(nextIncLoMask), counter1);

      incMsk = M512(nextIncLoMask);
      ctr512 = counter2;

      // convert back to BE and add nonce
      counter0 = applyNonce(counter0, ctrBitMask512, templateCtr512);
      counter1 = applyNonce(counter1, ctrBitMask512, templateCtr512);
      counter2 = applyNonce(counter2, ctrBitMask512, templateCtr512);

      cpAESEncrypt3_VAES_NI(&counter0, &counter1, &counter2, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512 + 2);

      blk0 = _mm512_xor_si512(blk0, counter0);
      blk1 = _mm512_xor_si512(blk1, counter1);
      blk2 = _mm512_xor_si512(blk2, counter2);

      _mm512_storeu_si512(pDst512, blk0);
      _mm512_storeu_si512(pDst512 + 1, blk1);
      _mm512_storeu_si512(pDst512 + 2, blk2);

      pSrc512 += 3;
      pDst512 += 3;
      blocks -= (3 * 4);
   }

   if ((4 * 2) <= blocks) {
      __m512i counter0 = adcLo_epi64(incMsk, ctr512);
      __m512i counter1 = adcLo_epi64(M512(nextIncLoMask), counter0);

      incMsk = M512(nextIncLoMask);
      ctr512 = counter1;

      // convert back to BE and add nonce
      counter0 = applyNonce(counter0, ctrBitMask512, templateCtr512);
      counter1 = applyNonce(counter1, ctrBitMask512, templateCtr512);

      cpAESEncrypt2_VAES_NI(&counter0, &counter1, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);

      blk0 = _mm512_xor_si512(blk0, counter0);
      blk1 = _mm512_xor_si512(blk1, counter1);

      _mm512_storeu_si512(pDst512, blk0);
      _mm512_storeu_si512(pDst512 + 1, blk1);

      pSrc512 += 2;
      pDst512 += 2;
      blocks -= (2 * 4);
   }

   for (; blocks >= 4; blocks -= 4) {
      __m512i counter0 = adcLo_epi64(incMsk, ctr512);

      incMsk = M512(nextIncLoMask);
      ctr512 = counter0;

      // convert back to BE and add nonce
      counter0 = applyNonce(counter0, ctrBitMask512, templateCtr512);

      cpAESEncrypt1_VAES_NI(&counter0, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      blk0 = _mm512_xor_si512(blk0, counter0);
      _mm512_storeu_si512(pDst512, blk0);

      pSrc512 += 1;
      pDst512 += 1;
   }

   if (blocks) {
      __mmask8 k8   = (__mmask8)((1 << (blocks + blocks)) - 1);   // 64-bit chunks
      __mmask64 k64 = (__mmask64)((1LL << (blocks << 4)) - 1);     // 8-bit chunks

      __m512i counter0 = _mm512_maskz_add_epi64(k8, incMsk, ctr512);
      __mmask8 overMsk = _mm512_mask_cmplt_epu64_mask(k8, counter0, ctr512);
      overMsk <<= 1;
      counter0 = _mm512_mask_add_epi64(counter0, overMsk, counter0, M512(incHiByOneMask));

      incMsk = M512(nextIncLoMask);
      ctr512 = counter0;

      // convert back to BE and add nonce
      counter0 = _mm512_maskz_shuffle_epi8(k64, counter0, M512(swapBytes));
      counter0 = _mm512_maskz_and_epi64(k8, counter0, ctrBitMask512);
      counter0 = _mm512_maskz_or_epi64(k8, counter0, templateCtr512);

      cpAESEncrypt1_VAES_NI(&counter0, pKeys, cipherRounds);

      __m512i blk0 = _mm512_maskz_loadu_epi64(k8, pSrc512);
      blk0 = _mm512_maskz_xor_epi64(k8, blk0, counter0);
      _mm512_mask_storeu_epi64(pDst512, k8, blk0);
   }

   // return last counter
   __mmask8  lastCtrK8  = blocks == 0 ? 0xC0 : (__mmask8)((Ipp8u)0x03<<((blocks-1)<<1));
   __mmask64 lastCtrK64 = blocks == 0 ? 0xFFFF000000000000 : (__mmask64)((Ipp64u)0xFFFF<<((blocks-1)<<4));

   ctr512 = adcLo_epi64(M512(incLoByOneMask), ctr512);

   ctr512 = _mm512_maskz_shuffle_epi8(lastCtrK64, ctr512, M512(swapBytes));
   ctr512 = _mm512_maskz_and_epi64(lastCtrK8, ctr512, ctrBitMask512);
   ctr512 = _mm512_maskz_or_epi64(lastCtrK8, ctr512, templateCtr512);

   _mm512_mask_compressstoreu_epi64(pCtrValue, lastCtrK8, ctr512);
}

#endif /* #if (_IPP32E>=_IPP32E_K1) */
