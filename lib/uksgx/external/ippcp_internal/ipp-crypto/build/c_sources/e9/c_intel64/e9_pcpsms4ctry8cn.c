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
//     SMS4 CTR decryption
// 
//  Contents:
//     cpSMS4_CTR_aesni_x4()
// 
// 
*/

#include "pcpsms4.h"
#include "owndefs.h"
#include "owncp.h"
#include "pcptool.h"

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)

#include "pcpsms4_y8cn.h"

static __ALIGN16 Ipp8u one128[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static __ALIGN16 Ipp8u endiannes[] = {15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0};
static __ALIGN16 Ipp8u endiannes_swap[] = {12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3};

__INLINE __m128i inc128(__m128i x)
{
   __m128i t = _mm_add_epi64(x,  M128(one128));
   x = _mm_cmpeq_epi64(t,  _mm_setzero_si128());
   t = _mm_sub_epi64(t, _mm_slli_si128(x, sizeof(Ipp64u)));
   return t;
}

#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)
IPP_OWN_DEFN (int, cpSMS4_CTR_aesni_x4, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr))
#else
IPP_OWN_DEFN (int, cpSMS4_CTR_aesni, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr))
#endif
{
   __ALIGN16 __m128i TMP[8];
   /*
      TMP[ 0] = T
      TMP[ 1] = K0
      TMP[ 2] = K1
      TMP[ 3] = K2
      TMP[ 4] = K3
      TMP[ 5] = ctrUnchanged
      TMP[ 6] = ctrMask
      TMP[ 7] = ctr
   */

   int processedLen = len & -(4*MBS_SMS4);
   int n;

   TMP[6] = _mm_loadu_si128((__m128i*)pCtrMask);
   TMP[7] = _mm_loadu_si128((__m128i*)pCtr);

   TMP[6] = _mm_shuffle_epi8(TMP[6], M128(endiannes));
   TMP[7] = _mm_shuffle_epi8(TMP[7], M128(endiannes));
   TMP[5] = _mm_andnot_si128(TMP[6], TMP[7]);

   for(n=0; n<processedLen; n+=(4*MBS_SMS4), pInp+=(4*MBS_SMS4), pOut+=(4*MBS_SMS4)) {
      int itr;
      TMP[1] = TMP[7];
      TMP[2] = inc128(TMP[1]);
      TMP[3] = inc128(TMP[2]);
      TMP[4] = inc128(TMP[3]);
      TMP[7] = inc128(TMP[4]);

      TMP[1] = _mm_xor_si128(TMP[5], _mm_and_si128(TMP[1], TMP[6]));
      TMP[2] = _mm_xor_si128(TMP[5], _mm_and_si128(TMP[2], TMP[6]));
      TMP[3] = _mm_xor_si128(TMP[5], _mm_and_si128(TMP[3], TMP[6]));
      TMP[4] = _mm_xor_si128(TMP[5], _mm_and_si128(TMP[4], TMP[6]));

      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(endiannes_swap));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(endiannes_swap));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(endiannes_swap));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(endiannes_swap));
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
      _mm_storeu_si128((__m128i*)(pOut),           _mm_xor_si128(TMP[4], _mm_loadu_si128((__m128i*)(pInp))));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4),  _mm_xor_si128(TMP[3], _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4))));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*2),_mm_xor_si128(TMP[2], _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*2))));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4*3),_mm_xor_si128(TMP[1], _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4*3))));
   }

   TMP[7] = _mm_xor_si128(TMP[5], _mm_and_si128(TMP[7], TMP[6]));
   TMP[7] = _mm_shuffle_epi8(TMP[7], M128(endiannes));
   _mm_storeu_si128((__m128i*)pCtr, TMP[7]);

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }

   return processedLen;
}

#endif /* _IPP_P8, _IPP32E_Y8 */
