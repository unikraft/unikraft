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
//     SMS4 ECB encryption/decryption
// 
//  Contents:
//     cpSMS4_ECB_aesni_x4()
//     cpSMS4_ECB_aesni_x8()
//     cpSMS4_ECB_aesni_x12()
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
// (1-3)*MBS_SMS4 processing
*/

static int cpSMS4_ECB_aesni_tail(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m128i TMP[6];
   /*
      TMP[0] = T
      TMP[1] = K0
      TMP[2] = K1
      TMP[3] = K2
      TMP[4] = K3
      TMP[5] = key4
   */

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
   TRANSPOSE_INP(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);

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
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(TMP[5], 0x55); /* broadcast(key4 TMP[1]) */
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(TMP[5], 0xAA);  /* broadcast(key4 TMP[2]) */
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(TMP[5], 0xFF);  /* broadcast(key4 TMP[3]) */
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L(TMP[0]));
      }
   }

   TRANSPOSE_OUT(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);
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

/*
// 4*MBS_SMS4 processing
*/
static
int cpSMS4_ECB_aesni_x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m128i TMP[5];
   /*
      TMP[0] = T
      TMP[1] = K0
      TMP[2] = K1
      TMP[3] = K2
      TMP[4] = K3
   */
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
      TRANSPOSE_INP(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);

      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L(TMP[0]));

         /* initial xors */
         TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L(TMP[0]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT(TMP[1],TMP[2],TMP[3],TMP[4], TMP[0]);
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut), TMP[4]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[3]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[2]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[1]);
   }

   len -= processedLen;
   if(len)
      processedLen += cpSMS4_ECB_aesni_tail(pOut, pInp, len, pRKey);

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   return processedLen;
}

/*
// 8*MBS_SMS4 processing
*/
static
int cpSMS4_ECB_aesni_x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey)
{
   __ALIGN16 __m128i TMP[10];
   /*
      TMP[0] = T
      TMP[1] = U
      TMP[2] = K0
      TMP[3] = K1
      TMP[4] = K2
      TMP[5] = K3
      TMP[6] = P0
      TMP[7] = P1
      TMP[8] = P2
      TMP[9] = P3
   */

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
      TRANSPOSE_INP(TMP[2],TMP[3],TMP[4],TMP[5], TMP[0]);

      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TRANSPOSE_INP(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[0]), L(TMP[0]));
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[1]), L(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[6]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L(TMP[0]));
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[1]), L(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[6]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L(TMP[0]));
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[1]), L(TMP[1]));

         /* initial xors */
         TMP[1] = TMP[0] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
         TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[6]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
         TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
         /* Sbox */
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         /* Sbox done, now L */
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[0]), L(TMP[0]));
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[1]), L(TMP[1]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT(TMP[2],TMP[3],TMP[4],TMP[5], TMP[0]);
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut), TMP[5]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[4]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[3]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[2]);

      TRANSPOSE_OUT(TMP[6],TMP[7],TMP[8],TMP[9], TMP[0]);
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*4), TMP[9]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*5), TMP[8]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*6), TMP[7]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*7), TMP[6]);
   }


   len -= processedLen;
   if(len)
      processedLen += cpSMS4_ECB_aesni_x4(pOut, pInp, len, pRKey);
   
   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   return processedLen;
}

/*
// 12*MBS_SMS4 processing
*/
#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)
#define cpSMS4_ECB_aesni_x12 OWNAPI(cpSMS4_ECB_aesni_x12)
   IPP_OWN_DECL (int, cpSMS4_ECB_aesni_x12, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey))
   IPP_OWN_DEFN (int, cpSMS4_ECB_aesni_x12, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey))
#else
#define cpSMS4_ECB_aesni OWNAPI(cpSMS4_ECB_aesni)
   IPP_OWN_DECL (int, cpSMS4_ECB_aesni, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey))
   IPP_OWN_DEFN (int, cpSMS4_ECB_aesni, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey))
#endif
{
   __ALIGN16 __m128i TMP[15];
   /*
      TMP[ 0] = T
      TMP[ 1] = U
      TMP[ 2] = V
      TMP[ 3] = K0
      TMP[ 4] = K1
      TMP[ 5] = K2
      TMP[ 6] = K3
      TMP[ 7] = P0
      TMP[ 8] = P1
      TMP[ 9] = P2
      TMP[10] = P3
      TMP[11] = Q0
      TMP[12] = Q1
      TMP[13] = Q2
      TMP[14] = Q3
   */

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
      TRANSPOSE_INP(TMP[3],TMP[4],TMP[5],TMP[6], TMP[0]);

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L(TMP[0]));
         TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[1]), L(TMP[1]));
         TMP[11] = _mm_xor_si128(_mm_xor_si128(TMP[11], TMP[2]), L(TMP[2]));

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L(TMP[0]));
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[1]), L(TMP[1]));
         TMP[12] = _mm_xor_si128(_mm_xor_si128(TMP[12], TMP[2]), L(TMP[2]));

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[0]), L(TMP[0]));
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[1]), L(TMP[1]));
         TMP[13] = _mm_xor_si128(_mm_xor_si128(TMP[13], TMP[2]), L(TMP[2]));

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
         TMP[0] = sBox(TMP[0]);
         TMP[1] = sBox(TMP[1]);
         TMP[2] = sBox(TMP[2]);
         /* Sbox done, now L */
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[0]), L(TMP[0]));
         TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[1]), L(TMP[1]));
         TMP[14] = _mm_xor_si128(_mm_xor_si128(TMP[14], TMP[2]), L(TMP[2]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT(TMP[3],TMP[4],TMP[5],TMP[6], TMP[0]);
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut), TMP[6]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4), TMP[5]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2), TMP[4]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3), TMP[3]);

      TRANSPOSE_OUT(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[9] = _mm_shuffle_epi8(TMP[9], M128(swapBytes));
      TMP[8] = _mm_shuffle_epi8(TMP[8], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*4), TMP[10]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*5), TMP[9]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*6), TMP[8]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*7), TMP[7]);

      TRANSPOSE_OUT(TMP[11],TMP[12],TMP[13],TMP[14], TMP[0]);
      TMP[14] = _mm_shuffle_epi8(TMP[14], M128(swapBytes));
      TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*8), TMP[14]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*9), TMP[13]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*10), TMP[12]);
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*11), TMP[11]);
   }

   len -= processedLen;
   if(len)
      processedLen += cpSMS4_ECB_aesni_x8(pOut, pInp, len, pRKey);
   
   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   return processedLen;
}

#endif /* _IPP_P8, _IPP32E_Y8 */
