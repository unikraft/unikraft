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
//     EncryptStreamCTR32_VAES_NI
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "pcpaes_encrypt_vaes512.h"

#if (_IPP32E>=_IPP32E_K1)

/* Mask to convert low 32-bit parts of four 128-bit Big-Endian numbers
 * stored in 512-bit register. The low 32-bit of each number converted
 * from LE to BE */
static __ALIGN32 Ipp8u swapBytes[] = {
   0,  1, 2, 3,  4, 5, 6, 7,  8, 9,10,11, 15,14,13,12,
   16,17,18,19, 20,21,22,23, 24,25,26,27, 31,30,29,28,
   32,33,34,35, 36,37,38,39, 40,41,42,43, 47,46,45,44,
   48,49,50,51, 52,53,54,55, 56,57,58,59, 63,62,61,60
};

////////////////////////////////////////////////////////////////////////////////

IPP_OWN_DEFN (void, EncryptStreamCTR32_VAES_NI, (const Ipp8u* pSrc,
                                                Ipp8u* pDst,
                                                int nr,
                                                const Ipp8u* pRKey,
                                                int length,         /* message length in bytes   */
                                                Ipp8u* pIV))         /* BE counter representation */
{
   int cipherRounds = nr - 1;

   __m128i* pKeys   = (__m128i*)pRKey;
   __m512i* pSrc512 = (__m512i*)pSrc;
   __m512i* pDst512 = (__m512i*)pDst;

   Ipp32u* pIV32    = (Ipp32u*)pIV;
   Ipp64u* pIV64    = (Ipp64u*)pIV;

   // start increment mask for IV
   __m512i startIncMask = _mm512_set_epi32(0x3, 0x0, 0x0, 0x0,
                                           0x2, 0x0, 0x0, 0x0,
                                           0x1, 0x0, 0x0, 0x0,
                                           0x0, 0x0, 0x0, 0x0);
 
   // continuous increment mask for IV
   __m512i incMask = _mm512_set_epi32(0x4, 0x0, 0x0, 0x0,
                                      0x4, 0x0, 0x0, 0x0,
                                      0x4, 0x0, 0x0, 0x0,
                                      0x4, 0x0, 0x0, 0x0);

   // initial BE counter with 32-bit low part converted to LE:
   // pIV: f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | fc fd fe ff
   // IV:  f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | ff fe fd fc
   __m512i IV = _mm512_set4_epi32((Ipp32s)ENDIANNESS32(pIV32[3]), (Ipp32s)pIV32[2], (Ipp32s)pIV32[1], (Ipp32s)pIV32[0]);


   // Update IV counter for next function calls
   int blocks = length / MBS_RIJ128;

   Ipp64u ctr64_h = ENDIANNESS64(pIV64[0]); // high 64-bit of BE IV converted to LE
   Ipp64u ctr64_l = ENDIANNESS64(pIV64[1]); // low 32-bit of BE pIV converted to LE

   ctr64_l += (Ipp64u)blocks;
   if (ctr64_l < blocks) { // overflow of low part
      ctr64_h += 1;
   }

   // update IV
   pIV64[0] = ENDIANNESS64(ctr64_h);
   pIV64[1] = ENDIANNESS64(ctr64_l);

   //--------------------------------------

   for (blocks = length / MBS_RIJ128; blocks >= (4 * 4); blocks -= (4 * 4)) {
      // counter0:  f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | ff fe fd fc
      // counter1:  f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | 00 ff fd fc
      // counter2:  f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | 01 ff fd fc
      // counter3:  f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | 02 ff fd fc
      __m512i counter0 = _mm512_add_epi32(startIncMask, IV);
      __m512i counter1 = _mm512_add_epi32(incMask, counter0);
      __m512i counter2 = _mm512_add_epi32(incMask, counter1);
      __m512i counter3 = _mm512_add_epi32(incMask, counter2);

      startIncMask = incMask;
      IV           = counter3;

      // convert last 32-bit (LE->BE):
      // counter0: f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | fc fd fe ff
      // counter1: f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | fc fd ff 00
      // counter2: f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | fc fd ff 01
      // counter3: f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb | fc fd ff 02
      counter0 = _mm512_shuffle_epi8(counter0, *((__m512i*)swapBytes));
      counter1 = _mm512_shuffle_epi8(counter1, *((__m512i*)swapBytes));
      counter2 = _mm512_shuffle_epi8(counter2, *((__m512i*)swapBytes));
      counter3 = _mm512_shuffle_epi8(counter3, *((__m512i*)swapBytes));

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
      __m512i counter0 = _mm512_add_epi32(startIncMask, IV);
      __m512i counter1 = _mm512_add_epi32(incMask, counter0);
      __m512i counter2 = _mm512_add_epi32(incMask, counter1);

      startIncMask = incMask;
      IV           = counter2;

      // convert last 32-bit (LE->BE)
      counter0 = _mm512_shuffle_epi8(counter0, *((__m512i*)swapBytes));
      counter1 = _mm512_shuffle_epi8(counter1, *((__m512i*)swapBytes));
      counter2 = _mm512_shuffle_epi8(counter2, *((__m512i*)swapBytes));

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
      __m512i counter0 = _mm512_add_epi32(startIncMask, IV);
      __m512i counter1 = _mm512_add_epi32(incMask, counter0);

      startIncMask = incMask;
      IV           = counter1;

      // convert last 32-bit (LE->BE)
      counter0 = _mm512_shuffle_epi8(counter0, *((__m512i*)swapBytes));
      counter1 = _mm512_shuffle_epi8(counter1, *((__m512i*)swapBytes));

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
      __m512i counter0 = _mm512_add_epi32(startIncMask, IV);

      startIncMask = incMask;
      IV           = counter0;

      // convert last 32-bit (LE->BE)
      counter0 = _mm512_shuffle_epi8(counter0, *((__m512i*)swapBytes));

      cpAESEncrypt1_VAES_NI(&counter0, pKeys, cipherRounds);

      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      blk0 = _mm512_xor_si512(blk0, counter0);
      _mm512_storeu_si512(pDst512, blk0);

      pSrc512 += 1;
      pDst512 += 1;
   }

   if (blocks) {
      __mmask8 k8   = (__mmask8)((1 << (blocks + blocks)) - 1);   // 64-bit chunks
      __mmask16 k16 = (__mmask16)((1 << (blocks << 2)) - 1);       // 32-bit chunks
      __mmask64 k64 = (__mmask64)((1LL << (blocks << 4)) - 1);     // 8-bit chunks

      __m512i counter0 = _mm512_maskz_add_epi32(k16, startIncMask, IV);

      // swap last 32-bit (LE->BE)
      counter0 = _mm512_maskz_shuffle_epi8(k64, counter0, *((__m512i*)swapBytes));

      cpAESEncrypt1_VAES_NI(&counter0, pKeys, cipherRounds);

      __m512i blk0 = _mm512_maskz_loadu_epi64(k8, pSrc512);
      blk0 = _mm512_maskz_xor_epi64(k8, blk0, counter0);
      _mm512_mask_storeu_epi64(pDst512, k8, blk0);
   }
}

#endif /* #if (_IPP32E>=_IPP32E_K1) */
