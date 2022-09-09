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
//     DecryptCFB_RIJ128pipe_VAES_NI()
//
//
*/

#if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)

#include "owncp.h"
#include "pcpaesm.h"
#include "pcpaes_encrypt_vaes512.h"

#if(_IPP32E>=_IPP32E_K1)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4310) // zmmintrin.h bug: truncation of constant value
#endif

__INLINE Ipp64u broadcast_16to64(Ipp16u mask16)
{
   Ipp64u mask64 = (Ipp64u)mask16;
   mask64 = (mask64 << 48) | (mask64 << 32) | (mask64 << 16) | mask64;
   return mask64;
}

__INLINE __m512i getInputBlocks(__m128i * const currentState, const __m512i * const pCipherBlocks, __mmask16 blocksCompressMask)
{
   // extract 128-bit cipher blocks
   __m128i c0 = _mm512_extracti64x2_epi64(*pCipherBlocks, 0);
   __m128i c1 = _mm512_extracti64x2_epi64(*pCipherBlocks, 1);
   __m128i c2 = _mm512_extracti64x2_epi64(*pCipherBlocks, 2);
   __m128i c3 = _mm512_extracti64x2_epi64(*pCipherBlocks, 3);

   //  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit
   __m128i inpBlk0 = *currentState;
   // drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits
   __m128i inpBlk1 = _mm_mask_compress_epi8(c0, blocksCompressMask, inpBlk0);
   __m128i inpBlk2 = _mm_mask_compress_epi8(c1, blocksCompressMask, inpBlk1);
   __m128i inpBlk3 = _mm_mask_compress_epi8(c2, blocksCompressMask, inpBlk2);

   // next InputBlock ready
   *currentState = _mm_mask_compress_epi8(c3, blocksCompressMask, inpBlk3);

   // inserts
   __m512i inpBlk512 = _mm512_setzero_si512();
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0x03, inpBlk0); // 0000 0011
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0x0C, inpBlk1); // 0000 1100
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0x30, inpBlk2); // 0011 0000
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0xC0, inpBlk3); // 1100 0000

   return inpBlk512;
}

////////////////////////////////////////////////////////////////////////////////
IPP_OWN_DEFN (void, DecryptCFB_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc,      // pointer to the ciphertext
                                                   Ipp8u* pDst,             // pointer to the plaintext
                                                   int len,                 // message length
                                                   int cfbBlkSize,          // CFB block size in bytes (1 <= cfbBlkSize <= 16)); s = 8*cfbBlkSize
                                                   const IppsAESSpec* pCtx, // pointer to context
                                                   const Ipp8u* pIV))       // pointer to the Initialization Vector
{
   const int cipherRounds = RIJ_NR(pCtx) - 1;

   __m128i* pRkey      = (__m128i*)RIJ_EKEYS(pCtx);
   Ipp8u* pSrc8        = (Ipp8u*)pSrc;
   Ipp8u* pDst8        = (Ipp8u*)pDst;

   const int bytesPerLoad512 = 4 * cfbBlkSize;
   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   // load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register
   __mmask64 kLsbMask64 = (__mmask64)broadcast_16to64((Ipp16u)(0xFFFF << (16-cfbBlkSize)));
   // same mask to load in MSB parts
   __mmask64 kMsbMask64 = (__mmask64)broadcast_16to64(~blocksCompressMask);

   // load IV
   __m128i IV128 = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   __m128i currentState = IV128;

   int blocks;
   for (blocks = len / cfbBlkSize; blocks >= (4 * 4); blocks -= (4 * 4)) {
      // load cipher blocks to LSB parts of registers
      __m512i ciphLsb0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8);
      __m512i ciphLsb1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8 + 1 * bytesPerLoad512);
      __m512i ciphLsb2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8 + 2 * bytesPerLoad512);
      __m512i ciphLsb3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8 + 3 * bytesPerLoad512);

      // load same cipher blocks to MSB parts of registers (shall be taken from cache)
      __m512i ciphMsb0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8);
      __m512i ciphMsb1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8 + 1 * bytesPerLoad512);
      __m512i ciphMsb2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8 + 2 * bytesPerLoad512);
      __m512i ciphMsb3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8 + 3 * bytesPerLoad512);

      // prepare InputBlocks for decryption
      __m512i inpBlk0 = getInputBlocks(&currentState, &ciphLsb0, blocksCompressMask);
      __m512i inpBlk1 = getInputBlocks(&currentState, &ciphLsb1, blocksCompressMask);
      __m512i inpBlk2 = getInputBlocks(&currentState, &ciphLsb2, blocksCompressMask);
      __m512i inpBlk3 = getInputBlocks(&currentState, &ciphLsb3, blocksCompressMask);

      cpAESEncrypt4_VAES_NI(&inpBlk0, &inpBlk1, &inpBlk2, &inpBlk3, pRkey, cipherRounds);

      ciphLsb0 = _mm512_xor_si512(ciphMsb0, inpBlk0);
      ciphLsb1 = _mm512_xor_si512(ciphMsb1, inpBlk1);
      ciphLsb2 = _mm512_xor_si512(ciphMsb2, inpBlk2);
      ciphLsb3 = _mm512_xor_si512(ciphMsb3, inpBlk3);

      _mm512_mask_compressstoreu_epi8(pDst8, kMsbMask64, ciphLsb0);
      _mm512_mask_compressstoreu_epi8(pDst8 + 1 * bytesPerLoad512, kMsbMask64, ciphLsb1);
      _mm512_mask_compressstoreu_epi8(pDst8 + 2 * bytesPerLoad512, kMsbMask64, ciphLsb2);
      _mm512_mask_compressstoreu_epi8(pDst8 + 3 * bytesPerLoad512, kMsbMask64, ciphLsb3);

      pSrc8 += 4 * bytesPerLoad512;
      pDst8 += 4 * bytesPerLoad512;
   }

   if ((3 * 4) <= blocks) {
      // load ciphertext
      __m512i ciphLsb0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8);
      __m512i ciphLsb1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8 + 1 * bytesPerLoad512);
      __m512i ciphLsb2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8 + 2 * bytesPerLoad512);

      __m512i ciphMsb0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8);
      __m512i ciphMsb1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8 + 1 * bytesPerLoad512);
      __m512i ciphMsb2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8 + 2 * bytesPerLoad512);

      // prepare InputBlocks for decryption
      __m512i inpBlk0 = getInputBlocks(&currentState, &ciphLsb0, blocksCompressMask);
      __m512i inpBlk1 = getInputBlocks(&currentState, &ciphLsb1, blocksCompressMask);
      __m512i inpBlk2 = getInputBlocks(&currentState, &ciphLsb2, blocksCompressMask);

      cpAESEncrypt3_VAES_NI(&inpBlk0, &inpBlk1, &inpBlk2, pRkey, cipherRounds);

      ciphLsb0 = _mm512_xor_si512(ciphMsb0, inpBlk0);
      ciphLsb1 = _mm512_xor_si512(ciphMsb1, inpBlk1);
      ciphLsb2 = _mm512_xor_si512(ciphMsb2, inpBlk2);

      _mm512_mask_compressstoreu_epi8(pDst8, kMsbMask64, ciphLsb0);
      _mm512_mask_compressstoreu_epi8(pDst8 + 1 * bytesPerLoad512, kMsbMask64, ciphLsb1);
      _mm512_mask_compressstoreu_epi8(pDst8 + 2 * bytesPerLoad512, kMsbMask64, ciphLsb2);

      pSrc8 += 3 * bytesPerLoad512;
      pDst8 += 3 * bytesPerLoad512;
      blocks -= (3 * 4);
   }

   if ((4 * 2) <= blocks) {
      // load ciphertext
      __m512i ciphLsb0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8);
      __m512i ciphLsb1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8 + 1 * bytesPerLoad512);

      __m512i ciphMsb0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8);
      __m512i ciphMsb1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8 + 1 * bytesPerLoad512);

      // prepare InputBlocks for decryption
      __m512i inpBlk0 = getInputBlocks(&currentState, &ciphLsb0, blocksCompressMask);
      __m512i inpBlk1 = getInputBlocks(&currentState, &ciphLsb1, blocksCompressMask);

      cpAESEncrypt2_VAES_NI(&inpBlk0, &inpBlk1, pRkey, cipherRounds);

      ciphLsb0 = _mm512_xor_si512(ciphMsb0, inpBlk0);
      ciphLsb1 = _mm512_xor_si512(ciphMsb1, inpBlk1);

      _mm512_mask_compressstoreu_epi8(pDst8, kMsbMask64, ciphLsb0);
      _mm512_mask_compressstoreu_epi8(pDst8 + 1 * bytesPerLoad512, kMsbMask64, ciphLsb1);

      pSrc8 += 2 * bytesPerLoad512;
      pDst8 += 2 * bytesPerLoad512;
      blocks -= (2 * 4);
   }

   for (; blocks >= 4; blocks -= 4) {
      // load ciphertext
      __m512i ciphLsb0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc8);
      __m512i ciphMsb0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc8);

      // prepare InputBlocks for decryption
      __m512i inpBlk0 = getInputBlocks(&currentState, &ciphLsb0, blocksCompressMask);

      cpAESEncrypt1_VAES_NI(&inpBlk0, pRkey, cipherRounds);

      ciphLsb0 = _mm512_xor_si512(ciphMsb0, inpBlk0);

      _mm512_mask_compressstoreu_epi8(pDst8, kMsbMask64, ciphLsb0);

      pSrc8 += bytesPerLoad512;
      pDst8 += bytesPerLoad512;
   }

   // at least one block left (max 3 blocks)
   if (blocks) {
       __mmask64 k64 = (1LL << (blocks << 4)) - 1;

      // load ciphertext
      __m512i ciphLsb0 = _mm512_maskz_expandloadu_epi8(k64 & kLsbMask64, pSrc8);
      __m512i ciphMsb0 = _mm512_maskz_expandloadu_epi8(k64 & kMsbMask64, pSrc8);

      // prepare InputBlocks for decryption
      __m512i inpBlk0 = _mm512_setzero_si512();
      inpBlk0 = _mm512_mask_broadcast_i64x2(inpBlk0, 0x03, currentState);
      __m128i c;
      for (int i = 0; i < blocks; i++) {
         // NB: we cannot provide non-immediate parameter to extract function
         switch (i) {
         case 0:  c = _mm512_extracti64x2_epi64(ciphLsb0, 0); break;
         case 1:  c = _mm512_extracti64x2_epi64(ciphLsb0, 1); break;
         default: c = _mm512_extracti64x2_epi64(ciphLsb0, 2);
         }
         currentState = _mm_mask_compress_epi8(c, blocksCompressMask, currentState);
         inpBlk0 = _mm512_mask_broadcast_i64x2(inpBlk0, (__mmask8)(0x03 << ((i + 1) << 1)), currentState);
      }

      cpAESEncrypt1_VAES_NI(&inpBlk0, pRkey, cipherRounds);

      ciphLsb0 = _mm512_xor_si512(ciphMsb0, inpBlk0);

      _mm512_mask_compressstoreu_epi8(pDst8, k64 & kMsbMask64, ciphLsb0);
   }
}

#endif /* _IPP32E>=_IPP32E_K1                                   */
#else
typedef int to_avoid_translation_unit_is_empty_warning;
#endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */
