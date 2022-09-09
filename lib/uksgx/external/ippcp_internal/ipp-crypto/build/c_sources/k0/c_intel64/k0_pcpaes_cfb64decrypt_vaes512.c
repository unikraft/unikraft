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
//     DecryptCFB64_RIJ128pipe_VAES_NI()
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

IPP_OWN_DEFN (void, DecryptCFB64_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc,       // pointer to the ciphertext
                                                    Ipp8u* pDst,             // pointer to the plaintext
                                                    int len,                 // message length
                                                    const IppsAESSpec* pCtx, // pointer to context
                                                    const Ipp8u* pIV))        // pointer to the Initialization Vector
{
   int cipherRounds = RIJ_NR(pCtx) - 1;

   __m128i* pRkey   = (__m128i*)RIJ_EKEYS(pCtx);
   __m256i* pSrc256 = (__m256i*)pSrc;
   __m256i* pDst256 = (__m256i*)pDst;

   // load/store masks
   __mmask8 k8_load  = 0xAA; // 1010 1010 - load to high addresses of 128-bit lanes
   __mmask8 k8_store = 0x55; // 0101 0101 - store low addresses of 128-bit lanes (MSB of OutputBlocks)

   // load IV and init IV vector in appropriate for further batch processing manner
   __m128i IV128 = _mm_maskz_loadu_epi64(0x03, pIV);
   // 64-bit values: IV_l IV_h IV_l IV_h 0 0 0 0
   __m512i IV    = _mm512_maskz_broadcast_i64x2(0xF0, IV128);
   // 64-bit values: IV_l IV_h IV_h IV_h 0 0 0 0
   IV = _mm512_maskz_permutex_epi64(0xF0, IV, 0xE1); // 0xE1 = 1110 0001

   int blocks = len / (MBS_RIJ128 >> 1);

   for (; blocks >= (4 * 4); blocks -= (4 * 4)) {
       // load 64-bit blocks of ciphertext to high part of 128-bit lanes
      __m512i blk0 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256);      // C3 0  C2 0  C1 0  C0 0
      __m512i blk1 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256 + 1);  // C7 0  C6 0  C5 0  C4 0
      __m512i blk2 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256 + 2);
      __m512i blk3 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256 + 3);

      // prepare vectors for decryption
      __m512i z00 = _mm512_alignr_epi64(blk0, IV, 4);    // C2  0 | C1  0 | IV_l IV_h | IV_h IV_l
      __m512i z01 = _mm512_alignr_epi64(blk0, IV, 6);    // C3  0 | C2  0 | C1   0    | IV_l IV_h
      z00 = _mm512_unpackhi_epi64(z00, z01);             // C3 C2 | C2 C1 | C1   IV_l | IV_l IV_h

      __m512i z10 = _mm512_alignr_epi64(blk1, blk0, 4);
      __m512i z11 = _mm512_alignr_epi64(blk1, blk0, 6);
      z10 = _mm512_unpackhi_epi64(z10, z11);

      __m512i z20 = _mm512_alignr_epi64(blk2, blk1, 4);
      __m512i z21 = _mm512_alignr_epi64(blk2, blk1, 6);
      z20 = _mm512_unpackhi_epi64(z20, z21);

      __m512i z30 = _mm512_alignr_epi64(blk3, blk2, 4);
      __m512i z31 = _mm512_alignr_epi64(blk3, blk2, 6);
      z30 = _mm512_unpackhi_epi64(z30, z31);

      // update IV
      IV = blk3;

      cpAESEncrypt4_VAES_NI(&z00, &z10, &z20, &z30, pRkey, cipherRounds);

      // move cipher blocks to MSB part of 128-bit lanes
      blk0 = _mm512_bsrli_epi128(blk0, 8);
      blk1 = _mm512_bsrli_epi128(blk1, 8);
      blk2 = _mm512_bsrli_epi128(blk2, 8);
      blk3 = _mm512_bsrli_epi128(blk3, 8);

      blk0 = _mm512_xor_si512(blk0, z00);
      blk1 = _mm512_xor_si512(blk1, z10);
      blk2 = _mm512_xor_si512(blk2, z20);
      blk3 = _mm512_xor_si512(blk3, z30);

      _mm512_mask_compressstoreu_epi64(pDst256, k8_store, blk0);
      _mm512_mask_compressstoreu_epi64(pDst256 + 1, k8_store,  blk1);
      _mm512_mask_compressstoreu_epi64(pDst256 + 2, k8_store, blk2);
      _mm512_mask_compressstoreu_epi64(pDst256 + 3, k8_store, blk3);

      pSrc256 += 4;
      pDst256 += 4;
   }

   if ((3 * 4) <= blocks) {
       // load 64-bit blocks of ciphertext to high part of 128-bit lanes
      __m512i blk0 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256);
      __m512i blk1 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256 + 1);
      __m512i blk2 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256 + 2);

      // prepare vectors for decryption
      __m512i z00 = _mm512_alignr_epi64(blk0, IV, 4);
      __m512i z01 = _mm512_alignr_epi64(blk0, IV, 6);
      z00 = _mm512_unpackhi_epi64(z00, z01);

      __m512i z10 = _mm512_alignr_epi64(blk1, blk0, 4);
      __m512i z11 = _mm512_alignr_epi64(blk1, blk0, 6);
      z10 = _mm512_unpackhi_epi64(z10, z11);

      __m512i z20 = _mm512_alignr_epi64(blk2, blk1, 4);
      __m512i z21 = _mm512_alignr_epi64(blk2, blk1, 6);
      z20 = _mm512_unpackhi_epi64(z20, z21);

      // update IV
      IV = blk2;

      cpAESEncrypt3_VAES_NI(&z00, &z10, &z20, pRkey, cipherRounds);

      // move cipher blocks to MSB part of 128-bit lanes
      blk0 = _mm512_bsrli_epi128(blk0, 8);
      blk1 = _mm512_bsrli_epi128(blk1, 8);
      blk2 = _mm512_bsrli_epi128(blk2, 8);

      blk0 = _mm512_xor_si512(blk0, z00);
      blk1 = _mm512_xor_si512(blk1, z10);
      blk2 = _mm512_xor_si512(blk2, z20);

      _mm512_mask_compressstoreu_epi64(pDst256, k8_store, blk0);
      _mm512_mask_compressstoreu_epi64(pDst256 + 1, k8_store,  blk1);
      _mm512_mask_compressstoreu_epi64(pDst256 + 2, k8_store, blk2);

      pSrc256 += 3;
      pDst256 += 3;
      blocks -= (3 * 4);
   }

   if ((4 * 2) <= blocks) {
       // load 64-bit blocks of ciphertext to high part of 128-bit lanes
      __m512i blk0 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256);
      __m512i blk1 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256 + 1);

      // prepare vectors for decryption
      __m512i z00 = _mm512_alignr_epi64(blk0, IV, 4);
      __m512i z01 = _mm512_alignr_epi64(blk0, IV, 6);
      z00 = _mm512_unpackhi_epi64(z00, z01);

      __m512i z10 = _mm512_alignr_epi64(blk1, blk0, 4);
      __m512i z11 = _mm512_alignr_epi64(blk1, blk0, 6);
      z10 = _mm512_unpackhi_epi64(z10, z11);

      // update IV
      IV = blk1;

      cpAESEncrypt2_VAES_NI(&z00, &z10, pRkey, cipherRounds);

      // move cipher blocks to MSB part of 128-bit lanes
      blk0 = _mm512_bsrli_epi128(blk0, 8);
      blk1 = _mm512_bsrli_epi128(blk1, 8);

      blk0 = _mm512_xor_si512(blk0, z00);
      blk1 = _mm512_xor_si512(blk1, z10);

      _mm512_mask_compressstoreu_epi64(pDst256, k8_store, blk0);
      _mm512_mask_compressstoreu_epi64(pDst256 + 1, k8_store,  blk1);

      pSrc256 += 2;
      pDst256 += 2;
      blocks -= (2 * 4);
   }

   for (; blocks >= 4; blocks -= 4) {
       // load 64-bit blocks of ciphertext to high part of 128-bit lanes
      __m512i blk0 = _mm512_maskz_expandloadu_epi64(k8_load, pSrc256);

      // prepare vectors for decryption
      __m512i z00 = _mm512_alignr_epi64(blk0, IV, 4);
      __m512i z01 = _mm512_alignr_epi64(blk0, IV, 6);
      z00 = _mm512_unpackhi_epi64(z00, z01);

      //  update IV
      IV = blk0;

      cpAESEncrypt1_VAES_NI(&z00, pRkey, cipherRounds);

      // move cipher blocks to MSB part of 128-bit lanes
      blk0 = _mm512_bsrli_epi128(blk0, 8);

      blk0 = _mm512_xor_si512(blk0, z00);

      _mm512_mask_compressstoreu_epi64(pDst256, k8_store, blk0);

      pSrc256 += 1;
      pDst256 += 1;
   }

   if (blocks) {
      __mmask8 k = (__mmask8)((1 << (blocks + blocks)) - 1); // 64-bit chunks

       // load 64-bit blocks of ciphertext to hi part of 128-bit lanes
      __m512i blk0 = _mm512_maskz_expandloadu_epi64(k & k8_load, pSrc256);

      // prepare vectors for decryption
      __m512i z00 = _mm512_alignr_epi64(blk0, IV, 4);
      __m512i z01 = _mm512_alignr_epi64(blk0, IV, 6);
      z00 = _mm512_maskz_unpackhi_epi64(k, z00, z01);

      cpAESEncrypt1_VAES_NI(&z00, pRkey, cipherRounds);

      // move cipher blocks to MSB part of 128-bit lanes
      blk0 = _mm512_bsrli_epi128(blk0, 8);

      blk0 = _mm512_xor_epi64(blk0, z00);

      _mm512_mask_compressstoreu_epi64(pDst256, k & k8_store, blk0);
   }
}

#endif /* _IPP32E>=_IPP32E_K1 */
