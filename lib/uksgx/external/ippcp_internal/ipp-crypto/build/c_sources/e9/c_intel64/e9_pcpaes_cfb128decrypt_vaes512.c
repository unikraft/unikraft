/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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
//     AES encryption/decryption (CFB mode)
//
//  Contents:
//     DecryptCFB128_RIJ128pipe_VAES_NI()
//
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "pcpaes_encrypt_vaes512.h"

#if(_IPP32E>=_IPP32E_K1)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4310) // zmmintrin.h bug: truncation of constant value
#endif

IPP_OWN_DEFN (void, DecryptCFB128_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc,       // pointer to the ciphertext
                                                        Ipp8u* pDst,             // pointer to the plaintext
                                                        int len,                 // message length
                                                        const IppsAESSpec* pCtx, // pointer to context
                                                        const Ipp8u* pIV))        // pointer to the Initialization Vector
{
   int cipherRounds = RIJ_NR(pCtx) - 1;

   __m128i* pRkey   = (__m128i*)RIJ_EKEYS(pCtx);
   __m512i* pSrc512 = (__m512i*)pSrc;
   __m512i* pDst512 = (__m512i*)pDst;

   // load IV (128-bit)
   __m512i IV = _mm512_maskz_expandloadu_epi64(0xC0, pIV); // IV 0 0 0

   int blocks;
   for (blocks = len / MBS_RIJ128; blocks >= (4 * 4); blocks -= (4 * 4)) {
       // load ciphertext
      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pSrc512 + 3);

      // prepare vectors for decryption
      __m512i z0 = _mm512_alignr_epi64(blk0, IV, 6);    // C3  C2  C1  IV
      __m512i z1 = _mm512_alignr_epi64(blk1, blk0, 6);  // C7  C6  C5  C4
      __m512i z2 = _mm512_alignr_epi64(blk2, blk1, 6);  // C11 C10 C9  C8
      __m512i z3 = _mm512_alignr_epi64(blk3, blk2, 6);  // C15 C14 C13 C12

      // update IV
      IV = blk3;

      cpAESEncrypt4_VAES_NI(&z0, &z1, &z2, &z3, pRkey, cipherRounds);

      blk0 = _mm512_xor_si512(blk0, z0);
      blk1 = _mm512_xor_si512(blk1, z1);
      blk2 = _mm512_xor_si512(blk2, z2);
      blk3 = _mm512_xor_si512(blk3, z3);

      _mm512_storeu_si512(pDst512, blk0);
      _mm512_storeu_si512(pDst512 + 1, blk1);
      _mm512_storeu_si512(pDst512 + 2, blk2);
      _mm512_storeu_si512(pDst512 + 3, blk3);

      pSrc512 += 4;
      pDst512 += 4;
   }

   if ((3 * 4) <= blocks) {
       // load ciphertext
      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pSrc512 + 2);

      __m512i z0 = _mm512_alignr_epi64(blk0, IV, 6);
      __m512i z1 = _mm512_alignr_epi64(blk1, blk0, 6);
      __m512i z2 = _mm512_alignr_epi64(blk2, blk1, 6);

      // update IV
      IV = blk2;

      cpAESEncrypt3_VAES_NI(&z0, &z1, &z2, pRkey, cipherRounds);

      blk0 = _mm512_xor_si512(blk0, z0);
      blk1 = _mm512_xor_si512(blk1, z1);
      blk2 = _mm512_xor_si512(blk2, z2);

      _mm512_storeu_si512(pDst512, blk0);
      _mm512_storeu_si512(pDst512 + 1, blk1);
      _mm512_storeu_si512(pDst512 + 2, blk2);

      pSrc512 += 3;
      pDst512 += 3;
      blocks -= (3 * 4);
   }

   if ((4 * 2) <= blocks) {
       // load ciphertext
      __m512i blk0 = _mm512_loadu_si512(pSrc512);
      __m512i blk1 = _mm512_loadu_si512(pSrc512 + 1);

      __m512i z0 = _mm512_alignr_epi64(blk0, IV, 6);
      __m512i z1 = _mm512_alignr_epi64(blk1, blk0, 6);

      // update IV
      IV = blk1;

      cpAESEncrypt2_VAES_NI(&z0, &z1, pRkey, cipherRounds);

      blk0 = _mm512_xor_si512(blk0, z0);
      blk1 = _mm512_xor_si512(blk1, z1);

      _mm512_storeu_si512(pDst512, blk0);
      _mm512_storeu_si512(pDst512 + 1, blk1);

      pSrc512 += 2;
      pDst512 += 2;
      blocks -= (2 * 4);
   }

   for (; blocks >= 4; blocks -= 4) {
       // load ciphertext
      __m512i blk0 = _mm512_loadu_si512(pSrc512);

      __m512i z0 = _mm512_alignr_epi64(blk0, IV, 6);

      // update IV
      IV = blk0;

      cpAESEncrypt1_VAES_NI(&z0, pRkey, cipherRounds);

      blk0 = _mm512_xor_si512(blk0, z0);

      _mm512_storeu_si512(pDst512, blk0);

      pSrc512 += 1;
      pDst512 += 1;
   }

   if (blocks) {
      __mmask8 k = (__mmask8)((1 << (blocks + blocks)) - 1);

      __m512i blk0 = _mm512_maskz_loadu_epi64(k, pSrc512);

      __m512i z0 = _mm512_maskz_alignr_epi64(k, blk0, IV, 6);

      cpAESEncrypt1_VAES_NI(&z0, pRkey, cipherRounds);

      blk0 = _mm512_maskz_xor_epi64(k, blk0, z0);

      _mm512_mask_storeu_epi64(pDst512, k, blk0);
   }
}

#endif /* _IPP32E>=_IPP32E_K1 */
