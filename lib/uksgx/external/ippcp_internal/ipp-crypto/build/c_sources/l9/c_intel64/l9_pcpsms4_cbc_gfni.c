/*******************************************************************************
* Copyright 2014-2021 Intel Corporation
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
//     SMS4 ECB decryption
//
//  Contents:
//     cpSMS4_ECB_gfni512()
//     cpSMS4_CBC_dec_gfni512x48()
//     cpSMS4_CBC_dec_gfni512x32()
//     cpSMS4_CBC_dec_gfni512x16()
//     cpSMS4_CBC_dec_gfni128x12()
//     cpSMS4_CBC_dec_gfni128x8()
//     cpSMS4_CBC_dec_gfni128x4()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

#if (_IPP32E>=_IPP32E_K1)

#if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)

#include "pcpsms4_gfni.h"

static
int cpSMS4_CBC_dec_gfni512x48(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV);
static
int cpSMS4_CBC_dec_gfni512x32(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV);
static
int cpSMS4_CBC_dec_gfni512x16(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV);
static 
int cpSMS4_CBC_dec_gfni128x12(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV);
static
int cpSMS4_CBC_dec_gfni128x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV);
static 
int cpSMS4_CBC_dec_gfni128x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV);

/*
// 64*MBS_SMS4 bytes processing
*/

IPP_OWN_DEFN (int, cpSMS4_CBC_dec_gfni512, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV))
{
   __ALIGN16 __m512i TMP[21];

   __m128i IV = _mm_loadu_si128((__m128i*)(pIV)); // Non-secret data

   int processedLen = len - (len % (64 * MBS_SMS4));
   int n;
   for (n = 0; n < processedLen; n += (64 * MBS_SMS4), pInp += (64 * MBS_SMS4), pOut += (64 * MBS_SMS4)) {
      int itr;

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], TMP[0], TMP[1], TMP[2], TMP[3]);

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 16));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 20));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 24));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 28));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[8], TMP[9], TMP[10], TMP[11], TMP[0], TMP[1], TMP[2], TMP[3]);

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 32));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 36));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 40));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 44));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[12], TMP[13], TMP[14], TMP[15], TMP[0], TMP[1], TMP[2], TMP[3]);

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 48));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 52));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 56));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 60));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[16], TMP[17], TMP[18], TMP[19], TMP[0], TMP[1], TMP[2], TMP[3]);

      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[3] = TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[13]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[14]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[15]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[17]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[18]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[19]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         TMP[3] = sBox512(TMP[3]);
         /* Sbox done, now L */
         TMP[4] = _mm512_xor_si512(_mm512_xor_si512(TMP[4], TMP[0]), L512(TMP[0]));
         TMP[8] = _mm512_xor_si512(_mm512_xor_si512(TMP[8], TMP[1]), L512(TMP[1]));
         TMP[12] = _mm512_xor_si512(_mm512_xor_si512(TMP[12], TMP[2]), L512(TMP[2]));
         TMP[16] = _mm512_xor_si512(_mm512_xor_si512(TMP[16], TMP[3]), L512(TMP[3]));

         /* initial xors */
         TMP[3] = TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[14]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[15]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[12]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[18]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[19]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[16]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         TMP[3] = sBox512(TMP[3]);
         /* Sbox done, now L */
         TMP[5] = _mm512_xor_si512(_mm512_xor_si512(TMP[5], TMP[0]), L512(TMP[0]));
         TMP[9] = _mm512_xor_si512(_mm512_xor_si512(TMP[9], TMP[1]), L512(TMP[1]));
         TMP[13] = _mm512_xor_si512(_mm512_xor_si512(TMP[13], TMP[2]), L512(TMP[2]));
         TMP[17] = _mm512_xor_si512(_mm512_xor_si512(TMP[17], TMP[3]), L512(TMP[3]));

         /* initial xors */
         TMP[3] = TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[15]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[12]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[13]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[19]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[16]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[17]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         TMP[3] = sBox512(TMP[3]);
         /* Sbox done, now L */
         TMP[6] = _mm512_xor_si512(_mm512_xor_si512(TMP[6], TMP[0]), L512(TMP[0]));
         TMP[10] = _mm512_xor_si512(_mm512_xor_si512(TMP[10], TMP[1]), L512(TMP[1]));
         TMP[14] = _mm512_xor_si512(_mm512_xor_si512(TMP[14], TMP[2]), L512(TMP[2]));
         TMP[18] = _mm512_xor_si512(_mm512_xor_si512(TMP[18], TMP[3]), L512(TMP[3]));

         /* initial xors */
         TMP[3] = TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[12]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[13]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[14]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[16]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[17]);
         TMP[3] = _mm512_xor_si512(TMP[3], TMP[18]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         TMP[3] = sBox512(TMP[3]);
         /* Sbox done, now L */
         TMP[7] = _mm512_xor_si512(_mm512_xor_si512(TMP[7], TMP[0]), L512(TMP[0]));
         TMP[11] = _mm512_xor_si512(_mm512_xor_si512(TMP[11], TMP[1]), L512(TMP[1]));
         TMP[15] = _mm512_xor_si512(_mm512_xor_si512(TMP[15], TMP[2]), L512(TMP[2]));
         TMP[19] = _mm512_xor_si512(_mm512_xor_si512(TMP[19], TMP[3]), L512(TMP[3]));
      }

      pRKey -= 32;

      /* Prepare the first block for xor*/

      TMP[20] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[20] = _mm512_alignr_epi64(TMP[20], TMP[20], 6);
      TMP[20] = _mm512_inserti64x2(TMP[20], IV, 0);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[4] = _mm512_xor_si512(TMP[0], TMP[20]);
      TMP[5] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*3)));
      TMP[6] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*7)));
      TMP[7] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*11)));

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[8] = _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*15)));
      TMP[9] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*19)));
      TMP[10] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*23)));
      TMP[11] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*27)));

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12], TMP[13], TMP[14], TMP[15]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[12] = _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*31)));
      TMP[13] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*35)));
      TMP[14] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*39)));
      TMP[15] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*43)));

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[16], TMP[17], TMP[18], TMP[19]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[16] = _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*47)));
      TMP[17] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*51)));
      TMP[18] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*55)));
      TMP[19] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*59)));

      IV = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*63)); // Ciphertext, non-secret data

      _mm512_storeu_si512((__m512i*)(pOut), TMP[4]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[5]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[6]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[7]);

      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), TMP[8]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), TMP[9]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), TMP[10]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), TMP[11]);

      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 32), TMP[12]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 36), TMP[13]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 40), TMP[14]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 44), TMP[15]);

      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 48), TMP[16]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 52), TMP[17]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 56), TMP[18]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 60), TMP[19]);
   }

   _mm_storeu_si128((__m128i*)(pIV), IV);
   
   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }

   len -= processedLen;
   if (len){
      if(len < 256){
         processedLen += cpSMS4_CBC_dec_gfni128x12(pOut, pInp, len, pRKey, pIV);
         return processedLen;
      }
      processedLen += cpSMS4_CBC_dec_gfni512x48(pOut, pInp, len, pRKey, pIV);
   }

   return processedLen;
}

/*
// 48*MBS_SMS4 bytes processing
*/

static
int cpSMS4_CBC_dec_gfni512x48(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m512i TMP[17];

   __m128i IV = _mm_loadu_si128((__m128i*)(pIV)); // Non-secret data

   int processedLen = len - (len % (48 * MBS_SMS4));
   int n;
   for (n = 0; n < processedLen; n += (48 * MBS_SMS4), pInp += (48 * MBS_SMS4), pOut += (48 * MBS_SMS4)) {
      int itr;

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], TMP[0], TMP[1], TMP[2], TMP[3]);

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 16));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 20));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 24));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 28));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[8], TMP[9], TMP[10], TMP[11], TMP[0], TMP[1], TMP[2], TMP[3]);

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 32));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 36));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 40));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 44));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[12], TMP[13], TMP[14], TMP[15], TMP[0], TMP[1], TMP[2], TMP[3]);

      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[13]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[14]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[15]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         /* Sbox done, now L */
         TMP[4] = _mm512_xor_si512(_mm512_xor_si512(TMP[4], TMP[0]), L512(TMP[0]));
         TMP[8] = _mm512_xor_si512(_mm512_xor_si512(TMP[8], TMP[1]), L512(TMP[1]));
         TMP[12] = _mm512_xor_si512(_mm512_xor_si512(TMP[12], TMP[2]), L512(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[14]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[15]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[12]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         /* Sbox done, now L */
         TMP[5] = _mm512_xor_si512(_mm512_xor_si512(TMP[5], TMP[0]), L512(TMP[0]));
         TMP[9] = _mm512_xor_si512(_mm512_xor_si512(TMP[9], TMP[1]), L512(TMP[1]));
         TMP[13] = _mm512_xor_si512(_mm512_xor_si512(TMP[13], TMP[2]), L512(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[15]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[12]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[13]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         /* Sbox done, now L */
         TMP[6] = _mm512_xor_si512(_mm512_xor_si512(TMP[6], TMP[0]), L512(TMP[0]));
         TMP[10] = _mm512_xor_si512(_mm512_xor_si512(TMP[10], TMP[1]), L512(TMP[1]));
         TMP[14] = _mm512_xor_si512(_mm512_xor_si512(TMP[14], TMP[2]), L512(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[12]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[13]);
         TMP[2] = _mm512_xor_si512(TMP[2], TMP[14]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         TMP[2] = sBox512(TMP[2]);
         /* Sbox done, now L */
         TMP[7] = _mm512_xor_si512(_mm512_xor_si512(TMP[7], TMP[0]), L512(TMP[0]));
         TMP[11] = _mm512_xor_si512(_mm512_xor_si512(TMP[11], TMP[1]), L512(TMP[1]));
         TMP[15] = _mm512_xor_si512(_mm512_xor_si512(TMP[15], TMP[2]), L512(TMP[2]));
      }

      pRKey -= 32;

      /* Prepare the first block for xor*/

      TMP[16] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[16] = _mm512_alignr_epi64(TMP[16], TMP[16], 6);
      TMP[16] = _mm512_inserti64x2(TMP[16], IV, 0);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[4] = _mm512_xor_si512(TMP[0], TMP[16]);
      TMP[5] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*3)));
      TMP[6] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*7)));
      TMP[7] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*11)));

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[8] = _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*15)));
      TMP[9] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*19)));
      TMP[10] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*23)));
      TMP[11] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*27)));

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12], TMP[13], TMP[14], TMP[15]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[12] = _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*31)));
      TMP[13] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*35)));
      TMP[14] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*39)));
      TMP[15] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*43)));

      IV = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*47)); // Ciphertext, non-secret data

      _mm512_storeu_si512((__m512i*)(pOut), TMP[4]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[5]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[6]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[7]);

      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), TMP[8]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), TMP[9]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), TMP[10]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), TMP[11]);

      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 32), TMP[12]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 36), TMP[13]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 40), TMP[14]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 44), TMP[15]);
   }

   _mm_storeu_si128((__m128i*)(pIV), IV);
   
   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }

   len -= processedLen;
   if (len)
      processedLen += cpSMS4_CBC_dec_gfni512x32(pOut, pInp, len, pRKey, pIV);

   return processedLen;
}

/*
// 32*MBS_SMS4 bytes processing
*/

static
int cpSMS4_CBC_dec_gfni512x32(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m512i TMP[13];

   __m128i IV = _mm_loadu_si128((__m128i*)(pIV)); // Non-secret data

   int processedLen = len - (len % (32 * MBS_SMS4));
   int n;
   for (n = 0; n < processedLen; n += (32 * MBS_SMS4), pInp += (32 * MBS_SMS4), pOut += (32 * MBS_SMS4)) {
      int itr;

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], TMP[0], TMP[1], TMP[2], TMP[3]);

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 16));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 20));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 24));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 28));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[8], TMP[9], TMP[10], TMP[11], TMP[0], TMP[1], TMP[2], TMP[3]);

      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         /* Sbox done, now L */
         TMP[4] = _mm512_xor_si512(_mm512_xor_si512(TMP[4], TMP[0]), L512(TMP[0]));
         TMP[8] = _mm512_xor_si512(_mm512_xor_si512(TMP[8], TMP[1]), L512(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         /* Sbox done, now L */
         TMP[5] = _mm512_xor_si512(_mm512_xor_si512(TMP[5], TMP[0]), L512(TMP[0]));
         TMP[9] = _mm512_xor_si512(_mm512_xor_si512(TMP[9], TMP[1]), L512(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[11]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         /* Sbox done, now L */
         TMP[6] = _mm512_xor_si512(_mm512_xor_si512(TMP[6], TMP[0]), L512(TMP[0]));
         TMP[10] = _mm512_xor_si512(_mm512_xor_si512(TMP[10], TMP[1]), L512(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[8]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[9]);
         TMP[1] = _mm512_xor_si512(TMP[1], TMP[10]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         TMP[1] = sBox512(TMP[1]);
         /* Sbox done, now L */
         TMP[7] = _mm512_xor_si512(_mm512_xor_si512(TMP[7], TMP[0]), L512(TMP[0]));
         TMP[11] = _mm512_xor_si512(_mm512_xor_si512(TMP[11], TMP[1]), L512(TMP[1]));
      }

      pRKey -= 32;

      /* Prepare the first block for xor*/

      TMP[12] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[12] = _mm512_alignr_epi64(TMP[12], TMP[12], 6);
      TMP[12] = _mm512_inserti64x2(TMP[12], IV, 0);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[4] = _mm512_xor_si512(TMP[0], TMP[12]);
      TMP[5] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*3)));
      TMP[6] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*7)));
      TMP[7] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*11)));

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[8] = _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*15)));
      TMP[9] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*19)));
      TMP[10] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*23)));
      TMP[11] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*27)));

      IV = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*31)); // Ciphertext, non-secret data

      _mm512_storeu_si512((__m512i*)(pOut), TMP[4]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[5]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[6]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[7]);

      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), TMP[8]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), TMP[9]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), TMP[10]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), TMP[11]);
   }

   _mm_storeu_si128((__m128i*)(pIV), IV);
   
   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }

   len -= processedLen;
   if (len)
      processedLen += cpSMS4_CBC_dec_gfni512x16(pOut, pInp, len, pRKey, pIV);

   return processedLen;
}

/*
// 16*MBS_SMS4 bytes processing
*/

static
int cpSMS4_CBC_dec_gfni512x16(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m512i TMP[9];

   __m128i IV = _mm_loadu_si128((__m128i*)(pIV)); // Non-secret data

   int processedLen = len - (len % (16 * MBS_SMS4));
   int n;
   for (n = 0; n < processedLen; n += (16 * MBS_SMS4), pInp += (16 * MBS_SMS4), pOut += (16 * MBS_SMS4)) {
      int itr;

      TMP[0] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[1] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4));
      TMP[2] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8));
      TMP[3] = _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12));
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], TMP[0], TMP[1], TMP[2], TMP[3]);

      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         /* Sbox done, now L */
         TMP[4] = _mm512_xor_si512(_mm512_xor_si512(TMP[4], TMP[0]), L512(TMP[0]));

         /* initial xors */
         TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         /* Sbox done, now L */
         TMP[5] = _mm512_xor_si512(_mm512_xor_si512(TMP[5], TMP[0]), L512(TMP[0]));

         /* initial xors */
         TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[7]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         /* Sbox done, now L */
         TMP[6] = _mm512_xor_si512(_mm512_xor_si512(TMP[6], TMP[0]), L512(TMP[0]));

         /* initial xors */
         TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[4]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[5]);
         TMP[0] = _mm512_xor_si512(TMP[0], TMP[6]);
         /* Sbox */
         TMP[0] = sBox512(TMP[0]);
         /* Sbox done, now L */
         TMP[7] = _mm512_xor_si512(_mm512_xor_si512(TMP[7], TMP[0]), L512(TMP[0]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));

      TMP[8] = _mm512_loadu_si512((__m512i*)(pInp));
      TMP[8] = _mm512_alignr_epi64(TMP[8], TMP[8], 6);
      TMP[8] = _mm512_inserti64x2(TMP[8], IV, 0);

      TMP[4] = _mm512_xor_si512(TMP[0], TMP[8]);
      TMP[5] = _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*3)));
      TMP[6] = _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*7)));
      TMP[7] = _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp+MBS_SMS4*11)));

      IV = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*15)); // Ciphertext, non-secret data

      _mm512_storeu_si512((__m512i*)(pOut), TMP[4]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[5]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[6]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[7]);
   }

   _mm_storeu_si128((__m128i*)(pIV), IV);

   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_CBC_dec_gfni128x12(pOut, pInp, len, pRKey, pIV);

   return processedLen;
}

/*
// 12*MBS_SMS4 bytes processing
*/

static 
int cpSMS4_CBC_dec_gfni128x12(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m128i TMP[19];
   TMP[3] = _mm_loadu_si128((__m128i*)(pIV));

   int processedLen = len -(len % (12*MBS_SMS4));
   int n;
   for(n=0; n<processedLen; n+=(12*MBS_SMS4), pInp+=(12*MBS_SMS4), pOut+=(12*MBS_SMS4)) {
      int itr;
      TMP[7] = _mm_loadu_si128((__m128i*)(pInp));
      TMP[8] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
      TMP[9] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
      TMP[10] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));

      TMP[11] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*4));
      TMP[12] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*5));
      TMP[13] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*6));
      TMP[14] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*7));

      TMP[15] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*8));
      TMP[16] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*9));
      TMP[17] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*10));
      TMP[18] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*11));

      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);

      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TMP[14] = _mm_shuffle_epi8(TMP[14], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[11],TMP[12],TMP[13],TMP[14], TMP[0]);

      TMP[15] = _mm_shuffle_epi8(TMP[15], M128(swapBytes));
      TMP[16] = _mm_shuffle_epi8(TMP[16], M128(swapBytes));
      TMP[17] = _mm_shuffle_epi8(TMP[17], M128(swapBytes));
      TMP[18] = _mm_shuffle_epi8(TMP[18], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[15],TMP[16],TMP[17],TMP[18], TMP[0]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[10]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[14]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[16]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[17]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[18]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[0]), L128(TMP[0]));
         TMP[11] = _mm_xor_si128(_mm_xor_si128(TMP[11], TMP[1]), L128(TMP[1]));
         TMP[15] = _mm_xor_si128(_mm_xor_si128(TMP[15], TMP[2]), L128(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[10]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[14]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[17]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[18]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[15]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[0]), L128(TMP[0]));
         TMP[12] = _mm_xor_si128(_mm_xor_si128(TMP[12], TMP[1]), L128(TMP[1]));
         TMP[16] = _mm_xor_si128(_mm_xor_si128(TMP[16], TMP[2]), L128(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[10]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[14]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[18]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[15]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[16]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[0]), L128(TMP[0]));
         TMP[13] = _mm_xor_si128(_mm_xor_si128(TMP[13], TMP[1]), L128(TMP[1]));
         TMP[17] = _mm_xor_si128(_mm_xor_si128(TMP[17], TMP[2]), L128(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[15]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[16]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[17]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[0]), L128(TMP[0]));
         TMP[14] = _mm_xor_si128(_mm_xor_si128(TMP[14], TMP[1]), L128(TMP[1]));
         TMP[18] = _mm_xor_si128(_mm_xor_si128(TMP[18], TMP[2]), L128(TMP[2]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));

      TRANSPOSE_OUT_128(TMP[11],TMP[12],TMP[13],TMP[14], TMP[0]);
      TMP[14] = _mm_shuffle_epi8(TMP[14], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));

      TRANSPOSE_OUT_128(TMP[15],TMP[16],TMP[17],TMP[18], TMP[0]);
      TMP[18] = _mm_shuffle_epi8(TMP[18], M128(swapBytes));
      TMP[17] = _mm_shuffle_epi8(TMP[17], M128(swapBytes));
      TMP[16] = _mm_shuffle_epi8(TMP[16], M128(swapBytes));
      TMP[15] = _mm_shuffle_epi8(TMP[15], M128(swapBytes));

      {
         /* next 3 IV */
         TMP[4] = _mm_loadu_si128((__m128i*)(pInp));
         TMP[5] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
         TMP[6] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
         /* CBC decryption output */
         TMP[10] = _mm_xor_si128(TMP[10], TMP[3]);
         TMP[9] = _mm_xor_si128(TMP[9], TMP[4]);
         TMP[8] = _mm_xor_si128(TMP[8], TMP[5]);
         TMP[7] = _mm_xor_si128(TMP[7], TMP[6]);

         /* next 4 IV */
         TMP[3] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));
         TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*4));
         TMP[5] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*5));
         TMP[6] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*6));
         /* CBC decryption output */
         TMP[14] = _mm_xor_si128(TMP[14], TMP[3]);
         TMP[13] = _mm_xor_si128(TMP[13], TMP[4]);
         TMP[12] = _mm_xor_si128(TMP[12], TMP[5]);
         TMP[11] = _mm_xor_si128(TMP[11], TMP[6]);

         /* next 4 IV */
         TMP[3] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*7));
         TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*8));
         TMP[5] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*9));
         TMP[6] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*10));
         /* CBC decryption output */
         TMP[18] = _mm_xor_si128(TMP[18], TMP[3]);
         TMP[17] = _mm_xor_si128(TMP[17], TMP[4]);
         TMP[16] = _mm_xor_si128(TMP[16], TMP[5]);
         TMP[15] = _mm_xor_si128(TMP[15], TMP[6]);
      }

      /* next IV */
      TMP[3] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*11));

      /* store output */
      _mm_storeu_si128((__m128i*)(pOut), TMP[10]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[9]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[8]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[7]);

      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*4), TMP[14]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*5), TMP[13]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*6), TMP[12]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*7), TMP[11]);

      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*8), TMP[18]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*9), TMP[17]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*10),TMP[16]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*11),TMP[15]);
   }

   _mm_storeu_si128((__m128i*)(pIV), TMP[3]);

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }

   return processedLen + cpSMS4_CBC_dec_gfni128x8(pOut, pInp, len-processedLen, pRKey, pIV);
}


/*
// 8*MBS_SMS4 bytes processing
*/

static
int cpSMS4_CBC_dec_gfni128x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m128i TMP[14];

   TMP[2] = _mm_loadu_si128((__m128i*)(pIV));

   int processedLen = len & -(8*MBS_SMS4);
   int n;
   for(n=0; n<processedLen; n+=(8*MBS_SMS4), pInp+=(8*MBS_SMS4), pOut+=(8*MBS_SMS4)) {
      int itr;
      TMP[6] = _mm_loadu_si128((__m128i*)(pInp));
      TMP[7] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
      TMP[8] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
      TMP[9] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));

      TMP[10] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*4));
      TMP[11] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*5));
      TMP[12] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*6));
      TMP[13] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*7));

      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);

      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[10],TMP[11],TMP[12],TMP[13], TMP[0]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[0]), L128(TMP[0]));
         TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[1]), L128(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[0]), L128(TMP[0]));
         TMP[11] = _mm_xor_si128(_mm_xor_si128(TMP[11], TMP[1]), L128(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[0]), L128(TMP[0]));
         TMP[12] = _mm_xor_si128(_mm_xor_si128(TMP[12], TMP[1]), L128(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[0]), L128(TMP[0]));
         TMP[13] = _mm_xor_si128(_mm_xor_si128(TMP[13], TMP[1]), L128(TMP[1]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));

      TRANSPOSE_OUT_128(TMP[10],TMP[11],TMP[12],TMP[13], TMP[0]);
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));

      {
         /* next 3 IV */
         TMP[3] = _mm_loadu_si128((__m128i*)(pInp));
         TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
         TMP[5] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
         /* CBC decryption output */
         TMP[9] = _mm_xor_si128(TMP[9], TMP[2]);
         TMP[8] = _mm_xor_si128(TMP[8], TMP[3]);
         TMP[7] = _mm_xor_si128(TMP[7], TMP[4]);
         TMP[6] = _mm_xor_si128(TMP[6], TMP[5]);

         /* next 4 IV */
         TMP[2] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));
         TMP[3] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*4));
         TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*5));
         TMP[5] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*6));
         /* CBC decryption output */
         TMP[13] = _mm_xor_si128(TMP[13], TMP[2]);
         TMP[12] = _mm_xor_si128(TMP[12], TMP[3]);
         TMP[11] = _mm_xor_si128(TMP[11], TMP[4]);
         TMP[10] = _mm_xor_si128(TMP[10], TMP[5]);
      }

      /* next IV */
      TMP[2] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*7));

      /* store output */
      _mm_storeu_si128((__m128i*)(pOut), TMP[9]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[8]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[7]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[6]);

      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*4), TMP[13]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*5), TMP[12]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*6), TMP[11]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*7), TMP[10]);
   }

   _mm_storeu_si128((__m128i*)(pIV), TMP[2]);

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }

   return processedLen + cpSMS4_CBC_dec_gfni128x4(pOut, pInp, len-processedLen, pRKey, pIV);
}

/*
// 4*MBS_SMS4 processing
*/

static 
int cpSMS4_CBC_dec_gfni128x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m128i TMP[9];
   /*
      TMP[0] = T
      TMP[1] = T0
      TMP[2] = T1
      TMP[3] = T2
      TMP[4] = T3
      TMP[5] = K0
      TMP[6] = K1
      TMP[7] = K2
      TMP[8] = K3
   */
   TMP[1] = _mm_loadu_si128((__m128i*)(pIV));

   int processedLen = len & -(4*MBS_SMS4);
   int n;
   for(n=0; n<processedLen; n+=(4*MBS_SMS4), pInp+=(4*MBS_SMS4), pOut+=(4*MBS_SMS4)) {
      int itr;
      TMP[5] = _mm_loadu_si128((__m128i*)(pInp));
      TMP[6] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
      TMP[7] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
      TMP[8] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[5],TMP[6],TMP[7],TMP[8], TMP[0]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[0]), L128(TMP[0]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[5],TMP[6],TMP[7],TMP[8], TMP[0]);
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));

      {
         /* next 3 IV */
         TMP[2] = _mm_loadu_si128((__m128i*)(pInp));
         TMP[3] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
         TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
         /* CBC decryption output */
         TMP[8] = _mm_xor_si128(TMP[8], TMP[1]);
         TMP[7] = _mm_xor_si128(TMP[7], TMP[2]);
         TMP[6] = _mm_xor_si128(TMP[6], TMP[3]);
         TMP[5] = _mm_xor_si128(TMP[5], TMP[4]);
      }

      /* next IV */
      TMP[1] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));

      /* store output */
      _mm_storeu_si128((__m128i*)(pOut), TMP[8]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[7]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[6]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[5]);
   }

   _mm_storeu_si128((__m128i*)(pIV), TMP[1]);

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }

   return processedLen;
}

#endif /* if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */

#endif /* _IPP32E>=_IPP32E_K1 */
