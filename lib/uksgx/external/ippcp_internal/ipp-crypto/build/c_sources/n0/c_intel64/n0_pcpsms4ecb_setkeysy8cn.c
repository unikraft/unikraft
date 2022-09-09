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
//     cpSMS4_SetRoundKeys_aesni()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)

#include "pcpsms4_y8cn.h"

__INLINE __m128i Ltag(__m128i x)
{
   __m128i T = _mm_slli_epi32(x, 13);
   T = _mm_xor_si128(T, _mm_srli_epi32 (x,19));
   T = _mm_xor_si128(T, _mm_slli_epi32 (x,23));
   T = _mm_xor_si128(T, _mm_srli_epi32 (x, 9));
   return T;
}

/*
// compute round keys
*/

#define cpSMS4_SetRoundKeys_aesni OWNAPI(cpSMS4_SetRoundKeys_aesni)
   IPP_OWN_DECL (void, cpSMS4_SetRoundKeys_aesni, (Ipp32u* pRoundKey, const Ipp8u* pSecretKey))

IPP_OWN_DEFN (void, cpSMS4_SetRoundKeys_aesni, (Ipp32u* pRoundKey, const Ipp8u* pSecretKey))
{
   __ALIGN16 __m128i TMP[5];
   /*
      TMP[0] = T
      TMP[1] = K0
      TMP[2] = K1
      TMP[3] = K2
      TMP[4] = K3
   */
   TMP[1] = _mm_cvtsi32_si128((Ipp32s)(ENDIANNESS32(((Ipp32u*)pSecretKey)[0]) ^ SMS4_FK[0]));
   TMP[2] = _mm_cvtsi32_si128((Ipp32s)(ENDIANNESS32(((Ipp32u*)pSecretKey)[1]) ^ SMS4_FK[1]));
   TMP[3] = _mm_cvtsi32_si128((Ipp32s)(ENDIANNESS32(((Ipp32u*)pSecretKey)[2]) ^ SMS4_FK[2]));
   TMP[4] = _mm_cvtsi32_si128((Ipp32s)(ENDIANNESS32(((Ipp32u*)pSecretKey)[3]) ^ SMS4_FK[3]));

   const Ipp32u* pCK = SMS4_CK;

   int itr;
   for(itr=0; itr<8; itr++) {
      /* initial xors */
      TMP[0] = _mm_cvtsi32_si128((Ipp32s)pCK[0]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
      /* Sbox */
      TMP[0] = sBox(TMP[0]);
      /* Sbox done, now Ltag */
      TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[0]), Ltag(TMP[0]));
      pRoundKey[0] = (Ipp32u)_mm_cvtsi128_si32(TMP[1]);

      /* initial xors */
      TMP[0] = _mm_cvtsi32_si128((Ipp32s)pCK[1]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
      /* Sbox */
      TMP[0] = sBox(TMP[0]);
      /* Sbox done, now Ltag */
      TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[0]), Ltag(TMP[0]));
      pRoundKey[1] = (Ipp32u)_mm_cvtsi128_si32(TMP[2]);

      /* initial xors */
      TMP[0] = _mm_cvtsi32_si128((Ipp32s)pCK[2]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[4]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
      /* Sbox */
      TMP[0] = sBox(TMP[0]);
      /* Sbox done, now Ltag */
      TMP[3] = _mm_xor_si128(_mm_xor_si128(TMP[3], TMP[0]), Ltag(TMP[0]));
      pRoundKey[2] = (Ipp32u)_mm_cvtsi128_si32(TMP[3]);

      /* initial xors */
      TMP[0] = _mm_cvtsi32_si128((Ipp32s)pCK[3]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[1]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[2]);
      TMP[0] = _mm_xor_si128(TMP[0], TMP[3]);
      /* Sbox */
      TMP[0] = sBox(TMP[0]);
      /* Sbox done, now Ltag */
      TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[0]), Ltag(TMP[0]));
      pRoundKey[3] = (Ipp32u)_mm_cvtsi128_si32(TMP[4]);

      pCK += 4;
      pRoundKey += 4;
   }

   /* clear secret data */
   for(int i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_xor_si128(TMP[i],TMP[i]);
   }
}

#endif /* _IPP_P8, _IPP32E_Y8 */
