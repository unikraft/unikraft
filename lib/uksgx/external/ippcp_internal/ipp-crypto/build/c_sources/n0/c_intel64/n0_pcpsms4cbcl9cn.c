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
//     SMS4 CBC decryption
// 
//  Contents:
//     cpSMS4_CBC_dec_aesni()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"

#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)

#include "pcpsms4_l9cn.h"

IPP_OWN_DEFN (int, cpSMS4_CBC_dec_aesni, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV))
{
   __ALIGN16 __m256i TMP[17];
   /*
      TMP[ 0] = T0
      TMP[ 1] = T1
      TMP[ 2] = T2
      TMP[ 3] = T3
      TMP[ 4] = K0
      TMP[ 5] = K1
      TMP[ 6] = K2
      TMP[ 7] = K3
      TMP[ 8] = P0
      TMP[ 9] = P1
      TMP[10] = P2
      TMP[11] = P3
      TMP[12] = Q0
      TMP[13] = Q1
      TMP[14] = Q2
      TMP[15] = Q3
      TMP[16] = IV
   */

   TMP[16] = _mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pIV)));

   int processedLen = len -(len % (24*MBS_SMS4));
   int n;
   for(n=0; n<processedLen; n+=(24*MBS_SMS4), pInp+=(24*MBS_SMS4), pOut+=(24*MBS_SMS4)) {
      int itr;

      TMP[0] = _mm256_loadu_si256((__m256i*)(pInp));
      TMP[1] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*2));
      TMP[2] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*4));
      TMP[3] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*6));
      TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
      TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
      TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
      TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
      TRANSPOSE_INP(TMP[4],TMP[5],TMP[6],TMP[7], TMP[0],TMP[1],TMP[2],TMP[3]);

      TMP[0] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*8));
      TMP[1] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*10));
      TMP[2] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*12));
      TMP[3] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*14));
      TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
      TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
      TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
      TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
      TRANSPOSE_INP(TMP[8],TMP[9],TMP[10],TMP[11], TMP[0],TMP[1],TMP[2],TMP[3]);

      TMP[0] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*16));
      TMP[1] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*18));
      TMP[2] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*20));
      TMP[3] = _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*22));
      TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
      TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
      TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
      TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
      TRANSPOSE_INP(TMP[12],TMP[13],TMP[14],TMP[15], TMP[0],TMP[1],TMP[2],TMP[3]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[0]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[5]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[6]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[7]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[9]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[10]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[11]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[13]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[14]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[15]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[4] = _mm256_xor_si256(_mm256_xor_si256(TMP[4], TMP[0]), L(TMP[0]));
         TMP[8] = _mm256_xor_si256(_mm256_xor_si256(TMP[8], TMP[1]), L(TMP[1]));
         TMP[12] = _mm256_xor_si256(_mm256_xor_si256(TMP[12], TMP[2]), L(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[1]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[6]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[7]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[4]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[10]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[11]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[8]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[14]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[15]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[12]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[5] = _mm256_xor_si256(_mm256_xor_si256(TMP[5], TMP[0]), L(TMP[0]));
         TMP[9] = _mm256_xor_si256(_mm256_xor_si256(TMP[9], TMP[1]), L(TMP[1]));
         TMP[13] = _mm256_xor_si256(_mm256_xor_si256(TMP[13], TMP[2]), L(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[2]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[7]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[4]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[5]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[11]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[8]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[9]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[15]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[12]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[13]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[6] = _mm256_xor_si256(_mm256_xor_si256(TMP[6], TMP[0]), L(TMP[0]));
         TMP[10] = _mm256_xor_si256(_mm256_xor_si256(TMP[10], TMP[1]), L(TMP[1]));
         TMP[14] = _mm256_xor_si256(_mm256_xor_si256(TMP[14], TMP[2]), L(TMP[2]));

         /* initial xors */
         TMP[2] = TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[3]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[4]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[5]);
         TMP[0] = _mm256_xor_si256(TMP[0], TMP[6]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[8]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[9]);
         TMP[1] = _mm256_xor_si256(TMP[1], TMP[10]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[12]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[13]);
         TMP[2] = _mm256_xor_si256(TMP[2], TMP[14]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[7] = _mm256_xor_si256(_mm256_xor_si256(TMP[7], TMP[0]), L(TMP[0]));
         TMP[11] = _mm256_xor_si256(_mm256_xor_si256(TMP[11], TMP[1]), L(TMP[1]));
         TMP[15] = _mm256_xor_si256(_mm256_xor_si256(TMP[15], TMP[2]), L(TMP[2]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT(TMP[0],TMP[1],TMP[2],TMP[3], TMP[4],TMP[5],TMP[6],TMP[7]);
      TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
      TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
      TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
      TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
      TMP[4] = _mm256_xor_si256(TMP[0], _mm256_inserti128_si256(TMP[16], _mm_loadu_si128((__m128i*)(pInp)), 1));
      TMP[5] = _mm256_xor_si256(TMP[1], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4)));
      TMP[6] = _mm256_xor_si256(TMP[2], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*3)));
      TMP[7] = _mm256_xor_si256(TMP[3], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*5)));

      TRANSPOSE_OUT(TMP[0],TMP[1],TMP[2],TMP[3], TMP[8],TMP[9],TMP[10],TMP[11]);
      TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
      TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
      TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
      TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
      TMP[8] = _mm256_xor_si256(TMP[0], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*7)));
      TMP[9] = _mm256_xor_si256(TMP[1], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*9)));
      TMP[10] = _mm256_xor_si256(TMP[2], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*11)));
      TMP[11] = _mm256_xor_si256(TMP[3], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*13)));

      TRANSPOSE_OUT(TMP[0],TMP[1],TMP[2],TMP[3], TMP[12],TMP[13],TMP[14],TMP[15]);
      TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
      TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
      TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
      TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
      TMP[12] = _mm256_xor_si256(TMP[0], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*15)));
      TMP[13] = _mm256_xor_si256(TMP[1], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*17)));
      TMP[14] = _mm256_xor_si256(TMP[2], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*19)));
      TMP[15] = _mm256_xor_si256(TMP[3], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*21)));

      TMP[16] = _mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*23)));

      _mm256_storeu_si256((__m256i*)(pOut),            TMP[4]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*2), TMP[5]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*4), TMP[6]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*6), TMP[7]);

      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*8),  TMP[8]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*10), TMP[9]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*12), TMP[10]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*14), TMP[11]);

      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*16), TMP[12]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*18), TMP[13]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*20), TMP[14]);
      _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*22), TMP[15]);
   }

   _mm_storeu_si128((__m128i*)(pIV), _mm256_castsi256_si128(TMP[16]));

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm256_xor_si256(TMP[i],TMP[i]);
   }

   return processedLen + cpSMS4_CBC_dec_aesni_x12(pOut, pInp, len-processedLen, pRKey, pIV);
}

#endif /* _IPP_G9, _IPP32E_L9 */
