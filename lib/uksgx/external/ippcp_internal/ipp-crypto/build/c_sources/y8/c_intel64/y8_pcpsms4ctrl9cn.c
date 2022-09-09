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
//     SMS4 EBC decryption
// 
//  Contents:
//     cpSMS4_CTR_aesni()
// 
// 
*/

#include "pcpsms4.h"
#include "owndefs.h"
#include "owncp.h"
#include "pcptool.h"

#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)

#include "pcpsms4_l9cn.h"

static __ALIGN32 Ipp8u endiannes_swap[] = {12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3,
                                           12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3};

static __ALIGN32 Ipp8u endiannes[] = {15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0,
                                      15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0};

static __ALIGN32 Ipp8u two256[] = {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static __ALIGN16 Ipp8u one256[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

__INLINE __m128i inc128(__m128i x)
{
   __m128i t = _mm_add_epi64(x,  M128(one256));
   x = _mm_cmpeq_epi64(t,  _mm_setzero_si128());
   t = _mm_sub_epi64(t, _mm_slli_si128(x, sizeof(Ipp64u)));
   return t;
}

__INLINE __m256i inc256(__m256i x)
{
   __m256i t = _mm256_add_epi64(x,  M256(two256));
   x = _mm256_cmpeq_epi64(t,  _mm256_setzero_si256());
   t = _mm256_sub_epi64(t, _mm256_slli_si256(x, sizeof(Ipp64u)));
   return t;
}
__INLINE __m256i inc256_2(__m256i x)
{
   __m256i t = _mm256_add_epi64(x,  M256(one256));
   x = _mm256_cmpeq_epi64(t,  _mm256_setzero_si256());
   t = _mm256_sub_epi64(t, _mm256_slli_si256(x, sizeof(Ipp64u)));

   t = _mm256_add_epi64(t,  M256(one256));
   x = _mm256_cmpeq_epi64(t,  _mm256_setzero_si256());
   t = _mm256_sub_epi64(t, _mm256_slli_si256(x, sizeof(Ipp64u)));
   return t;
}

IPP_OWN_DEFN (int, cpSMS4_CTR_aesni, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr))
{
   int processedLen = len & -(16*MBS_SMS4);

   if(processedLen) {
      int n;

      __ALIGN16 __m256i TMP[16];
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
      TMP[12] = ctr
      TMP[13] = mask
      TMP[14] = unch
   */

      /* read string counter and convert to numerical */
      TMP[12]  = _mm256_shuffle_epi8(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pCtr)), M256(endiannes));

      /* read string mask and convert to numerical */
      TMP[13] = _mm256_shuffle_epi8(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pCtrMask)), M256(endiannes));

      /* upchanged counter bits */
      TMP[14] = _mm256_andnot_si256(TMP[13], TMP[12]);

      TMP[12] = _mm256_inserti128_si256(TMP[12],  inc128(_mm256_castsi256_si128(TMP[12])),  1);
      TMP[13]= _mm256_inserti128_si256(TMP[13], _mm256_castsi256_si128(TMP[13]), 1);
      TMP[14]= _mm256_inserti128_si256(TMP[14], _mm256_castsi256_si128(TMP[14]), 1);
      TMP[12] = _mm256_and_si256(TMP[12], TMP[13]);

      for(n=0; n<processedLen; n+=(16*MBS_SMS4), pInp+=(16*MBS_SMS4), pOut+=(16*MBS_SMS4)) {
         int itr;

         TMP[0] = TMP[12];
         TMP[1] = inc256_2(TMP[0]);
         TMP[2] = inc256_2(TMP[1]);
         TMP[3] = inc256_2(TMP[2]);
         TMP[12] = inc256_2(TMP[3]);
         TMP[0] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[0], TMP[13]));
         TMP[1] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[1], TMP[13]));
         TMP[2] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[2], TMP[13]));
         TMP[3] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[3], TMP[13]));
         TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(endiannes_swap));
         TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(endiannes_swap));
         TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(endiannes_swap));
         TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(endiannes_swap));
         TRANSPOSE_INP(TMP[4],TMP[5],TMP[6],TMP[7], TMP[0],TMP[1],TMP[2],TMP[3]);

         TMP[0] = TMP[12];
         TMP[1] = inc256(TMP[0]);
         TMP[2] = inc256(TMP[1]);
         TMP[3] = inc256(TMP[2]);
         TMP[12] = inc256(TMP[3]);
         TMP[0] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[0], TMP[13]));
         TMP[1] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[1], TMP[13]));
         TMP[2] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[2], TMP[13]));
         TMP[3] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[3], TMP[13]));
         TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(endiannes_swap));
         TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(endiannes_swap));
         TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(endiannes_swap));
         TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(endiannes_swap));
         TRANSPOSE_INP(TMP[8],TMP[9],TMP[10],TMP[11], TMP[0],TMP[1],TMP[2],TMP[3]);

         for(itr=0; itr<8; itr++, pRKey+=4) {
            /* initial xors */
            TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[0]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[5]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[6]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[7]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[9]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[10]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[11]);
            /* Sbox */
            TMP[0] = sBox(TMP[0]);
            TMP[1] = sBox(TMP[1]);
            /* Sbox done, now L */
            TMP[4] = _mm256_xor_si256(_mm256_xor_si256(TMP[4], TMP[0]), L(TMP[0]));
            TMP[8] = _mm256_xor_si256(_mm256_xor_si256(TMP[8], TMP[1]), L(TMP[1]));

            /* initial xors */
            TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[1]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[6]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[7]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[4]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[10]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[11]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[8]);
            /* Sbox */
            TMP[0] = sBox(TMP[0]);
            TMP[1] = sBox(TMP[1]);
            /* Sbox done, now L */
            TMP[5] = _mm256_xor_si256(_mm256_xor_si256(TMP[5], TMP[0]), L(TMP[0]));
            TMP[9] = _mm256_xor_si256(_mm256_xor_si256(TMP[9], TMP[1]), L(TMP[1]));

            /* initial xors */
            TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[2]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[7]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[4]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[5]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[11]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[8]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[9]);
            /* Sbox */
            TMP[0] = sBox(TMP[0]);
            TMP[1] = sBox(TMP[1]);
            /* Sbox done, now L */
            TMP[6] = _mm256_xor_si256(_mm256_xor_si256(TMP[6], TMP[0]), L(TMP[0]));
            TMP[10] = _mm256_xor_si256(_mm256_xor_si256(TMP[10], TMP[1]), L(TMP[1]));

            /* initial xors */
            TMP[1] = TMP[0] = _mm256_set1_epi32((Ipp32s)pRKey[3]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[4]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[5]);
            TMP[0] = _mm256_xor_si256(TMP[0], TMP[6]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[8]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[9]);
            TMP[1] = _mm256_xor_si256(TMP[1], TMP[10]);
            /* Sbox */
            TMP[0] = sBox(TMP[0]);
            TMP[1] = sBox(TMP[1]);
            /* Sbox done, now L */
            TMP[7] = _mm256_xor_si256(_mm256_xor_si256(TMP[7], TMP[0]), L(TMP[0]));
            TMP[11] = _mm256_xor_si256(_mm256_xor_si256(TMP[11], TMP[1]), L(TMP[1]));
         }

         pRKey -= 32;

         TRANSPOSE_OUT(TMP[0],TMP[1],TMP[2],TMP[3], TMP[4],TMP[5],TMP[6],TMP[7]);
         TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
         TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
         TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
         TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
         _mm256_storeu_si256((__m256i*)(pOut),            _mm256_xor_si256(TMP[0], _mm256_loadu_si256((__m256i*)(pInp))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*2), _mm256_xor_si256(TMP[1], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*2))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*4), _mm256_xor_si256(TMP[2], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*4))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*6), _mm256_xor_si256(TMP[3], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*6))));

         TRANSPOSE_OUT(TMP[0],TMP[1],TMP[2],TMP[3], TMP[8],TMP[9],TMP[10],TMP[11]);
         TMP[0] = _mm256_shuffle_epi8(TMP[0], M256(swapBytes));
         TMP[1] = _mm256_shuffle_epi8(TMP[1], M256(swapBytes));
         TMP[2] = _mm256_shuffle_epi8(TMP[2], M256(swapBytes));
         TMP[3] = _mm256_shuffle_epi8(TMP[3], M256(swapBytes));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*8),  _mm256_xor_si256(TMP[0], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*8))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*10), _mm256_xor_si256(TMP[1], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*10))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*12), _mm256_xor_si256(TMP[2], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*12))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*14), _mm256_xor_si256(TMP[3], _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*14))));
      }

      TMP[12] = _mm256_xor_si256(TMP[14], _mm256_and_si256(TMP[12], TMP[13]));
      TMP[12] = _mm256_shuffle_epi8(TMP[12], M256(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, _mm256_castsi256_si128(TMP[12]));

      /* clear secret data */
      for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
         TMP[i] = _mm256_xor_si256(TMP[i],TMP[i]);
      }
   }

   return processedLen + cpSMS4_CTR_aesni_x4(pOut, pInp, len-processedLen, pRKey, pCtrMask, pCtr);
}

#endif /* _IPP_G9, _IPP32E_L9 */
