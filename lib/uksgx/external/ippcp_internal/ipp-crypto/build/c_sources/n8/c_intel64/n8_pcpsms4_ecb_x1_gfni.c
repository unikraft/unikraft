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
//     SMS4 ECB encryption/decryption
// 
//  Contents:
//     cpSMS4_ECB_gfni_x1()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

#if (_IPP32E>=_IPP32E_K1)

#if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)

#include "pcpsms4_gfni.h"

IPP_OWN_DEFN (void, cpSMS4_ECB_gfni_x1, (Ipp8u* pOut, const Ipp8u* pInp, const Ipp32u* pRKey))
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

   TMP[1] = _mm_loadu_si128((__m128i*)pInp);
   TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));

   TMP[2] = _mm_alignr_epi32(TMP[1], TMP[1], 1);
   TMP[3] = _mm_alignr_epi32(TMP[1], TMP[1], 2);
   TMP[4] = _mm_alignr_epi32(TMP[1], TMP[1], 3);

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
      TMP[0] = _mm_shuffle_epi32(TMP[5], 0xAA); /* broadcast(key4 TMP[2]) */
      TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
      /* Sbox */
      TMP[0] = sBox128(TMP[0]);
      /* Sbox done, now L */
      TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), L128(TMP[0]));

      /* initial xors */
      TMP[0] = _mm_shuffle_epi32(TMP[5], 0xFF); /* broadcast(key4 TMP[3]) */
      TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
      /* Sbox */
      TMP[0] = sBox128(TMP[0]);
      /* Sbox done, now L */
      TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), L128(TMP[0]));
   }

   TMP[0] = _mm_unpacklo_epi32(TMP[2], TMP[1]);
   TMP[5] = _mm_unpacklo_epi32(TMP[4], TMP[3]);
   TMP[1] = _mm_unpacklo_epi64(TMP[5], TMP[0]);

   TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));

   _mm_storeu_si128((__m128i*)pOut, TMP[1]);

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }
}

#endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */

#endif /* (_IPP32E>=_IPP32E_K1) */
