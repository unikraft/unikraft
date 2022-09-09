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
//     SMS4 ECB decryption
//
//  Contents:
//     cpSMS4_ECB_gfni512()
//     cpSMS4_ECB_gfni512x48()
//     cpSMS4_ECB_gfni512x32()
//     cpSMS4_ECB_gfni512x16()
//     cpSMS4_ECB_gfni128x12()
//     cpSMS4_ECB_gfni128x8()
//     cpSMS4_ECB_gfni128x4()
//     cpSMS4_ECB_gfni128x4()
//     cpSMS4_ECB_gfni128_tail()
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
int cpSMS4_ECB_gfni512x48(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey);
static
int cpSMS4_ECB_gfni512x32(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey);
static
int cpSMS4_ECB_gfni512x16(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey);
static 
int cpSMS4_ECB_gfni128x12(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey);
static
int  cpSMS4_ECB_gfni128x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey);
static
int  cpSMS4_ECB_gfni128x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey);
static
int  cpSMS4_ECB_gfni128_tail(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey);

/*
// 64*MBS_SMS4 bytes processing
*/

IPP_OWN_DEFN (int, cpSMS4_ECB_gfni512, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey))
{
   __ALIGN16 __m512i TMP[20];

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

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[3]);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), TMP[3]);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12], TMP[13], TMP[14], TMP[15]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 32), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 36), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 40), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 44), TMP[3]);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[16], TMP[17], TMP[18], TMP[19]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 48), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 52), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 56), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 60), TMP[3]);
   }

   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }

   len -= processedLen;
   if (len){
      if(len < 256){
         processedLen += cpSMS4_ECB_gfni128x12(pOut, pInp, len, pRKey);
         return processedLen;
      }
      processedLen += cpSMS4_ECB_gfni512x48(pOut, pInp, len, pRKey);
   }

   return processedLen;
}

/*
// 48*MBS_SMS4 bytes processing
*/

static
int cpSMS4_ECB_gfni512x48(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m512i TMP[16];

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

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[3]);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), TMP[3]);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12], TMP[13], TMP[14], TMP[15]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 32), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 36), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 40), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 44), TMP[3]);
   }

   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_ECB_gfni512x32(pOut, pInp, len, pRKey);

   return processedLen;
}

/*
// 32*MBS_SMS4 bytes processing
*/

static
int cpSMS4_ECB_gfni512x32(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m512i TMP[12];

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

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[3]);

      TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
      TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
      TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
      TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), TMP[3]);
   }

   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_ECB_gfni512x16(pOut, pInp, len, pRKey);

   return processedLen;
}

/*
// 16*MBS_SMS4 bytes processing
*/

static
int cpSMS4_ECB_gfni512x16(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m512i TMP[8];

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
      _mm512_storeu_si512((__m512i*)(pOut), TMP[0]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4), TMP[1]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8), TMP[2]);
      _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), TMP[3]);
   }

   /* clear secret data */
   for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i], TMP[i]);
   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_ECB_gfni128x12(pOut, pInp, len, pRKey);

   return processedLen;
}

/*
// 12*MBS_SMS4 processing
*/

static 
int cpSMS4_ECB_gfni128x12(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m128i TMP[15];

   int processedLen = len -(len % (12*MBS_SMS4));
   int n;
   for(n=0; n<processedLen; n+=(12*MBS_SMS4), pInp+=(12*MBS_SMS4), pOut+=(12*MBS_SMS4)) {
      int itr;

      TMP[3] = _mm_loadu_si128((__m128i*)(pInp));
      TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
      TMP[5] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
      TMP[6] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));

      TMP[7] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*4));
      TMP[8] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*5));
      TMP[9] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*6));
      TMP[10] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*7));

      TMP[11] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*8));
      TMP[12] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*9));
      TMP[13] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*10));
      TMP[14] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*11));

      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[3],TMP[4],TMP[5],TMP[6], TMP[0]);

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

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[1] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[2] = TMP[1];
         TMP[0] = TMP[1];
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[12]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[13]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[14]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L128(TMP[0]));
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[1]), L128(TMP[1]));
         TMP[11] = _mm_xor_si128(_mm_xor_si128(TMP[11], TMP[2]), L128(TMP[2]));

         /* initial xors */
         TMP[1] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[2] = TMP[1];
         TMP[0] = TMP[1];
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[13]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[14]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[11]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L128(TMP[0]));
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[1]), L128(TMP[1]));
         TMP[12] = _mm_xor_si128(_mm_xor_si128(TMP[12], TMP[2]), L128(TMP[2]));

         /* initial xors */
         TMP[1] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[2] = TMP[1];
         TMP[0] = TMP[1];
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[14]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[11]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[12]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[0]), L128(TMP[0]));
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[1]), L128(TMP[1]));
         TMP[13] = _mm_xor_si128(_mm_xor_si128(TMP[13], TMP[2]), L128(TMP[2]));

         /* initial xors */
         TMP[1] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[2] = TMP[1];
         TMP[0] = TMP[1];
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[11]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[12]);
         TMP[2] = _mm_xor_si128(TMP[2], TMP[13]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         TMP[2] = sBox128(TMP[2]);
         /* Sbox done, now L */
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[0]), L128(TMP[0]));
         TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[1]), L128(TMP[1]));
         TMP[14] = _mm_xor_si128(_mm_xor_si128(TMP[14], TMP[2]), L128(TMP[2]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[3],TMP[4],TMP[5],TMP[6], TMP[0]);
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut), TMP[6]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[5]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[4]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[3]);

      TRANSPOSE_OUT_128(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*4), TMP[10]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*5), TMP[9]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*6), TMP[8]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*7), TMP[7]);

      TRANSPOSE_OUT_128(TMP[11],TMP[12],TMP[13],TMP[14], TMP[0]);
      TMP[14] = _mm_shuffle_epi8(TMP[14], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*8), TMP[14]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*9), TMP[13]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*10), TMP[12]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*11), TMP[11]);
   }

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   len -= processedLen;
   if(len)
      processedLen += cpSMS4_ECB_gfni128x8(pOut, pInp, len, pRKey);

   return processedLen;
}

/*
// 8*MBS_SMS4 bytes processing
*/

static 
int cpSMS4_ECB_gfni128x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m128i TMP[10];

   int processedLen = len & -(8*MBS_SMS4);
   int n;
   for(n=0; n<processedLen; n+=(8*MBS_SMS4), pInp+=(8*MBS_SMS4), pOut+=(8*MBS_SMS4)) {
      int itr;
      TMP[2] = _mm_loadu_si128((__m128i*)(pInp));
      TMP[3] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
      TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
      TMP[5] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));

      TMP[6] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*4));
      TMP[7] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*5));
      TMP[8] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*6));
      TMP[9] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*7));

      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[2],TMP[3],TMP[4],TMP[5], TMP[0]);

      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[0]), L128(TMP[0]));
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[1]), L128(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[6]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L128(TMP[0]));
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[1]), L128(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[6]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L128(TMP[0]));
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[1]), L128(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[6]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         TMP[1] = sBox128(TMP[1]);
         /* Sbox done, now L */
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[0]), L128(TMP[0]));
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[1]), L128(TMP[1]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[2],TMP[3],TMP[4],TMP[5], TMP[0]);
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut), TMP[5]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[4]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[3]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[2]);

      TRANSPOSE_OUT_128(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*4), TMP[9]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*5), TMP[8]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*6), TMP[7]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*7), TMP[6]);
   }

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   len -= processedLen;
   if(len)
      processedLen += cpSMS4_ECB_gfni128x4(pOut, pInp, len, pRKey);

   return processedLen;
}

static
int cpSMS4_ECB_gfni128x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m128i TMP[5];

   int processedLen = len & -(4*MBS_SMS4);
   int n;
   for(n=0; n<processedLen; n+=(4*MBS_SMS4), pInp+=(4*MBS_SMS4), pOut+=(4*MBS_SMS4)) {
      int itr;
      TMP[1] = _mm_loadu_si128((__m128i*)(pInp));
      TMP[2] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
      TMP[3] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
      TMP[4] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L128(TMP[0]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut), TMP[4]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[3]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[2]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[1]);
   }

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   len -= processedLen;
   if(len)
      processedLen += cpSMS4_ECB_gfni128_tail(pOut, pInp, len, pRKey);

   return processedLen;
}

/*
// (1-3)*MBS_SMS4 processing
*/

static 
int cpSMS4_ECB_gfni128_tail(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m128i TMP[6];

   TMP[2] = _mm_setzero_si128();
   TMP[3] = _mm_setzero_si128();
   TMP[4] = _mm_setzero_si128();

   switch (len) {
   case (3*MBS_SMS4):
      TMP[3] = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(pInp+2*MBS_SMS4)), M128(swapBytes));
   case (2*MBS_SMS4):
      TMP[2] = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(pInp+1*MBS_SMS4)), M128(swapBytes));
   case (1*MBS_SMS4):
      TMP[1] = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(pInp+0*MBS_SMS4)), M128(swapBytes));
      break;
   default: return 0;
   }
   TRANSPOSE_INP_128(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);

   {
      int itr;
      for(itr=0; itr<8; itr++, pRKey+=4) {
      TMP[5] = _mm_loadu_si128((__m128i*)pRKey);

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(TMP[5], 0x00); /* broadcast(key4 TMP[0]) */
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(TMP[5], 0x55); /* broadcast(key4 TMP[1]) */
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(TMP[5], 0xAA);  /* broadcast(key4 TMP[2]) */
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L128(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(TMP[5], 0xFF);  /* broadcast(key4 TMP[3]) */
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         /* Sbox */
         TMP[0] = sBox128(TMP[0]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L128(TMP[0]));
      }
   }

   TRANSPOSE_OUT_128(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);
   TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
   TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
   TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
   TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));

   switch (len) {
   case (3*MBS_SMS4):
      _mm_storeu_si128((__m128i*)(pOut+2*MBS_SMS4), TMP[2]);
   case (2*MBS_SMS4):
      _mm_storeu_si128((__m128i*)(pOut+1*MBS_SMS4), TMP[3]);
   case (1*MBS_SMS4):
      _mm_storeu_si128((__m128i*)(pOut+0*MBS_SMS4), TMP[4]);
      break;
   }

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }

   return len;
}

#endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */

#endif /* _IPP32E>=_IPP32E_K1 */
