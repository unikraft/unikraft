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
//     cpSMS4_ECB_aesni_x1()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)

#include "pcpsms4_y8cn.h"

/*
// 1*MBS_SMS4 processing
*/

IPP_OWN_DEFN (void, cpSMS4_ECB_aesni_x1, (Ipp8u* pOut, const Ipp8u* pInp, const Ipp32u* pRKey))
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

   TMP[1] = _mm_shuffle_epi8( _mm_cvtsi32_si128(((Ipp32s*)pInp)[0]), M128(swapBytes));
   TMP[2] = _mm_shuffle_epi8( _mm_cvtsi32_si128(((Ipp32s*)pInp)[1]), M128(swapBytes));
   TMP[3] = _mm_shuffle_epi8( _mm_cvtsi32_si128(((Ipp32s*)pInp)[2]), M128(swapBytes));
   TMP[4] = _mm_shuffle_epi8( _mm_cvtsi32_si128(((Ipp32s*)pInp)[3]), M128(swapBytes));

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
      TMP[0] = _mm_shuffle_epi32(TMP[5], 0xAA); /* broadcast(key4 TMP[2]) */
      TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
      /* Sbox */
      TMP[0] = sBox(TMP[0]);
      /* Sbox done, now L */
      TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L(TMP[0]));

      /* initial xors */
      TMP[0] = _mm_shuffle_epi32(TMP[5], 0xFF); /* broadcast(key4 TMP[3]) */
      TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
      /* Sbox */
      TMP[0] = sBox(TMP[0]);
      /* Sbox done, now L */
      TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L(TMP[0]));
   }

   ((Ipp32u*)(pOut))[0] = (Ipp32u)_mm_cvtsi128_si32(_mm_shuffle_epi8(TMP[4], M128(swapBytes)));
   ((Ipp32u*)(pOut))[1] = (Ipp32u)_mm_cvtsi128_si32(_mm_shuffle_epi8(TMP[3], M128(swapBytes)));
   ((Ipp32u*)(pOut))[2] = (Ipp32u)_mm_cvtsi128_si32(_mm_shuffle_epi8(TMP[2], M128(swapBytes)));
   ((Ipp32u*)(pOut))[3] = (Ipp32u)_mm_cvtsi128_si32(_mm_shuffle_epi8(TMP[1], M128(swapBytes)));

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }
}

#endif /* _IPP_P8, _IPP32E_Y8 */
