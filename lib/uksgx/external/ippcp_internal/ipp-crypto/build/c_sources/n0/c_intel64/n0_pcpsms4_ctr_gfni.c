/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
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
//     cpSMS4_CTR_gfni512()
//     cpSMS4_CTR_gfni512x48()
//     cpSMS4_CTR_gfni512x32()
//     cpSMS4_CTR_gfni512x16()
//     cpSMS4_CTR_gfni128x12()
//     cpSMS4_CTR_gfni128x8()
//     cpSMS4_CTR_gfni128x4()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

#if (_IPP32E>=_IPP32E_K1)

#if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)

#include "pcpsms4_gfni.h"

static __ALIGN32 Ipp8u endiannes_swap[] = {12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3,
                                           12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3,
                                           12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3,
                                           12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3};

static __ALIGN32 Ipp8u endiannes[] = {15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0,
                                      15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0,
                                      15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0,
                                      15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0};

static __ALIGN16 Ipp8u  first_inc[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static __ALIGN16 Ipp8u  next_inc[] =  {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static __ALIGN16 Ipp8u one128[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

__INLINE __m512i inc512(__m512i x, Ipp8u* increment)
{
   __m512i t = _mm512_add_epi64(x,  M512(increment));
   __mmask8 carryMask = _mm512_cmplt_epu64_mask(t, x);
   carryMask = (__mmask8)(carryMask << 1);
   t = _mm512_add_epi64(t, _mm512_mask_set1_epi64(_mm512_setzero_si512(), carryMask, 1));

   return t;
}

__INLINE __m128i inc128(__m128i x)
{
   __m128i t = _mm_add_epi64(x,  M128(one128));
   x = _mm_cmpeq_epi64(t,  _mm_setzero_si128());
   t = _mm_sub_epi64(t, _mm_slli_si128(x, sizeof(Ipp64u)));
   return t;
}

static
int cpSMS4_CTR_gfni512x48(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr);
static
int cpSMS4_CTR_gfni512x32(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr);
static
int cpSMS4_CTR_gfni512x16(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr);
static 
int cpSMS4_CTR_gfni128x12(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr);
static 
int cpSMS4_CTR_gfni128x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr);
static
int cpSMS4_ECB_gfni128x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr);

/*
// 64*MBS_SMS4 bytes processing
*/

IPP_OWN_DEFN (int, cpSMS4_CTR_gfni512, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr))
{
   int processedLen = len - (len % (64 * MBS_SMS4));
   int n;

   if(processedLen){

      __ALIGN16 __m512i TMP[23];

      // TMP[20] - ctr
      // TMP[21] - ctrMask
      // TMP[22] - ctrUnch

      TMP[20]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtr));
      TMP[21]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtrMask)); 

      /* read string counter and convert to numerical */
      TMP[20]  = _mm512_shuffle_epi8(TMP[20], M512(endiannes));

      /* read string mask and convert to numerical */
      TMP[21]  = _mm512_shuffle_epi8(TMP[21], M512(endiannes));

      /* upchanged counter bits */
      TMP[22] = _mm512_andnot_si512(TMP[21], TMP[20]);
      
      /* first incremention */
      TMP[20] = inc512(TMP[20], first_inc);
      
      TMP[20] = _mm512_and_si512(TMP[21], TMP[20]);

      for (n = 0; n < processedLen; n += (64 * MBS_SMS4), pInp += (64 * MBS_SMS4), pOut += (64 * MBS_SMS4)) {
         int itr; 

         TMP[0]  = TMP[20];
         TMP[1]  = inc512(TMP[0], next_inc);
         TMP[2]  = inc512(TMP[1], next_inc);
         TMP[3]  = inc512(TMP[2], next_inc);
         TMP[20] = inc512(TMP[3], next_inc);    

         TMP[0] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[0], TMP[21]));
         TMP[1] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[1], TMP[21]));
         TMP[2] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[2], TMP[21]));
         TMP[3] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[3], TMP[21]));

         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
         TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], TMP[0], TMP[1], TMP[2], TMP[3]);
         
         TMP[0]  = TMP[20];
         TMP[1]  = inc512(TMP[0], next_inc);
         TMP[2]  = inc512(TMP[1], next_inc);
         TMP[3]  = inc512(TMP[2], next_inc);
         TMP[20] = inc512(TMP[3], next_inc);    

         TMP[0] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[0], TMP[21]));
         TMP[1] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[1], TMP[21]));
         TMP[2] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[2], TMP[21]));
         TMP[3] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[3], TMP[21]));

         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
         TRANSPOSE_INP_512(TMP[8], TMP[9], TMP[10], TMP[11], TMP[0], TMP[1], TMP[2], TMP[3]);

         TMP[0]  = TMP[20];
         TMP[1]  = inc512(TMP[0], next_inc);
         TMP[2]  = inc512(TMP[1], next_inc);
         TMP[3]  = inc512(TMP[2], next_inc);
         TMP[20] = inc512(TMP[3], next_inc);    

         TMP[0] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[0], TMP[21]));
         TMP[1] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[1], TMP[21]));
         TMP[2] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[2], TMP[21]));
         TMP[3] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[3], TMP[21]));

         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
         TRANSPOSE_INP_512(TMP[12], TMP[13], TMP[14], TMP[15], TMP[0], TMP[1], TMP[2], TMP[3]);

         TMP[0]  = TMP[20];
         TMP[1]  = inc512(TMP[0], next_inc);
         TMP[2]  = inc512(TMP[1], next_inc);
         TMP[3]  = inc512(TMP[2], next_inc);
         TMP[20] = inc512(TMP[3], next_inc);    

         TMP[0] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[0], TMP[21]));
         TMP[1] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[1], TMP[21]));
         TMP[2] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[2], TMP[21]));
         TMP[3] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[3], TMP[21]));

         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
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
         _mm512_storeu_si512((__m512i*)(pOut),                 _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4),  _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8),  _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12))));

         TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 16))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 20))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 24))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 28))));

         TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12], TMP[13], TMP[14], TMP[15]);
         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 32), _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 32))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 36), _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 36))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 40), _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 40))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 44), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 44))));

         TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[16], TMP[17], TMP[18], TMP[19]);
         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 48), _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 48))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 52), _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 52))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 56), _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 56))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 60), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 60))));

      }

      /* Save counter */
      TMP[20] = _mm512_xor_si512(TMP[22], _mm512_and_si512(TMP[20], TMP[21]));
      TMP[20] = _mm512_shuffle_epi8(TMP[20],  M512(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, _mm512_castsi512_si128(TMP[20]));

      /* clear secret data */
      for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
         TMP[i] = _mm512_xor_si512(TMP[i], TMP[i]);
      }

   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_CTR_gfni512x48(pOut, pInp, len, pRKey, pCtrMask, pCtr);

   return processedLen;
}

/*
// 48*MBS_SMS4 bytes processing
*/

static
int cpSMS4_CTR_gfni512x48(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr)
{
   int processedLen = len - (len % (48 * MBS_SMS4));
   int n;

   if(processedLen){

      __ALIGN16 __m512i TMP[19];

      // TMP[16] - ctr
      // TMP[17] - ctrMask
      // TMP[18] - ctrUnch

      TMP[16]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtr));
      TMP[17]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtrMask)); 

      /* read string counter and convert to numerical */
      TMP[16]  = _mm512_shuffle_epi8(TMP[16], M512(endiannes));

      /* read string mask and convert to numerical */
      TMP[17]  = _mm512_shuffle_epi8(TMP[17], M512(endiannes));

      /* upchanged counter bits */
      TMP[18] = _mm512_andnot_si512(TMP[17], TMP[16]);
      
      /* first incremention */
      TMP[16] = inc512(TMP[16], first_inc);
      
      TMP[16] = _mm512_and_si512(TMP[17], TMP[16]);

      for (n = 0; n < processedLen; n += (48 * MBS_SMS4), pInp += (48 * MBS_SMS4), pOut += (48 * MBS_SMS4)) {
         int itr; 

            TMP[0]  = TMP[16];
            TMP[1]  = inc512(TMP[0], next_inc);
            TMP[2]  = inc512(TMP[1], next_inc);
            TMP[3]  = inc512(TMP[2], next_inc);
            TMP[16] = inc512(TMP[3], next_inc);    

            TMP[0] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[0], TMP[17]));
            TMP[1] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[1], TMP[17]));
            TMP[2] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[2], TMP[17]));
            TMP[3] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[3], TMP[17]));

            TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
            TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
            TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
            TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
            TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], TMP[0], TMP[1], TMP[2], TMP[3]);
            
            TMP[0]  = TMP[16];
            TMP[1]  = inc512(TMP[0], next_inc);
            TMP[2]  = inc512(TMP[1], next_inc);
            TMP[3]  = inc512(TMP[2], next_inc);
            TMP[16] = inc512(TMP[3], next_inc);    

            TMP[0] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[0], TMP[17]));
            TMP[1] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[1], TMP[17]));
            TMP[2] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[2], TMP[17]));
            TMP[3] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[3], TMP[17]));

            TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
            TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
            TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
            TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
            TRANSPOSE_INP_512(TMP[8], TMP[9], TMP[10], TMP[11], TMP[0], TMP[1], TMP[2], TMP[3]);

            TMP[0]  = TMP[16];
            TMP[1]  = inc512(TMP[0], next_inc);
            TMP[2]  = inc512(TMP[1], next_inc);
            TMP[3]  = inc512(TMP[2], next_inc);
            TMP[16] = inc512(TMP[3], next_inc);    

            TMP[0] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[0], TMP[17]));
            TMP[1] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[1], TMP[17]));
            TMP[2] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[2], TMP[17]));
            TMP[3] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[3], TMP[17]));

            TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
            TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
            TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
            TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
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
            /* Sbox */
            TMP[0] = sBox512(TMP[0]);
            TMP[1] = sBox512(TMP[1]);
            TMP[2] = sBox512(TMP[2]);
            /* Sbox done, now L */
            TMP[6] = _mm512_xor_si512(_mm512_xor_si512(TMP[6], TMP[0]), L512(TMP[0]));
            TMP[10] = _mm512_xor_si512(_mm512_xor_si512(TMP[10], TMP[1]), L512(TMP[1]));
            TMP[14] = _mm512_xor_si512(_mm512_xor_si512(TMP[14], TMP[2]), L512(TMP[2]));

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
         _mm512_storeu_si512((__m512i*)(pOut),                 _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4),  _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8),  _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12))));

         TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 16))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 20))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 24))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 28))));

         TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12], TMP[13], TMP[14], TMP[15]);
         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 32), _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 32))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 36), _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 36))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 40), _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 40))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 44), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 44))));

      }

      /* Save counter */
      TMP[16] = _mm512_xor_si512(TMP[18], _mm512_and_si512(TMP[16], TMP[17]));
      TMP[16] = _mm512_shuffle_epi8(TMP[16],  M512(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, _mm512_castsi512_si128(TMP[16]));

      /* clear secret data */
      for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
         TMP[i] = _mm512_xor_si512(TMP[i], TMP[i]);
      }

   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_CTR_gfni512x32(pOut, pInp, len, pRKey, pCtrMask, pCtr);

   return processedLen;
}

/*
// 32*MBS_SMS4 bytes processing
*/

static
int cpSMS4_CTR_gfni512x32(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr)
{
   int processedLen = len - (len % (32 * MBS_SMS4));
   int n;

   if(processedLen){

      __ALIGN16 __m512i TMP[15];

      // TMP[12] - ctr
      // TMP[13] - ctrMask
      // TMP[14] - ctrUnch

      TMP[12]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtr));
      TMP[13]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtrMask)); 

      /* read string counter and convert to numerical */
      TMP[12]  = _mm512_shuffle_epi8(TMP[12], M512(endiannes));

      /* read string mask and convert to numerical */
      TMP[13]  = _mm512_shuffle_epi8(TMP[13], M512(endiannes));

      /* upchanged counter bits */
      TMP[14] = _mm512_andnot_si512(TMP[13], TMP[12]);
      
      /* first incremention */
      TMP[12] = inc512(TMP[12], first_inc);
      
      TMP[12] = _mm512_and_si512(TMP[13], TMP[12]);

      for (n = 0; n < processedLen; n += (32 * MBS_SMS4), pInp += (32 * MBS_SMS4), pOut += (32 * MBS_SMS4)) {
         int itr; 

            TMP[0]  = TMP[12];
            TMP[1]  = inc512(TMP[0], next_inc);
            TMP[2]  = inc512(TMP[1], next_inc);
            TMP[3]  = inc512(TMP[2], next_inc);
            TMP[12] = inc512(TMP[3], next_inc);    

            TMP[0] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[0], TMP[13]));
            TMP[1] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[1], TMP[13]));
            TMP[2] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[2], TMP[13]));
            TMP[3] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[3], TMP[13]));

            TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
            TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
            TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
            TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
            TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], TMP[0], TMP[1], TMP[2], TMP[3]);
            
            TMP[0]  = TMP[12];
            TMP[1]  = inc512(TMP[0], next_inc);
            TMP[2]  = inc512(TMP[1], next_inc);
            TMP[3]  = inc512(TMP[2], next_inc);
            TMP[12] = inc512(TMP[3], next_inc);    

            TMP[0] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[0], TMP[13]));
            TMP[1] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[1], TMP[13]));
            TMP[2] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[2], TMP[13]));
            TMP[3] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[3], TMP[13]));

            TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
            TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
            TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
            TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
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
            TMP[3] = TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
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
            TMP[3] = TMP[2] = TMP[1] = TMP[0] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
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
         _mm512_storeu_si512((__m512i*)(pOut),                 _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4),  _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8),  _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12))));

         TRANSPOSE_OUT_512(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8], TMP[9], TMP[10], TMP[11]);
         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(swapBytes));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(swapBytes));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(swapBytes));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(swapBytes));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 16), _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 16))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 20), _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 20))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 24), _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 24))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 28), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 28))));

      }

      /* Save counter */
      TMP[12] = _mm512_xor_si512(TMP[14], _mm512_and_si512(TMP[12], TMP[13]));
      TMP[12] = _mm512_shuffle_epi8(TMP[12],  M512(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, _mm512_castsi512_si128(TMP[12]));

      /* clear secret data */
      for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
         TMP[i] = _mm512_xor_si512(TMP[i], TMP[i]);
      }

   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_CTR_gfni512x16(pOut, pInp, len, pRKey, pCtrMask, pCtr);

   return processedLen;
}

/*
// 16*MBS_SMS4 bytes processing
*/

static
int cpSMS4_CTR_gfni512x16(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr)
{
   int processedLen = len - (len % (16 * MBS_SMS4));
   int n;

   if(processedLen){

      __ALIGN16 __m512i TMP[11];

      // TMP[8]  - ctr
      // TMP[9]  - ctrMask
      // TMP[10] - ctrUnch

      TMP[8]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtr));
      TMP[9]  = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i*)pCtrMask)); 

      /* read string counter and convert to numerical */
      TMP[8]  = _mm512_shuffle_epi8(TMP[8], M512(endiannes));

      /* read string mask and convert to numerical */
      TMP[9]  = _mm512_shuffle_epi8(TMP[9], M512(endiannes));

      /* upchanged counter bits */
      TMP[10] = _mm512_andnot_si512(TMP[9], TMP[8]);
      
      /* first incremention */
      TMP[8] = inc512(TMP[8], first_inc);
      
      TMP[8] = _mm512_and_si512(TMP[9], TMP[8]);

      for (n = 0; n < processedLen; n += (16 * MBS_SMS4), pInp += (16 * MBS_SMS4), pOut += (16 * MBS_SMS4)) {
         int itr; 

         TMP[0] = TMP[8];
         TMP[1] = inc512(TMP[0], next_inc);
         TMP[2] = inc512(TMP[1], next_inc);
         TMP[3] = inc512(TMP[2], next_inc);
         TMP[8] = inc512(TMP[3], next_inc);    

         TMP[0] = _mm512_xor_si512(TMP[10], _mm512_and_si512(TMP[0], TMP[9]));
         TMP[1] = _mm512_xor_si512(TMP[10], _mm512_and_si512(TMP[1], TMP[9]));
         TMP[2] = _mm512_xor_si512(TMP[10], _mm512_and_si512(TMP[2], TMP[9]));
         TMP[3] = _mm512_xor_si512(TMP[10], _mm512_and_si512(TMP[3], TMP[9]));

         TMP[0] = _mm512_shuffle_epi8(TMP[0], M512(endiannes_swap));
         TMP[1] = _mm512_shuffle_epi8(TMP[1], M512(endiannes_swap));
         TMP[2] = _mm512_shuffle_epi8(TMP[2], M512(endiannes_swap));
         TMP[3] = _mm512_shuffle_epi8(TMP[3], M512(endiannes_swap));
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

         _mm512_storeu_si512((__m512i*)(pOut),                 _mm512_xor_si512(TMP[0], _mm512_loadu_si512((__m512i*)(pInp))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 4),  _mm512_xor_si512(TMP[1], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 4))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 8),  _mm512_xor_si512(TMP[2], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 8))));
         _mm512_storeu_si512((__m512i*)(pOut + MBS_SMS4 * 12), _mm512_xor_si512(TMP[3], _mm512_loadu_si512((__m512i*)(pInp + MBS_SMS4 * 12))));
      }

      /* Save counter */
      TMP[8] = _mm512_xor_si512(TMP[10], _mm512_and_si512(TMP[8], TMP[9]));
      TMP[8] = _mm512_shuffle_epi8(TMP[8],  M512(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, _mm512_castsi512_si128(TMP[8]));

      /* clear secret data */
      for (unsigned int i = 0; i < sizeof(TMP) / sizeof(TMP[0]); ++i) {
         TMP[i] = _mm512_xor_si512(TMP[i], TMP[i]);
      }

   }
   
   len -= processedLen;
   if (len)
      processedLen += cpSMS4_CTR_gfni128x12(pOut, pInp, len, pRKey, pCtrMask, pCtr);

   return processedLen;
}

/*
// 12*MBS_SMS4 processing
*/

static 
int cpSMS4_CTR_gfni128x12(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr)
{
   int processedLen = len - (len % (12 * MBS_SMS4));
   int n;

   if(processedLen){
   
      __ALIGN16 __m128i TMP[22];

      // TMP[15] - ctr
      // TMP[16] - ctrMask
      // TMP[17] - ctrUnch

      TMP[15] = _mm_loadu_si128((__m128i*)pCtr);
      TMP[16] = _mm_loadu_si128((__m128i*)pCtrMask);

      TMP[16] = _mm_shuffle_epi8(TMP[16], M128(endiannes));
      TMP[15] = _mm_shuffle_epi8(TMP[15], M128(endiannes));
      TMP[17] = _mm_andnot_si128(TMP[16], TMP[15]);

      for(n=0; n<processedLen; n+=(12*MBS_SMS4), pInp+=(12*MBS_SMS4), pOut+=(12*MBS_SMS4)) {
         int itr;

         TMP[18] = TMP[15];
         TMP[19] = inc128(TMP[18]);
         TMP[20] = inc128(TMP[19]);
         TMP[21] = inc128(TMP[20]);
         TMP[15] = inc128(TMP[21]);

         TMP[18] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[18], TMP[16]));
         TMP[19] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[19], TMP[16]));
         TMP[20] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[20], TMP[16]));
         TMP[21] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[21], TMP[16]));

         TMP[3] = _mm_shuffle_epi8(TMP[18], M128(endiannes_swap));
         TMP[4] = _mm_shuffle_epi8(TMP[19], M128(endiannes_swap));
         TMP[5] = _mm_shuffle_epi8(TMP[20], M128(endiannes_swap));
         TMP[6] = _mm_shuffle_epi8(TMP[21], M128(endiannes_swap));

         TRANSPOSE_INP_128(TMP[3],TMP[4],TMP[5],TMP[6], TMP[0]);

         TMP[18] = TMP[15];
         TMP[19] = inc128(TMP[18]);
         TMP[20] = inc128(TMP[19]);
         TMP[21] = inc128(TMP[20]);
         TMP[15] = inc128(TMP[21]);

         TMP[18] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[18], TMP[16]));
         TMP[19] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[19], TMP[16]));
         TMP[20] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[20], TMP[16]));
         TMP[21] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[21], TMP[16]));

         TMP[7]  = _mm_shuffle_epi8(TMP[18], M128(endiannes_swap));
         TMP[8]  = _mm_shuffle_epi8(TMP[19], M128(endiannes_swap));
         TMP[9]  = _mm_shuffle_epi8(TMP[20], M128(endiannes_swap));
         TMP[10] = _mm_shuffle_epi8(TMP[21], M128(endiannes_swap));

         TRANSPOSE_INP_128(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);

         TMP[18] = TMP[15];
         TMP[19] = inc128(TMP[18]);
         TMP[20] = inc128(TMP[19]);
         TMP[21] = inc128(TMP[20]);
         TMP[15] = inc128(TMP[21]);

         TMP[18] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[18], TMP[16]));
         TMP[19] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[19], TMP[16]));
         TMP[20] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[20], TMP[16]));
         TMP[21] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[21], TMP[16]));

         TMP[11] = _mm_shuffle_epi8(TMP[18], M128(endiannes_swap));
         TMP[12] = _mm_shuffle_epi8(TMP[19], M128(endiannes_swap));
         TMP[13] = _mm_shuffle_epi8(TMP[20], M128(endiannes_swap));
         TMP[14] = _mm_shuffle_epi8(TMP[21], M128(endiannes_swap));

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

         TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
         TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
         TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
         TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));

         _mm_storeu_si128((__m128i*)(pOut),           _mm_xor_si128(TMP[6], _mm_loadu_si128((__m128i*)(pInp))));
         _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4),  _mm_xor_si128(TMP[5], _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 2), _mm_xor_si128(TMP[4], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 2))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 3), _mm_xor_si128(TMP[3], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 3))));

         TRANSPOSE_OUT_128(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);

         TMP[7]  = _mm_shuffle_epi8(TMP[7],  M128(swapBytes));
         TMP[8]  = _mm_shuffle_epi8(TMP[8],  M128(swapBytes));
         TMP[9]  = _mm_shuffle_epi8(TMP[9],  M128(swapBytes));
         TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));

         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 4), _mm_xor_si128(TMP[10], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 4))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 5), _mm_xor_si128(TMP[9],  _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 5))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 6), _mm_xor_si128(TMP[8],  _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 6))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 7), _mm_xor_si128(TMP[7],  _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 7))));
        

         TRANSPOSE_OUT_128(TMP[11],TMP[12],TMP[13],TMP[14], TMP[0]);

         TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
         TMP[12] = _mm_shuffle_epi8(TMP[12], M128(swapBytes));
         TMP[13] = _mm_shuffle_epi8(TMP[13], M128(swapBytes));
         TMP[14] = _mm_shuffle_epi8(TMP[14], M128(swapBytes));

         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 8), _mm_xor_si128(TMP[14], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 8))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 9), _mm_xor_si128(TMP[13], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 9))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 10), _mm_xor_si128(TMP[12], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 10))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 11), _mm_xor_si128(TMP[11], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 11))));

      }

      TMP[15] = _mm_xor_si128(TMP[17], _mm_and_si128(TMP[15], TMP[16]));
      TMP[15] = _mm_shuffle_epi8(TMP[15], M128(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, TMP[15]);

      /* clear secret data */
      for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
         TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
      }
   }

   len -= processedLen;
   if(len)
      processedLen += cpSMS4_CTR_gfni128x8(pOut, pInp, len, pRKey, pCtrMask, pCtr);

   return processedLen;
}

/*
// 8*MBS_SMS4 processing
*/

static 
int cpSMS4_CTR_gfni128x8(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr)
{
   int processedLen = len - (len % (8 * MBS_SMS4));
   int n;

   if(processedLen){
   
      __ALIGN16 __m128i TMP[18];

      // TMP[11] - ctr
      // TMP[12] - ctrMask
      // TMP[13] - ctrUnch

      TMP[11] = _mm_loadu_si128((__m128i*)pCtr);
      TMP[12] = _mm_loadu_si128((__m128i*)pCtrMask);

      TMP[12] = _mm_shuffle_epi8(TMP[12], M128(endiannes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(endiannes));
      TMP[13] = _mm_andnot_si128(TMP[12], TMP[11]);

      for(n=0; n<processedLen; n+=(8*MBS_SMS4), pInp+=(8*MBS_SMS4), pOut+=(8*MBS_SMS4)) {
         int itr;

         TMP[14] = TMP[11];
         TMP[15] = inc128(TMP[14]);
         TMP[16] = inc128(TMP[15]);
         TMP[17] = inc128(TMP[16]);
         TMP[11] = inc128(TMP[17]);

         TMP[14] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[14], TMP[12]));
         TMP[15] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[15], TMP[12]));
         TMP[16] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[16], TMP[12]));
         TMP[17] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[17], TMP[12]));

         TMP[3] = _mm_shuffle_epi8(TMP[14], M128(endiannes_swap));
         TMP[4] = _mm_shuffle_epi8(TMP[15], M128(endiannes_swap));
         TMP[5] = _mm_shuffle_epi8(TMP[16], M128(endiannes_swap));
         TMP[6] = _mm_shuffle_epi8(TMP[17], M128(endiannes_swap));

         TRANSPOSE_INP_128(TMP[3],TMP[4],TMP[5],TMP[6], TMP[0]);

         TMP[14] = TMP[11];
         TMP[15] = inc128(TMP[14]);
         TMP[16] = inc128(TMP[15]);
         TMP[17] = inc128(TMP[16]);
         TMP[11] = inc128(TMP[17]);

         TMP[14] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[14], TMP[12]));
         TMP[15] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[15], TMP[12]));
         TMP[16] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[16], TMP[12]));
         TMP[17] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[17], TMP[12]));

         TMP[7]  = _mm_shuffle_epi8(TMP[14], M128(endiannes_swap));
         TMP[8]  = _mm_shuffle_epi8(TMP[15], M128(endiannes_swap));
         TMP[9]  = _mm_shuffle_epi8(TMP[16], M128(endiannes_swap));
         TMP[10] = _mm_shuffle_epi8(TMP[17], M128(endiannes_swap));

         TRANSPOSE_INP_128(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);

         for(itr=0; itr<8; itr++, pRKey+=4) {
            /* initial xors */
            TMP[1] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
            TMP[0] = TMP[1];
            TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
            TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
            TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
            /* Sbox */
            TMP[0] = sBox128(TMP[0]);
            TMP[1] = sBox128(TMP[1]);
            /* Sbox done, now L */
            TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L128(TMP[0]));
            TMP[7] = _mm_xor_si128(_mm_xor_si128(TMP[7], TMP[1]), L128(TMP[1]));

            /* initial xors */
            TMP[1] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
            TMP[0] = TMP[1];
            TMP[0] = _mm_xor_si128(TMP[0], TMP[5]);
            TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
            TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[9]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
            /* Sbox */
            TMP[0] = sBox128(TMP[0]);
            TMP[1] = sBox128(TMP[1]);
            /* Sbox done, now L */
            TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L128(TMP[0]));
            TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[1]), L128(TMP[1]));

            /* initial xors */
            TMP[1] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
            TMP[0] = TMP[1];
            TMP[0] = _mm_xor_si128(TMP[0], TMP[6]);
            TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
            TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[10]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[7]);
            TMP[1] = _mm_xor_si128(TMP[1], TMP[8]);
            /* Sbox */
            TMP[0] = sBox128(TMP[0]);
            TMP[1] = sBox128(TMP[1]);
            /* Sbox done, now L */
            TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[0]), L128(TMP[0]));
            TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[1]), L128(TMP[1]));

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
            /* Sbox */
            TMP[0] = sBox128(TMP[0]);
            TMP[1] = sBox128(TMP[1]);
            /* Sbox done, now L */
            TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[0]), L128(TMP[0]));
            TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[1]), L128(TMP[1]));
         }

         pRKey -= 32;

         TRANSPOSE_OUT_128(TMP[3],TMP[4],TMP[5],TMP[6], TMP[0]);

         TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
         TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
         TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
         TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));

         _mm_storeu_si128((__m128i*)(pOut),           _mm_xor_si128(TMP[6], _mm_loadu_si128((__m128i*)(pInp))));
         _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4),  _mm_xor_si128(TMP[5], _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 2), _mm_xor_si128(TMP[4], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 2))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 3), _mm_xor_si128(TMP[3], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 3))));

         TRANSPOSE_OUT_128(TMP[7],TMP[8],TMP[9],TMP[10], TMP[0]);

         TMP[7]  = _mm_shuffle_epi8(TMP[7],  M128(swapBytes));
         TMP[8]  = _mm_shuffle_epi8(TMP[8],  M128(swapBytes));
         TMP[9]  = _mm_shuffle_epi8(TMP[9],  M128(swapBytes));
         TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));

         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 4), _mm_xor_si128(TMP[10], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 4))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 5), _mm_xor_si128(TMP[9],  _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 5))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 6), _mm_xor_si128(TMP[8],  _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 6))));
         _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 7), _mm_xor_si128(TMP[7],  _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 7))));

      }

      TMP[11] = _mm_xor_si128(TMP[13], _mm_and_si128(TMP[11], TMP[12]));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, TMP[11]);

      /* clear secret data */
      for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
         TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
      }
   }

   len -= processedLen;
   if(len)
      processedLen += cpSMS4_ECB_gfni128x4(pOut, pInp, len, pRKey, pCtrMask, pCtr);

   return processedLen;
}

/*
// 4*MBS_SMS4 processing
*/

static
int cpSMS4_ECB_gfni128x4(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr)
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
      _mm_storeu_si128((__m128i*)(pOut),           _mm_xor_si128(TMP[4], _mm_loadu_si128((__m128i*)(pInp))));
      _mm_storeu_si128((__m128i*)(pOut+MBS_SMS4),  _mm_xor_si128(TMP[3], _mm_loadu_si128((__m128i*)(pInp+MBS_SMS4))));
      _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 2), _mm_xor_si128(TMP[2], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 2))));
      _mm_storeu_si128((__m128i*)(pOut + MBS_SMS4 * 3), _mm_xor_si128(TMP[1], _mm_loadu_si128((__m128i*)(pInp + MBS_SMS4 * 3))));
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

#endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */

#endif /* _IPP32E>=_IPP32E_K1 */
