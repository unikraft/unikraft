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
//     cpSMS4_CBC_aesni_x4()
//     cpSMS4_CBC_aesni_x8()
//     cpSMS4_CBC_aesni_x12()
//
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)

#include "pcpsms4_y8cn.h"

/*
// 4*MBS_SMS4 processing
*/

static int cpSMS4_CBC_dec_aesni_x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
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
      //TMP[0];
      TMP[5] = _mm_loadu_si128((__m128i*)(pInp));
      TMP[6] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4));
      TMP[7] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2));
      TMP[8] = _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TRANSPOSE_INP(TMP[5],TMP[6],TMP[7],TMP[8], TMP[0]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[0]), L(TMP[0]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT(TMP[5],TMP[6],TMP[7],TMP[8], TMP[0]);
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

/*
// 8*MBS_SMS4 processing
*/
static int cpSMS4_CBC_dec_aesni_x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m128i TMP[14];
   /*
      TMP[ 0] = T
      TMP[ 1] = U
      TMP[ 2] = T0
      TMP[ 3] = T1
      TMP[ 4] = T2
      TMP[ 5] = T5
      TMP[ 6] = K0
      TMP[ 7] = K1
      TMP[ 8] = K2
      TMP[ 9] = K3
      TMP[10] = P0
      TMP[11] = P1
      TMP[12] = P2
      TMP[13] = P3
   */
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
      TRANSPOSE_INP(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);

      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TRANSPOSE_INP(TMP[10],TMP[11],TMP[12],TMP[13], TMP[0]);

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[0]), L(TMP[0]));
         TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[1]), L(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[0]), L(TMP[0]));
         TMP[11] = _mm_xor_si128(_mm_xor_si128(TMP[11], TMP[1]), L(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[9]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[13]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[0]), L(TMP[0]));
         TMP[12] = _mm_xor_si128(_mm_xor_si128(TMP[12], TMP[1]), L(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[7]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[11]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[12]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[0]), L(TMP[0]));
         TMP[13] = _mm_xor_si128(_mm_xor_si128(TMP[13], TMP[1]), L(TMP[1]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));

      TRANSPOSE_OUT(TMP[10],TMP[11],TMP[12],TMP[13], TMP[0]);
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

   return processedLen + cpSMS4_CBC_dec_aesni_x4(pOut, pInp, len-processedLen, pRKey, pIV);
}

/*
// 12*MBS_SMS4 processing
*/
#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)
IPP_OWN_DEFN (int, cpSMS4_CBC_dec_aesni_x12, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV))
#else
IPP_OWN_DEFN (int, cpSMS4_CBC_dec_aesni, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV))
#endif
{
   __ALIGN16 __m128i TMP[19];
   /*
      TMP[ 0] = T
      TMP[ 1] = U
      TMP[ 2] = V
      TMP[ 3] = T0
      TMP[ 4] = T1
      TMP[ 5] = T2
      TMP[ 6] = T3
      TMP[ 7] = K0
      TMP[ 8] = K1
      TMP[ 9] = K2
      TMP[10] = K3
      TMP[11] = P0
      TMP[12] = P1
      TMP[13] = P2
      TMP[14] = P3
      TMP[15] = Q0
      TMP[16] = Q1
      TMP[17] = Q2
      TMP[18] = Q3
   */
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
      TRANSPOSE_INP(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);

      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TMP[14] = _mm_shuffle_epi8(TMP[14], M128(swapBytes));
      TRANSPOSE_INP(TMP[11],TMP[12],TMP[13],TMP[14], TMP[0]);

      TMP[15] = _mm_shuffle_epi8(TMP[15], M128(swapBytes));
      TMP[16] = _mm_shuffle_epi8(TMP[16], M128(swapBytes));
      TMP[17] = _mm_shuffle_epi8(TMP[17], M128(swapBytes));
      TMP[18] = _mm_shuffle_epi8(TMP[18], M128(swapBytes));
      TRANSPOSE_INP(TMP[15],TMP[16],TMP[17],TMP[18], TMP[0]);

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[0]), L(TMP[0]));
         TMP[11] = _mm_xor_si128(_mm_xor_si128(TMP[11], TMP[1]), L(TMP[1]));
         TMP[15] = _mm_xor_si128(_mm_xor_si128(TMP[15], TMP[2]), L(TMP[2]));

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[0]), L(TMP[0]));
         TMP[12] = _mm_xor_si128(_mm_xor_si128(TMP[12], TMP[1]), L(TMP[1]));
         TMP[16] = _mm_xor_si128(_mm_xor_si128(TMP[16], TMP[2]), L(TMP[2]));

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[0]), L(TMP[0]));
         TMP[13] = _mm_xor_si128(_mm_xor_si128(TMP[13], TMP[1]), L(TMP[1]));
         TMP[17] = _mm_xor_si128(_mm_xor_si128(TMP[17], TMP[2]), L(TMP[2]));

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[0]), L(TMP[0]));
         TMP[14] = _mm_xor_si128(_mm_xor_si128(TMP[14], TMP[1]), L(TMP[1]));
         TMP[18] = _mm_xor_si128(_mm_xor_si128(TMP[18], TMP[2]), L(TMP[2]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));

      TRANSPOSE_OUT(TMP[11],TMP[12],TMP[13],TMP[14], TMP[0]);
      TMP[14] = _mm_shuffle_epi8(TMP[14], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));

      TRANSPOSE_OUT(TMP[15],TMP[16],TMP[17],TMP[18], TMP[0]);
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

   return processedLen + cpSMS4_CBC_dec_aesni_x8(pOut, pInp, len-processedLen, pRKey, pIV);
}

#endif /* _IPP_P8, _IPP32E_Y8 */
