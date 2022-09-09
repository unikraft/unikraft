/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     AES encryption/decryption (ECB mode)
//
//  Contents:
//     EncryptECB_RIJ128pipe_VAES_NI
//     DecryptECB_RIJ128pipe_VAES_NI
//
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "pcpaes_encrypt_vaes512.h"
#include "pcpaes_decrypt_vaes512.h"

#if (_IPP32E>=_IPP32E_K1)

IPP_OWN_DEFN (void, EncryptECB_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc,        // pointer to the plaintext
                                                   Ipp8u* pDst,              // pointer to the ciphertext buffer
                                                   int len,                  // text length in bytes
                                                   const IppsAESSpec* pCtx))  // pointer to the context
{
   int cipherRounds = RIJ_NR(pCtx) - 1;

   __m128i* pRkey = (__m128i*)RIJ_EKEYS(pCtx);
   __m512i* pInp512 = (__m512i*)pSrc;
   __m512i* pOut512 = (__m512i*)pDst;

   int blocks;
   for (blocks = len / MBS_RIJ128; blocks >= (4 * 4); blocks -= (4 * 4)) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pInp512 + 3);

      cpAESEncrypt4_VAES_NI(&blk0, &blk1, &blk2, &blk3, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);
      _mm512_storeu_si512(pOut512 + 3, blk3);

      pInp512 += 4;
      pOut512 += 4;
   }

   if ((3 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);

      cpAESEncrypt3_VAES_NI(&blk0, &blk1, &blk2, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);

      pInp512 += 3;
      pOut512 += 3;
      blocks -= (3 * 4);
   }

   if ((4 * 2) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);

      cpAESEncrypt2_VAES_NI(&blk0, &blk1, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);

      pInp512 += 2;
      pOut512 += 2;
      blocks -= (2 * 4);
   }

   for (; blocks >= 4; blocks -= 4) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);

      cpAESEncrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);

      pInp512 += 1;
      pOut512 += 1;
   }

   if (blocks) {
      __mmask8 k = (__mmask8)((1 << (blocks + blocks)) - 1);
      __m512i blk0 = _mm512_maskz_loadu_epi64(k, pInp512);

      cpAESEncrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      _mm512_mask_storeu_epi64(pOut512, k, blk0);
   }
}

////////////////////////////////////////////////////////////////////////////////

IPP_OWN_DEFN (void, DecryptECB_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc,         // pointer to the plaintext
                                                   Ipp8u* pDst,               // pointer to the ciphertext buffer
                                                   int len,                   // text length in bytes
                                                   const IppsAESSpec* pCtx))   // pointer to the context
{
   int cipherRounds = RIJ_NR(pCtx) - 1;

   __m128i* pRkey = (__m128i*)RIJ_DKEYS(pCtx) + cipherRounds + 1;
   __m512i* pInp512 = (__m512i*)pSrc;
   __m512i* pOut512 = (__m512i*)pDst;

   int blocks;
   for (blocks = len / MBS_RIJ128; blocks >= (4 * 4); blocks -= (4 * 4)) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pInp512 + 3);

      cpAESDecrypt4_VAES_NI(&blk0, &blk1, &blk2, &blk3, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);
      _mm512_storeu_si512(pOut512 + 3, blk3);

      pInp512 += 4;
      pOut512 += 4;
   }

   if ((3 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);

      cpAESDecrypt3_VAES_NI(&blk0, &blk1, &blk2, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);

      pInp512 += 3;
      pOut512 += 3;
      blocks -= (3 * 4);
   }

   if ((4 * 2) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);

      cpAESDecrypt2_VAES_NI(&blk0, &blk1, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);

      pInp512 += 2;
      pOut512 += 2;
      blocks -= (2 * 4);
   }

   for (; blocks >= 4; blocks -= 4) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);

      cpAESDecrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      _mm512_storeu_si512(pOut512, blk0);
      pInp512 += 1;
      pOut512 += 1;
   }

   if (blocks) {
      __mmask8 k = (__mmask8)((1 << (blocks + blocks)) - 1);
      __m512i blk0 = _mm512_maskz_loadu_epi64(k, pInp512);

      cpAESDecrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      _mm512_mask_storeu_epi64(pOut512, k, blk0);
   }
}

#endif /* _IPP32E>=_IPP32E_K1 */
