/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
* All Rights Reserved.
*
* If this  software was obtained  under the  Intel Simplified  Software License,
* the following terms apply:
*
* The source code,  information  and material  ("Material") contained  herein is
* owned by Intel Corporation or its  suppliers or licensors,  and  title to such
* Material remains with Intel  Corporation or its  suppliers or  licensors.  The
* Material  contains  proprietary  information  of  Intel or  its suppliers  and
* licensors.  The Material is protected by  worldwide copyright  laws and treaty
* provisions.  No part  of  the  Material   may  be  used,  copied,  reproduced,
* modified, published,  uploaded, posted, transmitted,  distributed or disclosed
* in any way without Intel's prior express written permission.  No license under
* any patent,  copyright or other  intellectual property rights  in the Material
* is granted to  or  conferred  upon  you,  either   expressly,  by implication,
* inducement,  estoppel  or  otherwise.  Any  license   under such  intellectual
* property rights must be express and approved by Intel in writing.
*
* Unless otherwise agreed by Intel in writing,  you may not remove or alter this
* notice or  any  other  notice   embedded  in  Materials  by  Intel  or Intel's
* suppliers or licensors in any way.
*
*
* If this  software  was obtained  under the  Apache License,  Version  2.0 (the
* "License"), the following terms apply:
*
* You may  not use this  file except  in compliance  with  the License.  You may
* obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*
*
* Unless  required  by   applicable  law  or  agreed  to  in  writing,  software
* distributed under the License  is distributed  on an  "AS IS"  BASIS,  WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
* See the   License  for the   specific  language   governing   permissions  and
* limitations under the License.
*******************************************************************************/

/*
//
//  Purpose:
//     Cryptography Primitive.
//     SMS4 CFB decryption
//
//  Contents:
//     cpSMS4_CFB_gfni512()
//     cpSMS4_CFB_gfni512x48()
//     cpSMS4_CFB_gfni512x32()
//     cpSMS4_CFB_gfni512x16()
//     cpSMS4_CFB_gfni128x12()
//     cpSMS4_CFB_gfni128x8()
//     cpSMS4_CFB_gfni128x4()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

#if (_IPP32E>=_IPP32E_K1)
#if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)

#include "pcpsms4_gfni.h"

static 
void cpSMS4_CFB_dec_gfni512x48(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV);
static 
void cpSMS4_CFB_dec_gfni512x32(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV);
static 
void cpSMS4_CFB_dec_gfni512x16(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV);
static 
void cpSMS4_CFB_dec_gfni128x12(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV);
static 
void cpSMS4_CFB_dec_gfni128x8(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV);
static 
void cpSMS4_CFB_dec_gfni128x4(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV);

__FORCEINLINE Ipp64u broadcast_16to64(Ipp16u mask16)
{
	Ipp64u mask64 = (Ipp64u)mask16;
	mask64 = (mask64 << 48) | (mask64 << 32) | (mask64 << 16) | mask64;
	return mask64;
}

__FORCEINLINE __m512i getInputBlocks(__m128i * const currentState, const __m512i * const pCipherBlocks, __mmask16 blocksCompressMask)
{
   /* extract 128-bit cipher blocks */
   __m128i c0 = _mm512_extracti64x2_epi64(*pCipherBlocks, 0);
   __m128i c1 = _mm512_extracti64x2_epi64(*pCipherBlocks, 1);
   __m128i c2 = _mm512_extracti64x2_epi64(*pCipherBlocks, 2);
   __m128i c3 = _mm512_extracti64x2_epi64(*pCipherBlocks, 3);

   /*  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit */
   __m128i inpBlk0 = *currentState;
   /* drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits */
   __m128i inpBlk1 = _mm_mask_compress_epi8(c0, blocksCompressMask, inpBlk0);
   __m128i inpBlk2 = _mm_mask_compress_epi8(c1, blocksCompressMask, inpBlk1);
   __m128i inpBlk3 = _mm_mask_compress_epi8(c2, blocksCompressMask, inpBlk2);

   /* next InputBlock ready */
   *currentState = _mm_mask_compress_epi8(c3, blocksCompressMask, inpBlk3);

   /* inserts */
   __m512i inpBlk512 = _mm512_setzero_si512();
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0x03, inpBlk0); /* 0000 0011 */
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0x0C, inpBlk1); /* 0000 1100 */
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0x30, inpBlk2); /* 0011 0000 */
   inpBlk512 = _mm512_mask_broadcast_i64x2(inpBlk512, 0xC0, inpBlk3); /* 1100 0000 */

   return inpBlk512;
}

/*
// 64*cfbBlkSize bytes processing
*/

IPP_OWN_DEFN (void, cpSMS4_CFB_dec_gfni512, (Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV))
{
   __ALIGN16 __m512i TMP[24];

   /* 
   // TMP[0..15]  - decrypted text blocks 
   // TMP[16..19] - key
   // TMP[20..23] - temp out
   */

   const int bytesPerLoad512 = 4 * cfbBlkSize;
   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   int processedLen = len - (len % (64 * cfbBlkSize));
   int blocks;

   /* load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register */
   __mmask64 kLsbMask64 = broadcast_16to64((Ipp16u)(0xFFFF << (16-cfbBlkSize)));
   /* same mask to load in MSB parts */
   __mmask64 kMsbMask64 = broadcast_16to64(~blocksCompressMask);

   /* load IV */
   __m128i currentState = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   for (blocks = len / cfbBlkSize; blocks >= (64); blocks -= (64)) {
      /* load cipher blocks to LSB parts of registers */
      __m512i ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 0 * bytesPerLoad512);
      __m512i ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 1 * bytesPerLoad512);
      __m512i ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 2 * bytesPerLoad512);
      __m512i ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 3 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[0], TMP[1], TMP[2], TMP[3], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 4 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 5 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 6 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 7 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 8  * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 9  * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 10 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 11 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[8], TMP[9], TMP[10], TMP[11], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 12 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 13 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 14 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 15 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[12], TMP[13], TMP[14], TMP[15], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      int itr;
      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[19] = TMP[18] = TMP[17] = TMP[16] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[1] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[2] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[3] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[5] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[6] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[7] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[9] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[10]);
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[11]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[13]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[14]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[15]);
         /* Sbox */
         TMP[16] = sBox512(TMP[16]);
         TMP[17] = sBox512(TMP[17]);
         TMP[18] = sBox512(TMP[18]);
         TMP[19] = sBox512(TMP[19]);
         /* Sbox done, now L */
         TMP[0]  = _mm512_xor_si512(_mm512_xor_si512(TMP[0],  TMP[16]), L512(TMP[16]));
         TMP[4]  = _mm512_xor_si512(_mm512_xor_si512(TMP[4],  TMP[17]), L512(TMP[17]));
         TMP[8]  = _mm512_xor_si512(_mm512_xor_si512(TMP[8],  TMP[18]), L512(TMP[18]));
         TMP[12] = _mm512_xor_si512(_mm512_xor_si512(TMP[12], TMP[19]), L512(TMP[19]));

         /* initial xors */
         TMP[19] = TMP[18] = TMP[17] = TMP[16] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[2] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[3] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[0] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[6] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[7] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[4] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[10]);
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[11]);
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[8] );
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[14]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[15]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[12]);
         /* Sbox */
         TMP[16] = sBox512(TMP[16]);
         TMP[17] = sBox512(TMP[17]);
         TMP[18] = sBox512(TMP[18]);
         TMP[19] = sBox512(TMP[19]);
         /* Sbox done, now L */
         TMP[1]  = _mm512_xor_si512(_mm512_xor_si512(TMP[1],  TMP[16]), L512(TMP[16]));
         TMP[5]  = _mm512_xor_si512(_mm512_xor_si512(TMP[5],  TMP[17]), L512(TMP[17]));
         TMP[9]  = _mm512_xor_si512(_mm512_xor_si512(TMP[9],  TMP[18]), L512(TMP[18]));
         TMP[13] = _mm512_xor_si512(_mm512_xor_si512(TMP[13], TMP[19]), L512(TMP[19]));

         /* initial xors */
         TMP[19] = TMP[18] = TMP[17] = TMP[16] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[3] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[0] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[1] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[7] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[4] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[5] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[11]);
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[8] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[9] );
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[15]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[12]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[13]);
         /* Sbox */
         TMP[16] = sBox512(TMP[16]);
         TMP[17] = sBox512(TMP[17]);
         TMP[18] = sBox512(TMP[18]);
         TMP[19] = sBox512(TMP[19]);
         /* Sbox done, now L */
         TMP[2]  = _mm512_xor_si512(_mm512_xor_si512(TMP[2],  TMP[16]), L512(TMP[16]));
         TMP[6]  = _mm512_xor_si512(_mm512_xor_si512(TMP[6],  TMP[17]), L512(TMP[17]));
         TMP[10] = _mm512_xor_si512(_mm512_xor_si512(TMP[10], TMP[18]), L512(TMP[18]));
         TMP[14] = _mm512_xor_si512(_mm512_xor_si512(TMP[14], TMP[19]), L512(TMP[19]));

         /* initial xors */
         TMP[19] = TMP[18] = TMP[17] = TMP[16] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[0] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[1] );
         TMP[16] = _mm512_xor_si512(TMP[16], TMP[2] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[4] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[5] );
         TMP[17] = _mm512_xor_si512(TMP[17], TMP[6] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[8] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[9] );
         TMP[18] = _mm512_xor_si512(TMP[18], TMP[10]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[12]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[13]);
         TMP[19] = _mm512_xor_si512(TMP[19], TMP[14]);
         /* Sbox */
         TMP[16] = sBox512(TMP[16]);
         TMP[17] = sBox512(TMP[17]);
         TMP[18] = sBox512(TMP[18]);
         TMP[19] = sBox512(TMP[19]);
         /* Sbox done, now L */
         TMP[3] = _mm512_xor_si512(_mm512_xor_si512(TMP[3],   TMP[16]), L512(TMP[16]));
         TMP[7] = _mm512_xor_si512(_mm512_xor_si512(TMP[7],   TMP[17]), L512(TMP[17]));
         TMP[11] = _mm512_xor_si512(_mm512_xor_si512(TMP[11], TMP[18]), L512(TMP[18]));
         TMP[15] = _mm512_xor_si512(_mm512_xor_si512(TMP[15], TMP[19]), L512(TMP[19]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_512(TMP[20], TMP[21], TMP[22], TMP[23], TMP[0], TMP[1], TMP[2], TMP[3]);
      TMP[20] = _mm512_shuffle_epi8(TMP[20], M512(swapBytes));
      TMP[21] = _mm512_shuffle_epi8(TMP[21], M512(swapBytes));
      TMP[22] = _mm512_shuffle_epi8(TMP[22], M512(swapBytes));
      TMP[23] = _mm512_shuffle_epi8(TMP[23], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 0 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 1 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 2 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 3 * bytesPerLoad512);
      
      TMP[20] = _mm512_xor_si512(ciphBytes0, TMP[20]);
      TMP[21] = _mm512_xor_si512(ciphBytes1, TMP[21]);
      TMP[22] = _mm512_xor_si512(ciphBytes2, TMP[22]);
      TMP[23] = _mm512_xor_si512(ciphBytes3, TMP[23]);

      _mm512_mask_compressstoreu_epi8(pDst + 0 * bytesPerLoad512, kMsbMask64, TMP[20]);
      _mm512_mask_compressstoreu_epi8(pDst + 1 * bytesPerLoad512, kMsbMask64, TMP[21]);
      _mm512_mask_compressstoreu_epi8(pDst + 2 * bytesPerLoad512, kMsbMask64, TMP[22]);
      _mm512_mask_compressstoreu_epi8(pDst + 3 * bytesPerLoad512, kMsbMask64, TMP[23]);

      TRANSPOSE_OUT_512(TMP[20], TMP[21], TMP[22], TMP[23], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[20] = _mm512_shuffle_epi8(TMP[20], M512(swapBytes));
      TMP[21] = _mm512_shuffle_epi8(TMP[21], M512(swapBytes));
      TMP[22] = _mm512_shuffle_epi8(TMP[22], M512(swapBytes));
      TMP[23] = _mm512_shuffle_epi8(TMP[23], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 4 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 5 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 6 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 7 * bytesPerLoad512);
      
      TMP[20] = _mm512_xor_si512(ciphBytes0, TMP[20]);
      TMP[21] = _mm512_xor_si512(ciphBytes1, TMP[21]);
      TMP[22] = _mm512_xor_si512(ciphBytes2, TMP[22]);
      TMP[23] = _mm512_xor_si512(ciphBytes3, TMP[23]);

      _mm512_mask_compressstoreu_epi8(pDst + 4 * bytesPerLoad512, kMsbMask64, TMP[20]);
      _mm512_mask_compressstoreu_epi8(pDst + 5 * bytesPerLoad512, kMsbMask64, TMP[21]);
      _mm512_mask_compressstoreu_epi8(pDst + 6 * bytesPerLoad512, kMsbMask64, TMP[22]);
      _mm512_mask_compressstoreu_epi8(pDst + 7 * bytesPerLoad512, kMsbMask64, TMP[23]);

      TRANSPOSE_OUT_512(TMP[20], TMP[21], TMP[22], TMP[23], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[20] = _mm512_shuffle_epi8(TMP[20], M512(swapBytes));
      TMP[21] = _mm512_shuffle_epi8(TMP[21], M512(swapBytes));
      TMP[22] = _mm512_shuffle_epi8(TMP[22], M512(swapBytes));
      TMP[23] = _mm512_shuffle_epi8(TMP[23], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 8  * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 9  * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 10 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 11 * bytesPerLoad512);
      
      TMP[20] = _mm512_xor_si512(ciphBytes0, TMP[20]);
      TMP[21] = _mm512_xor_si512(ciphBytes1, TMP[21]);
      TMP[22] = _mm512_xor_si512(ciphBytes2, TMP[22]);
      TMP[23] = _mm512_xor_si512(ciphBytes3, TMP[23]);

      _mm512_mask_compressstoreu_epi8(pDst + 8  * bytesPerLoad512, kMsbMask64, TMP[20]);
      _mm512_mask_compressstoreu_epi8(pDst + 9  * bytesPerLoad512, kMsbMask64, TMP[21]);
      _mm512_mask_compressstoreu_epi8(pDst + 10 * bytesPerLoad512, kMsbMask64, TMP[22]);
      _mm512_mask_compressstoreu_epi8(pDst + 11 * bytesPerLoad512, kMsbMask64, TMP[23]);

      TRANSPOSE_OUT_512(TMP[20], TMP[21], TMP[22], TMP[23], TMP[12], TMP[13], TMP[14], TMP[15]);
      TMP[20] = _mm512_shuffle_epi8(TMP[20], M512(swapBytes));
      TMP[21] = _mm512_shuffle_epi8(TMP[21], M512(swapBytes));
      TMP[22] = _mm512_shuffle_epi8(TMP[22], M512(swapBytes));
      TMP[23] = _mm512_shuffle_epi8(TMP[23], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 12 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 13 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 14 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 15 * bytesPerLoad512);
      
      TMP[20] = _mm512_xor_si512(ciphBytes0, TMP[20]);
      TMP[21] = _mm512_xor_si512(ciphBytes1, TMP[21]);
      TMP[22] = _mm512_xor_si512(ciphBytes2, TMP[22]);
      TMP[23] = _mm512_xor_si512(ciphBytes3, TMP[23]);

      _mm512_mask_compressstoreu_epi8(pDst + 12 * bytesPerLoad512, kMsbMask64, TMP[20]);
      _mm512_mask_compressstoreu_epi8(pDst + 13 * bytesPerLoad512, kMsbMask64, TMP[21]);
      _mm512_mask_compressstoreu_epi8(pDst + 14 * bytesPerLoad512, kMsbMask64, TMP[22]);
      _mm512_mask_compressstoreu_epi8(pDst + 15 * bytesPerLoad512, kMsbMask64, TMP[23]);

      pSrc += 16 * bytesPerLoad512;
      pDst += 16 * bytesPerLoad512;
   }

   /* clear secret data */
   for(Ipp32u i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i],TMP[i]);
   }

   len -= processedLen;

   if (len){
      _mm_storeu_si128((__m128i*)(pIV), currentState);
      cpSMS4_CFB_dec_gfni512x48(pDst, pSrc, len, cfbBlkSize, pRKey, pIV);
   }

}

/*
// 48*cfbBlkSize bytes processing
*/

static
void cpSMS4_CFB_dec_gfni512x48(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m512i TMP[19];

   /* 
   // TMP[0..11]  - decrypted text blocks 
   // TMP[12..14] - key
   // TMP[15..18] - temp out
   */

   const int bytesPerLoad512 = 4 * cfbBlkSize;
   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   int processedLen = len - (len % (48 * cfbBlkSize));
   int blocks;

   /* load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register */
   __mmask64 kLsbMask64 = broadcast_16to64((Ipp16u)(0xFFFF << (16-cfbBlkSize)));
   /* same mask to load in MSB parts */
   __mmask64 kMsbMask64 = broadcast_16to64(~blocksCompressMask);

   /* load IV */
   __m128i currentState = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   for (blocks = len / cfbBlkSize; blocks >= (48); blocks -= (48)) {
      /* load cipher blocks to LSB parts of registers */
      __m512i ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 0 * bytesPerLoad512);
      __m512i ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 1 * bytesPerLoad512);
      __m512i ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 2 * bytesPerLoad512);
      __m512i ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 3 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[0], TMP[1], TMP[2], TMP[3], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 4 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 5 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 6 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 7 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 8  * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 9  * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 10 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 11 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[8], TMP[9], TMP[10], TMP[11], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      int itr;
      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[14] = TMP[13] = TMP[12] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[1] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[2] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[3] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[5] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[6] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[7] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[9] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[10]);
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[11]);
         /* Sbox */
         TMP[12] = sBox512(TMP[12]);
         TMP[13] = sBox512(TMP[13]);
         TMP[14] = sBox512(TMP[14]);
         /* Sbox done, now L */
         TMP[0]  = _mm512_xor_si512(_mm512_xor_si512(TMP[0],  TMP[12]), L512(TMP[12]));
         TMP[4]  = _mm512_xor_si512(_mm512_xor_si512(TMP[4],  TMP[13]), L512(TMP[13]));
         TMP[8]  = _mm512_xor_si512(_mm512_xor_si512(TMP[8],  TMP[14]), L512(TMP[14]));

         /* initial xors */
         TMP[14] = TMP[13] = TMP[12] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[2] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[3] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[0] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[6] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[7] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[4] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[10]);
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[11]);
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[8] );
         /* Sbox */
         TMP[12] = sBox512(TMP[12]);
         TMP[13] = sBox512(TMP[13]);
         TMP[14] = sBox512(TMP[14]);
         /* Sbox done, now L */
         TMP[1]  = _mm512_xor_si512(_mm512_xor_si512(TMP[1],  TMP[12]), L512(TMP[12]));
         TMP[5]  = _mm512_xor_si512(_mm512_xor_si512(TMP[5],  TMP[13]), L512(TMP[13]));
         TMP[9]  = _mm512_xor_si512(_mm512_xor_si512(TMP[9],  TMP[14]), L512(TMP[14]));

         /* initial xors */
         TMP[14] = TMP[13] = TMP[12] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[3] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[0] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[1] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[7] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[4] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[5] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[11]);
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[8] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[9] );
         /* Sbox */
         TMP[12] = sBox512(TMP[12]);
         TMP[13] = sBox512(TMP[13]);
         TMP[14] = sBox512(TMP[14]);
         /* Sbox done, now L */
         TMP[2]  = _mm512_xor_si512(_mm512_xor_si512(TMP[2],  TMP[12]), L512(TMP[12]));
         TMP[6]  = _mm512_xor_si512(_mm512_xor_si512(TMP[6],  TMP[13]), L512(TMP[13]));
         TMP[10] = _mm512_xor_si512(_mm512_xor_si512(TMP[10], TMP[14]), L512(TMP[14]));

         /* initial xors */
         TMP[14] = TMP[13] = TMP[12] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[0] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[1] );
         TMP[12] = _mm512_xor_si512(TMP[12], TMP[2] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[4] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[5] );
         TMP[13] = _mm512_xor_si512(TMP[13], TMP[6] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[8] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[9] );
         TMP[14] = _mm512_xor_si512(TMP[14], TMP[10]);
         /* Sbox */
         TMP[12] = sBox512(TMP[12]);
         TMP[13] = sBox512(TMP[13]);
         TMP[14] = sBox512(TMP[14]);
         /* Sbox done, now L */
         TMP[3] = _mm512_xor_si512(_mm512_xor_si512(TMP[3],   TMP[12]), L512(TMP[12]));
         TMP[7] = _mm512_xor_si512(_mm512_xor_si512(TMP[7],   TMP[13]), L512(TMP[13]));
         TMP[11] = _mm512_xor_si512(_mm512_xor_si512(TMP[11], TMP[14]), L512(TMP[14]));
      }
      pRKey -= 32;

      TRANSPOSE_OUT_512(TMP[15], TMP[16], TMP[17], TMP[18], TMP[0], TMP[1], TMP[2], TMP[3]);
      TMP[15] = _mm512_shuffle_epi8(TMP[15], M512(swapBytes));
      TMP[16] = _mm512_shuffle_epi8(TMP[16], M512(swapBytes));
      TMP[17] = _mm512_shuffle_epi8(TMP[17], M512(swapBytes));
      TMP[18] = _mm512_shuffle_epi8(TMP[18], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 0 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 1 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 2 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 3 * bytesPerLoad512);
      
      TMP[15] = _mm512_xor_si512(ciphBytes0, TMP[15]);
      TMP[16] = _mm512_xor_si512(ciphBytes1, TMP[16]);
      TMP[17] = _mm512_xor_si512(ciphBytes2, TMP[17]);
      TMP[18] = _mm512_xor_si512(ciphBytes3, TMP[18]);

      _mm512_mask_compressstoreu_epi8(pDst + 0 * bytesPerLoad512, kMsbMask64, TMP[15]);
      _mm512_mask_compressstoreu_epi8(pDst + 1 * bytesPerLoad512, kMsbMask64, TMP[16]);
      _mm512_mask_compressstoreu_epi8(pDst + 2 * bytesPerLoad512, kMsbMask64, TMP[17]);
      _mm512_mask_compressstoreu_epi8(pDst + 3 * bytesPerLoad512, kMsbMask64, TMP[18]);

      TRANSPOSE_OUT_512(TMP[15], TMP[16], TMP[17], TMP[18], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[15] = _mm512_shuffle_epi8(TMP[15], M512(swapBytes));
      TMP[16] = _mm512_shuffle_epi8(TMP[16], M512(swapBytes));
      TMP[17] = _mm512_shuffle_epi8(TMP[17], M512(swapBytes));
      TMP[18] = _mm512_shuffle_epi8(TMP[18], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 4 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 5 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 6 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 7 * bytesPerLoad512);
      
      TMP[15] = _mm512_xor_si512(ciphBytes0, TMP[15]);
      TMP[16] = _mm512_xor_si512(ciphBytes1, TMP[16]);
      TMP[17] = _mm512_xor_si512(ciphBytes2, TMP[17]);
      TMP[18] = _mm512_xor_si512(ciphBytes3, TMP[18]);

      _mm512_mask_compressstoreu_epi8(pDst + 4 * bytesPerLoad512, kMsbMask64, TMP[15]);
      _mm512_mask_compressstoreu_epi8(pDst + 5 * bytesPerLoad512, kMsbMask64, TMP[16]);
      _mm512_mask_compressstoreu_epi8(pDst + 6 * bytesPerLoad512, kMsbMask64, TMP[17]);
      _mm512_mask_compressstoreu_epi8(pDst + 7 * bytesPerLoad512, kMsbMask64, TMP[18]);

      TRANSPOSE_OUT_512(TMP[15], TMP[16], TMP[17], TMP[18], TMP[8], TMP[9], TMP[10], TMP[11]);
      TMP[15] = _mm512_shuffle_epi8(TMP[15], M512(swapBytes));
      TMP[16] = _mm512_shuffle_epi8(TMP[16], M512(swapBytes));
      TMP[17] = _mm512_shuffle_epi8(TMP[17], M512(swapBytes));
      TMP[18] = _mm512_shuffle_epi8(TMP[18], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 8  * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 9  * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 10 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 11 * bytesPerLoad512);
      
      TMP[15] = _mm512_xor_si512(ciphBytes0, TMP[15]);
      TMP[16] = _mm512_xor_si512(ciphBytes1, TMP[16]);
      TMP[17] = _mm512_xor_si512(ciphBytes2, TMP[17]);
      TMP[18] = _mm512_xor_si512(ciphBytes3, TMP[18]);

      _mm512_mask_compressstoreu_epi8(pDst + 8  * bytesPerLoad512, kMsbMask64, TMP[15]);
      _mm512_mask_compressstoreu_epi8(pDst + 9  * bytesPerLoad512, kMsbMask64, TMP[16]);
      _mm512_mask_compressstoreu_epi8(pDst + 10 * bytesPerLoad512, kMsbMask64, TMP[17]);
      _mm512_mask_compressstoreu_epi8(pDst + 11 * bytesPerLoad512, kMsbMask64, TMP[18]);

      pSrc += 12 * bytesPerLoad512;
      pDst += 12 * bytesPerLoad512;
   }

   /* clear secret data */
   for(Ipp32u i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i],TMP[i]);
   }

   len -= processedLen;

   if (len){
      _mm_storeu_si128((__m128i*)(pIV), currentState);
      cpSMS4_CFB_dec_gfni512x32(pDst, pSrc, len, cfbBlkSize, pRKey, pIV);
   }

}

/*
// 32*cfbBlkSize bytes processing
*/

static
void cpSMS4_CFB_dec_gfni512x32(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m512i TMP[15];

   /* 
   // TMP[0..7]   - decrypted text blocks 
   // TMP[8..9]   - key
   // TMP[10..14] - temp out
   */

   const int bytesPerLoad512 = 4 * cfbBlkSize;
   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   int processedLen = len - (len % (32 * cfbBlkSize));
   int blocks;

   /* load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register */
   __mmask64 kLsbMask64 = broadcast_16to64((Ipp16u)(0xFFFF << (16-cfbBlkSize)));
   /* same mask to load in MSB parts */
   __mmask64 kMsbMask64 = broadcast_16to64(~blocksCompressMask);

   /* load IV */
   __m128i currentState = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   for (blocks = len / cfbBlkSize; blocks >= (32); blocks -= (32)) {
      /* load cipher blocks to LSB parts of registers */
      __m512i ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 0 * bytesPerLoad512);
      __m512i ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 1 * bytesPerLoad512);
      __m512i ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 2 * bytesPerLoad512);
      __m512i ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 3 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[0], TMP[1], TMP[2], TMP[3], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 4 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 5 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 6 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 7 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));
      TRANSPOSE_INP_512(TMP[4], TMP[5], TMP[6], TMP[7], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);
      
      int itr;
      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[9] = TMP[8] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[1] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[2] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[3] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[5] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[6] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[7] );
         /* Sbox */
         TMP[8] = sBox512(TMP[8]);
         TMP[9] = sBox512(TMP[9]);
         /* Sbox done, now L */
         TMP[0]  = _mm512_xor_si512(_mm512_xor_si512(TMP[0],  TMP[8]), L512(TMP[8]));
         TMP[4]  = _mm512_xor_si512(_mm512_xor_si512(TMP[4],  TMP[9]), L512(TMP[9]));

         /* initial xors */
         TMP[9] = TMP[8] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[2] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[3] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[0] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[6] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[7] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[4] );
         /* Sbox */
         TMP[8] = sBox512(TMP[8]);
         TMP[9] = sBox512(TMP[9]);
         /* Sbox done, now L */
         TMP[1]  = _mm512_xor_si512(_mm512_xor_si512(TMP[1],  TMP[8]), L512(TMP[8]));
         TMP[5]  = _mm512_xor_si512(_mm512_xor_si512(TMP[5],  TMP[9]), L512(TMP[9]));

         /* initial xors */
         TMP[9] = TMP[8] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[3] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[0] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[1] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[7] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[4] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[5] );
         /* Sbox */
         TMP[8] = sBox512(TMP[8]);
         TMP[9] = sBox512(TMP[9]);
         /* Sbox done, now L */
         TMP[2]  = _mm512_xor_si512(_mm512_xor_si512(TMP[2],  TMP[8]), L512(TMP[8]));
         TMP[6]  = _mm512_xor_si512(_mm512_xor_si512(TMP[6],  TMP[9]), L512(TMP[9]));

         /* initial xors */
         TMP[9] = TMP[8] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[0] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[1] );
         TMP[8] = _mm512_xor_si512(TMP[8], TMP[2] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[4] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[5] );
         TMP[9] = _mm512_xor_si512(TMP[9], TMP[6] );
         /* Sbox */
         TMP[8] = sBox512(TMP[8]);
         TMP[9] = sBox512(TMP[9]);
         /* Sbox done, now L */
         TMP[3] = _mm512_xor_si512(_mm512_xor_si512(TMP[3],   TMP[8]), L512(TMP[8]));
         TMP[7] = _mm512_xor_si512(_mm512_xor_si512(TMP[7],   TMP[9]), L512(TMP[9]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_512(TMP[10], TMP[11], TMP[12], TMP[13], TMP[0], TMP[1], TMP[2], TMP[3]);
      TMP[10] = _mm512_shuffle_epi8(TMP[10], M512(swapBytes));
      TMP[11] = _mm512_shuffle_epi8(TMP[11], M512(swapBytes));
      TMP[12] = _mm512_shuffle_epi8(TMP[12], M512(swapBytes));
      TMP[13] = _mm512_shuffle_epi8(TMP[13], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 0 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 1 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 2 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 3 * bytesPerLoad512);
      
      TMP[10] = _mm512_xor_si512(ciphBytes0, TMP[10]);
      TMP[11] = _mm512_xor_si512(ciphBytes1, TMP[11]);
      TMP[12] = _mm512_xor_si512(ciphBytes2, TMP[12]);
      TMP[13] = _mm512_xor_si512(ciphBytes3, TMP[13]);

      _mm512_mask_compressstoreu_epi8(pDst + 0 * bytesPerLoad512, kMsbMask64, TMP[10]);
      _mm512_mask_compressstoreu_epi8(pDst + 1 * bytesPerLoad512, kMsbMask64, TMP[11]);
      _mm512_mask_compressstoreu_epi8(pDst + 2 * bytesPerLoad512, kMsbMask64, TMP[12]);
      _mm512_mask_compressstoreu_epi8(pDst + 3 * bytesPerLoad512, kMsbMask64, TMP[13]);

      TRANSPOSE_OUT_512(TMP[10], TMP[11], TMP[12], TMP[13], TMP[4], TMP[5], TMP[6], TMP[7]);
      TMP[10] = _mm512_shuffle_epi8(TMP[10], M512(swapBytes));
      TMP[11] = _mm512_shuffle_epi8(TMP[11], M512(swapBytes));
      TMP[12] = _mm512_shuffle_epi8(TMP[12], M512(swapBytes));
      TMP[13] = _mm512_shuffle_epi8(TMP[13], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 4 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 5 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 6 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 7 * bytesPerLoad512);
      
      TMP[10] = _mm512_xor_si512(ciphBytes0, TMP[10]);
      TMP[11] = _mm512_xor_si512(ciphBytes1, TMP[11]);
      TMP[12] = _mm512_xor_si512(ciphBytes2, TMP[12]);
      TMP[13] = _mm512_xor_si512(ciphBytes3, TMP[13]);

      _mm512_mask_compressstoreu_epi8(pDst + 4 * bytesPerLoad512, kMsbMask64, TMP[10]);
      _mm512_mask_compressstoreu_epi8(pDst + 5 * bytesPerLoad512, kMsbMask64, TMP[11]);
      _mm512_mask_compressstoreu_epi8(pDst + 6 * bytesPerLoad512, kMsbMask64, TMP[12]);
      _mm512_mask_compressstoreu_epi8(pDst + 7 * bytesPerLoad512, kMsbMask64, TMP[13]);

      pSrc += 8 * bytesPerLoad512;
      pDst += 8 * bytesPerLoad512;

   }

   /* clear secret data */
   for(Ipp32u i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i],TMP[i]);
   }

   len -= processedLen;

   if (len){
     _mm_storeu_si128((__m128i*)(pIV), currentState);
     cpSMS4_CFB_dec_gfni512x16(pDst, pSrc, len, cfbBlkSize, pRKey, pIV);
   }

}

/*
// 16*cfbBlkSize bytes processing
*/

static 
void cpSMS4_CFB_dec_gfni512x16(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m512i TMP[9];

   /* 
   // TMP[0..3] - decrypted text blocks 
   // TMP[4]    - key
   // TMP[5..8] - temp out
   */

   const int bytesPerLoad512 = 4 * cfbBlkSize;
   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   int processedLen = len - (len % (16 * cfbBlkSize));
   int blocks;

   /* load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register */
   __mmask64 kLsbMask64 = broadcast_16to64((Ipp16u)(0xFFFF << (16-cfbBlkSize)));
   /* same mask to load in MSB parts */
   __mmask64 kMsbMask64 = broadcast_16to64(~blocksCompressMask);

   /* load IV */
   __m128i currentState = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   for (blocks = len / cfbBlkSize; blocks >= (16); blocks -= (16)) {
      /* load cipher blocks to LSB parts of registers */
      __m512i ciphBytes0 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 0 * bytesPerLoad512);
      __m512i ciphBytes1 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 1 * bytesPerLoad512);
      __m512i ciphBytes2 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 2 * bytesPerLoad512);
      __m512i ciphBytes3 = _mm512_maskz_expandloadu_epi8(kLsbMask64, pSrc + 3 * bytesPerLoad512);

      /* prepare InputBlocks for decryption */
      ciphBytes0 = getInputBlocks(&currentState, &ciphBytes0, blocksCompressMask);
      ciphBytes1 = getInputBlocks(&currentState, &ciphBytes1, blocksCompressMask);
      ciphBytes2 = getInputBlocks(&currentState, &ciphBytes2, blocksCompressMask);
      ciphBytes3 = getInputBlocks(&currentState, &ciphBytes3, blocksCompressMask);

      ciphBytes0 = _mm512_shuffle_epi8(ciphBytes0, M512(swapBytes));
      ciphBytes1 = _mm512_shuffle_epi8(ciphBytes1, M512(swapBytes));
      ciphBytes2 = _mm512_shuffle_epi8(ciphBytes2, M512(swapBytes));
      ciphBytes3 = _mm512_shuffle_epi8(ciphBytes3, M512(swapBytes));

      TRANSPOSE_INP_512(TMP[0], TMP[1], TMP[2], TMP[3], ciphBytes0, ciphBytes1, ciphBytes2, ciphBytes3);
      int itr;
      for (itr = 0; itr < 8; itr++, pRKey += 4) {
         /* initial xors */
         TMP[4] = _mm512_set1_epi32((Ipp32s)pRKey[0]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[1]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[2]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[3]);
         /* Sbox */
         TMP[4] = sBox512(TMP[4]);
         /* Sbox done, now L */
         TMP[0] = _mm512_xor_si512(_mm512_xor_si512(TMP[0], TMP[4]), L512(TMP[4]));

         /* initial xors */
         TMP[4] = _mm512_set1_epi32((Ipp32s)pRKey[1]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[2]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[3]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[0]);
         /* Sbox */
         TMP[4] = sBox512(TMP[4]);
         /* Sbox done, now L */
         TMP[1] = _mm512_xor_si512(_mm512_xor_si512(TMP[1], TMP[4]), L512(TMP[4]));

         /* initial xors */
         TMP[4] = _mm512_set1_epi32((Ipp32s)pRKey[2]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[3]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[0]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[1]);
         /* Sbox */
         TMP[4] = sBox512(TMP[4]);
         /* Sbox done, now L */
         TMP[2] = _mm512_xor_si512(_mm512_xor_si512(TMP[2], TMP[4]), L512(TMP[4]));

         /* initial xors */
         TMP[4] = _mm512_set1_epi32((Ipp32s)pRKey[3]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[0]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[1]);
         TMP[4] = _mm512_xor_si512(TMP[4], TMP[2]);
         /* Sbox */
         TMP[4] = sBox512(TMP[4]);
         /* Sbox done, now L */
         TMP[3] = _mm512_xor_si512(_mm512_xor_si512(TMP[3], TMP[4]), L512(TMP[4]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_512(TMP[5], TMP[6], TMP[7], TMP[8], TMP[0], TMP[1], TMP[2], TMP[3]);
      TMP[5] = _mm512_shuffle_epi8(TMP[5], M512(swapBytes));
      TMP[6] = _mm512_shuffle_epi8(TMP[6], M512(swapBytes));
      TMP[7] = _mm512_shuffle_epi8(TMP[7], M512(swapBytes));
      TMP[8] = _mm512_shuffle_epi8(TMP[8], M512(swapBytes));

      /* load cipher blocks to MSB parts of registers */
      ciphBytes0 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 0 * bytesPerLoad512);
      ciphBytes1 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 1 * bytesPerLoad512);
      ciphBytes2 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 2 * bytesPerLoad512);
      ciphBytes3 = _mm512_maskz_expandloadu_epi8(kMsbMask64, pSrc + 3 * bytesPerLoad512);

      TMP[5] = _mm512_xor_si512(ciphBytes0, TMP[5]);
      TMP[6] = _mm512_xor_si512(ciphBytes1, TMP[6]);
      TMP[7] = _mm512_xor_si512(ciphBytes2, TMP[7]);
      TMP[8] = _mm512_xor_si512(ciphBytes3, TMP[8]);

      _mm512_mask_compressstoreu_epi8(pDst + 0 * bytesPerLoad512, kMsbMask64, TMP[5]);
      _mm512_mask_compressstoreu_epi8(pDst + 1 * bytesPerLoad512, kMsbMask64, TMP[6]);
      _mm512_mask_compressstoreu_epi8(pDst + 2 * bytesPerLoad512, kMsbMask64, TMP[7]);
      _mm512_mask_compressstoreu_epi8(pDst + 3 * bytesPerLoad512, kMsbMask64, TMP[8]);

      pSrc += 4 * bytesPerLoad512;
      pDst += 4 * bytesPerLoad512;
   }

   /* clear secret data */
   for(Ipp32u i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm512_setzero_si512(); //_mm512_xor_si512(TMP[i],TMP[i]);
   }

   len -= processedLen;

   if (len){
      _mm_storeu_si128((__m128i*)(pIV), currentState);
      cpSMS4_CFB_dec_gfni128x12(pDst, pSrc, len, cfbBlkSize, pRKey, pIV);
   }

}

/*
// 12*cfbBlkSize bytes processing
*/

static 
void cpSMS4_CFB_dec_gfni128x12(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV)
{
   __ALIGN16 __m128i TMP[15];

   /* 
   // TMP[0..11]  - decrypted text blocks 
   // TMP[12..14] - key
   */

   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   int processedLen = len - (len % (12 * cfbBlkSize));
   int blocks;

   /* load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register */
   __mmask16 kLsbMask16 = (Ipp16u)(0xFFFF << (16-cfbBlkSize));
   /* same mask to load in MSB parts */
   __mmask16 kMsbMask16 = ~blocksCompressMask;

   /* load IV */
   __m128i IV128 = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   __m128i currentState = IV128;

   for (blocks = len / cfbBlkSize; blocks >= (12); blocks -= (12)) {

      /* load cipher blocks to LSB parts of registers */
      __m128i ciphBytes0 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 0 * cfbBlkSize);
      __m128i ciphBytes1 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 1 * cfbBlkSize);
      __m128i ciphBytes2 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 2 * cfbBlkSize);
      __m128i ciphBytes3 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 3 * cfbBlkSize);

      /*  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit */
      TMP[0] = currentState;
      /* drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits */
      TMP[1] = _mm_mask_compress_epi8(ciphBytes0, blocksCompressMask, TMP[0]);
      TMP[2] = _mm_mask_compress_epi8(ciphBytes1, blocksCompressMask, TMP[1]);
      TMP[3] = _mm_mask_compress_epi8(ciphBytes2, blocksCompressMask, TMP[2]);

      /* next InputBlock ready */
      currentState = _mm_mask_compress_epi8(ciphBytes3, blocksCompressMask, TMP[3]);

      TMP[0] = _mm_shuffle_epi8(TMP[0], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12]); /* TMP[12] - buffer */

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 4 * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 5 * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 6 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 7 * cfbBlkSize);

      /*  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit */
      TMP[4] = currentState;
      /* drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits */
      TMP[5] = _mm_mask_compress_epi8(ciphBytes0, blocksCompressMask, TMP[4]);
      TMP[6] = _mm_mask_compress_epi8(ciphBytes1, blocksCompressMask, TMP[5]);
      TMP[7] = _mm_mask_compress_epi8(ciphBytes2, blocksCompressMask, TMP[6]);

      /* next InputBlock ready */
      currentState = _mm_mask_compress_epi8(ciphBytes3, blocksCompressMask, TMP[7]);

      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[4], TMP[5], TMP[6], TMP[7], TMP[12]); /* TMP[12] - buffer */

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 8  * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 9  * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 10 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 11 * cfbBlkSize);

      /*  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit */
      TMP[8]  = currentState;
      /* drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits */
      TMP[9]  = _mm_mask_compress_epi8(ciphBytes0, blocksCompressMask, TMP[8] );
      TMP[10] = _mm_mask_compress_epi8(ciphBytes1, blocksCompressMask, TMP[9] );
      TMP[11] = _mm_mask_compress_epi8(ciphBytes2, blocksCompressMask, TMP[10]);

      /* next InputBlock ready */
      currentState = _mm_mask_compress_epi8(ciphBytes3, blocksCompressMask, TMP[11]);

      TMP[8]  = _mm_shuffle_epi8(TMP[8],  M128(swapBytes));
      TMP[9]  = _mm_shuffle_epi8(TMP[9],  M128(swapBytes));
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[8], TMP[9], TMP[10], TMP[11], TMP[12]); /* TMP[12] - buffer */

   
      int itr;
      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[13] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[14] = TMP[13];
         TMP[12] = TMP[13];
         TMP[12] = _mm_xor_si128(TMP[12], TMP[1] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[2] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[3] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[5] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[6] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[7] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[9] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[10]);
         TMP[14] = _mm_xor_si128(TMP[14], TMP[11]);
         /* Sbox */
         TMP[12] = sBox128(TMP[12]);
         TMP[13] = sBox128(TMP[13]);
         TMP[14] = sBox128(TMP[14]);
         /* Sbox done, now L */
         TMP[0] = _mm_xor_si128(_mm_xor_si128(TMP[0], TMP[12]), L128(TMP[12]));
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[13]), L128(TMP[13]));
         TMP[8] = _mm_xor_si128(_mm_xor_si128(TMP[8], TMP[14]), L128(TMP[14]));

         /* initial xors */
         TMP[13] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[14] = TMP[13];
         TMP[12] = TMP[13];
         TMP[12] = _mm_xor_si128(TMP[12], TMP[2] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[3] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[0] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[6] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[7] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[4] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[10]);
         TMP[14] = _mm_xor_si128(TMP[14], TMP[11]);
         TMP[14] = _mm_xor_si128(TMP[14], TMP[8] );
         /* Sbox */
         TMP[12] = sBox128(TMP[12]);
         TMP[13] = sBox128(TMP[13]);
         TMP[14] = sBox128(TMP[14]);
         /* Sbox done, now L */
         TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[12]), L128(TMP[12]));
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[13]), L128(TMP[13]));
         TMP[9] = _mm_xor_si128(_mm_xor_si128(TMP[9], TMP[14]), L128(TMP[14]));

         /* initial xors */
         TMP[13] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[14] = TMP[13];
         TMP[12] = TMP[13];
         TMP[12] = _mm_xor_si128(TMP[12], TMP[3] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[0] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[1] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[7] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[4] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[5] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[11]);
         TMP[14] = _mm_xor_si128(TMP[14], TMP[8] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[9] );
         /* Sbox */
         TMP[12] = sBox128(TMP[12]);
         TMP[13] = sBox128(TMP[13]);
         TMP[14] = sBox128(TMP[14]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[12]), L128(TMP[12]));
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[13]), L128(TMP[13]));
         TMP[10] = _mm_xor_si128(_mm_xor_si128(TMP[10], TMP[14]), L128(TMP[14]));

         /* initial xors */
         TMP[13] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[14] = TMP[13];
         TMP[12] = TMP[13];
         TMP[12] = _mm_xor_si128(TMP[12], TMP[0] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[1] );
         TMP[12] = _mm_xor_si128(TMP[12], TMP[2] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[4] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[5] );
         TMP[13] = _mm_xor_si128(TMP[13], TMP[6] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[8] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[9] );
         TMP[14] = _mm_xor_si128(TMP[14], TMP[10]);
         /* Sbox */
         TMP[12] = sBox128(TMP[12]);
         TMP[13] = sBox128(TMP[13]);
         TMP[14] = sBox128(TMP[14]);
         /* Sbox done, now L */
         TMP[3]  = _mm_xor_si128(_mm_xor_si128(TMP[3],  TMP[12]), L128(TMP[12]));
         TMP[7]  = _mm_xor_si128(_mm_xor_si128(TMP[7],  TMP[13]), L128(TMP[13]));
         TMP[11] = _mm_xor_si128(_mm_xor_si128(TMP[11], TMP[14]), L128(TMP[14]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[0], TMP[1], TMP[2], TMP[3], TMP[12]); /* TMP[12] - buffer */
      /* Order of blocks is inverted */

      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      TMP[0] = _mm_shuffle_epi8(TMP[0], M128(swapBytes));

      ciphBytes0 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 0 * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 1 * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 2 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 3 * cfbBlkSize);

      TMP[3] = _mm_xor_si128(ciphBytes0, TMP[3]);
      TMP[2] = _mm_xor_si128(ciphBytes1, TMP[2]);
      TMP[1] = _mm_xor_si128(ciphBytes2, TMP[1]);
      TMP[0] = _mm_xor_si128(ciphBytes3, TMP[0]);

      _mm_mask_compressstoreu_epi8(pDst + 0 * cfbBlkSize, kMsbMask16, TMP[3]);
      _mm_mask_compressstoreu_epi8(pDst + 1 * cfbBlkSize, kMsbMask16, TMP[2]);
      _mm_mask_compressstoreu_epi8(pDst + 2 * cfbBlkSize, kMsbMask16, TMP[1]);
      _mm_mask_compressstoreu_epi8(pDst + 3 * cfbBlkSize, kMsbMask16, TMP[0]);

      TRANSPOSE_OUT_128(TMP[4], TMP[5], TMP[6], TMP[7], TMP[12]); /* TMP[12] - buffer */
      /* Order of blocks is inverted */

      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));

      ciphBytes0 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 4 * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 5 * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 6 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 7 * cfbBlkSize);

      TMP[7] = _mm_xor_si128(ciphBytes0, TMP[7]);
      TMP[6] = _mm_xor_si128(ciphBytes1, TMP[6]);
      TMP[5] = _mm_xor_si128(ciphBytes2, TMP[5]);
      TMP[4] = _mm_xor_si128(ciphBytes3, TMP[4]);

      _mm_mask_compressstoreu_epi8(pDst + 4 * cfbBlkSize, kMsbMask16, TMP[7]);
      _mm_mask_compressstoreu_epi8(pDst + 5 * cfbBlkSize, kMsbMask16, TMP[6]);
      _mm_mask_compressstoreu_epi8(pDst + 6 * cfbBlkSize, kMsbMask16, TMP[5]);
      _mm_mask_compressstoreu_epi8(pDst + 7 * cfbBlkSize, kMsbMask16, TMP[4]);

      TRANSPOSE_OUT_128(TMP[8], TMP[9], TMP[10], TMP[11], TMP[12]); /* TMP[12] - buffer */
      /* Order of blocks is inverted */

      TMP[11] = _mm_shuffle_epi8(TMP[11], M128(swapBytes));
      TMP[10] = _mm_shuffle_epi8(TMP[10], M128(swapBytes));
      TMP[9]  = _mm_shuffle_epi8(TMP[9],  M128(swapBytes));
      TMP[8]  = _mm_shuffle_epi8(TMP[8],  M128(swapBytes));

      ciphBytes0 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 8  * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 9  * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 10 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 11 * cfbBlkSize);

      TMP[11] = _mm_xor_si128(ciphBytes0, TMP[11]);
      TMP[10] = _mm_xor_si128(ciphBytes1, TMP[10]);
      TMP[9]  = _mm_xor_si128(ciphBytes2, TMP[9]);
      TMP[8]  = _mm_xor_si128(ciphBytes3, TMP[8]);

      _mm_mask_compressstoreu_epi8(pDst + 8  * cfbBlkSize, kMsbMask16, TMP[11]);
      _mm_mask_compressstoreu_epi8(pDst + 9  * cfbBlkSize, kMsbMask16, TMP[10]);
      _mm_mask_compressstoreu_epi8(pDst + 10 * cfbBlkSize, kMsbMask16, TMP[9]);
      _mm_mask_compressstoreu_epi8(pDst + 11 * cfbBlkSize, kMsbMask16, TMP[8]);
   
      pSrc += 12 * cfbBlkSize;
      pDst += 12 * cfbBlkSize;
   }

   /* clear secret data */
   for(Ipp32u i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   len -= processedLen;

   if (len){
      _mm_storeu_si128((__m128i*)(pIV), currentState);
      cpSMS4_CFB_dec_gfni128x8(pDst, pSrc, len, cfbBlkSize, pRKey, pIV);
   }
}

/*
// 8*cfbBlkSize bytes processing
*/

static 
void cpSMS4_CFB_dec_gfni128x8(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV)
{

   __ALIGN16 __m128i TMP[10];

   /* 
   // TMP[0..7] - decrypted text blocks 
   // TMP[8..9] - key
   */

   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   int processedLen = len - (len % (8 * cfbBlkSize));
   int blocks;

   /* load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register */
   __mmask16 kLsbMask16 = (Ipp16u)(0xFFFF << (16-cfbBlkSize));
   /* same mask to load in MSB parts */
   __mmask16 kMsbMask16 = ~blocksCompressMask;

   /* load IV */
   __m128i currentState = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   for (blocks = len / cfbBlkSize; blocks >= (8); blocks -= (8)) {

      /* load cipher blocks to LSB parts of registers */
      __m128i ciphBytes0 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 0 * cfbBlkSize);
      __m128i ciphBytes1 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 1 * cfbBlkSize);
      __m128i ciphBytes2 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 2 * cfbBlkSize);
      __m128i ciphBytes3 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 3 * cfbBlkSize);

      /*  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit */
      TMP[0] = currentState;
      /* drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits */
      TMP[1] = _mm_mask_compress_epi8(ciphBytes0, blocksCompressMask, TMP[0]);
      TMP[2] = _mm_mask_compress_epi8(ciphBytes1, blocksCompressMask, TMP[1]);
      TMP[3] = _mm_mask_compress_epi8(ciphBytes2, blocksCompressMask, TMP[2]);

      /* next InputBlock ready */
      currentState = _mm_mask_compress_epi8(ciphBytes3, blocksCompressMask, TMP[3]);

      TMP[0] = _mm_shuffle_epi8(TMP[0], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8]); /* TMP[8] - buffer */

      /* load cipher blocks to LSB parts of registers */
      ciphBytes0 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 4 * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 5 * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 6 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 7 * cfbBlkSize);

      /*  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit */
      TMP[4] = currentState;
      /* drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits */
      TMP[5] = _mm_mask_compress_epi8(ciphBytes0, blocksCompressMask, TMP[4]);
      TMP[6] = _mm_mask_compress_epi8(ciphBytes1, blocksCompressMask, TMP[5]);
      TMP[7] = _mm_mask_compress_epi8(ciphBytes2, blocksCompressMask, TMP[6]);

      /* next InputBlock ready */
      currentState = _mm_mask_compress_epi8(ciphBytes3, blocksCompressMask, TMP[7]);

      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[4], TMP[5], TMP[6], TMP[7], TMP[8]); /* TMP[8] - buffer */
   
      int itr;
      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[9] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[8] = TMP[9];
         TMP[8] = _mm_xor_si128(TMP[8], TMP[1] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[2] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[3] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[5] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[6] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[7] );
         /* Sbox */
         TMP[8] = sBox128(TMP[8]);
         TMP[9] = sBox128(TMP[9]);
         /* Sbox done, now L */
         TMP[0] = _mm_xor_si128(_mm_xor_si128(TMP[0], TMP[8]), L128(TMP[8]));
         TMP[4] = _mm_xor_si128(_mm_xor_si128(TMP[4], TMP[9]), L128(TMP[9]));

         /* initial xors */
         TMP[9] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[8] = TMP[9];
         TMP[8] = _mm_xor_si128(TMP[8], TMP[2] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[3] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[0] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[6] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[7] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[4] );
         /* Sbox */
         TMP[8] = sBox128(TMP[8]);
         TMP[9] = sBox128(TMP[9]);
         /* Sbox done, now L */
         TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[8]), L128(TMP[8]));
         TMP[5] = _mm_xor_si128(_mm_xor_si128(TMP[5], TMP[9]), L128(TMP[9]));

         /* initial xors */
         TMP[9] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[8] = TMP[9];
         TMP[8] = _mm_xor_si128(TMP[8], TMP[3] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[0] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[1] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[7] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[4] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[5] );
         /* Sbox */
         TMP[8] = sBox128(TMP[8]);
         TMP[9] = sBox128(TMP[9]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[8]), L128(TMP[8]));
         TMP[6] = _mm_xor_si128(_mm_xor_si128(TMP[6], TMP[9]), L128(TMP[9]));

         /* initial xors */
         TMP[9] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[8] = TMP[9];
         TMP[8] = _mm_xor_si128(TMP[8], TMP[0] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[1] );
         TMP[8] = _mm_xor_si128(TMP[8], TMP[2] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[4] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[5] );
         TMP[9] = _mm_xor_si128(TMP[9], TMP[6] );
         /* Sbox */
         TMP[8] = sBox128(TMP[8]);
         TMP[9] = sBox128(TMP[9]);
         /* Sbox done, now L */
         TMP[3]  = _mm_xor_si128(_mm_xor_si128(TMP[3],  TMP[8]), L128(TMP[8]));
         TMP[7]  = _mm_xor_si128(_mm_xor_si128(TMP[7],  TMP[9]), L128(TMP[9]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[0], TMP[1], TMP[2], TMP[3], TMP[8]); /* TMP[8] - buffer */
      /* Order of blocks is inverted */

      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      TMP[0] = _mm_shuffle_epi8(TMP[0], M128(swapBytes));

      ciphBytes0 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 0 * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 1 * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 2 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 3 * cfbBlkSize);

      TMP[3] = _mm_xor_si128(ciphBytes0, TMP[3]);
      TMP[2] = _mm_xor_si128(ciphBytes1, TMP[2]);
      TMP[1] = _mm_xor_si128(ciphBytes2, TMP[1]);
      TMP[0] = _mm_xor_si128(ciphBytes3, TMP[0]);

      _mm_mask_compressstoreu_epi8(pDst + 0 * cfbBlkSize, kMsbMask16, TMP[3]);
      _mm_mask_compressstoreu_epi8(pDst + 1 * cfbBlkSize, kMsbMask16, TMP[2]);
      _mm_mask_compressstoreu_epi8(pDst + 2 * cfbBlkSize, kMsbMask16, TMP[1]);
      _mm_mask_compressstoreu_epi8(pDst + 3 * cfbBlkSize, kMsbMask16, TMP[0]);

      TRANSPOSE_OUT_128(TMP[4], TMP[5], TMP[6], TMP[7], TMP[8]); /* TMP[8] - buffer */
      /* Order of blocks is inverted */

      TMP[7] = _mm_shuffle_epi8(TMP[7], M128(swapBytes));
      TMP[6] = _mm_shuffle_epi8(TMP[6], M128(swapBytes));
      TMP[5] = _mm_shuffle_epi8(TMP[5], M128(swapBytes));
      TMP[4] = _mm_shuffle_epi8(TMP[4], M128(swapBytes));

      ciphBytes0 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 4 * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 5 * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 6 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 7 * cfbBlkSize);

      TMP[7] = _mm_xor_si128(ciphBytes0, TMP[7]);
      TMP[6] = _mm_xor_si128(ciphBytes1, TMP[6]);
      TMP[5] = _mm_xor_si128(ciphBytes2, TMP[5]);
      TMP[4] = _mm_xor_si128(ciphBytes3, TMP[4]);

      _mm_mask_compressstoreu_epi8(pDst + 4 * cfbBlkSize, kMsbMask16, TMP[7]);
      _mm_mask_compressstoreu_epi8(pDst + 5 * cfbBlkSize, kMsbMask16, TMP[6]);
      _mm_mask_compressstoreu_epi8(pDst + 6 * cfbBlkSize, kMsbMask16, TMP[5]);
      _mm_mask_compressstoreu_epi8(pDst + 7 * cfbBlkSize, kMsbMask16, TMP[4]);
   
      pSrc += 8 * cfbBlkSize;
      pDst += 8 * cfbBlkSize;
   }

   /* clear secret data */
   for(Ipp32u i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   len -= processedLen;

   if (len){
      _mm_storeu_si128((__m128i*)(pIV), currentState);
      cpSMS4_CFB_dec_gfni128x4(pDst, pSrc, len, cfbBlkSize, pRKey, pIV);
   }
}

/*
// 4*cfbBlkSize bytes processing
*/

static 
void cpSMS4_CFB_dec_gfni128x4(Ipp8u* pDst, const Ipp8u* pSrc, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV)
{ 
   __ALIGN16 __m128i TMP[5];

   /* 
   // TMP[0..3] - decrypted text blocks 
   // TMP[4]    - key
   */

   const Ipp16u blocksCompressMask = (Ipp16u)(0xFFFF << cfbBlkSize);

   int processedLen = len - (len % (4 * cfbBlkSize));
   int blocks;

   /* load masks; allows to load 4 source blocks of cfbBlkSize each (in bytes) to LSB parts of 128-bit lanes in 512-bit register */
   __mmask16 kLsbMask16 = (Ipp16u)(0xFFFF << (16-cfbBlkSize));
   /* same mask to load in MSB parts */
   __mmask16 kMsbMask16 = ~blocksCompressMask;

   /* load IV */
   __m128i currentState = _mm_maskz_loadu_epi64(0x03 /* load 128-bit */, pIV);

   for (blocks = len / cfbBlkSize; blocks >= (4); blocks -= (4)) {

      /* load cipher blocks to LSB parts of registers */
      __m128i ciphBytes0 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 0 * cfbBlkSize);
      __m128i ciphBytes1 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 1 * cfbBlkSize);
      __m128i ciphBytes2 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 2 * cfbBlkSize);
      __m128i ciphBytes3 = _mm_maskz_expandloadu_epi8(kLsbMask16, pSrc + 3 * cfbBlkSize);

      /*  InpBlk_j = LSB_(b-s)(InpBlk_(j-1)) | C_(j-1); b = 128 bit, s = 8*cfbBlkSize bit */
      TMP[0] = currentState;
      /* drop MSB bits (size = cfbBlkSize) from the inpBlk_i (by mask) and append c_(i-1) to LSB bits */
      TMP[1] = _mm_mask_compress_epi8(ciphBytes0, blocksCompressMask, TMP[0]);
      TMP[2] = _mm_mask_compress_epi8(ciphBytes1, blocksCompressMask, TMP[1]);
      TMP[3] = _mm_mask_compress_epi8(ciphBytes2, blocksCompressMask, TMP[2]);

      /* next InputBlock ready */
      currentState = _mm_mask_compress_epi8(ciphBytes3, blocksCompressMask, TMP[3]);

      TMP[0] = _mm_shuffle_epi8(TMP[0], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TRANSPOSE_INP_128(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4]); /* TMP[4] - buffer */
   
      int itr;
      for(itr=0; itr<8; itr++, pRKey+=4) {
         /* initial xors */
         TMP[4] = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[0]), 0);
         TMP[4] = _mm_xor_si128(TMP[4], TMP[1] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[2] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[3] );
         /* Sbox */
         TMP[4] = sBox128(TMP[4]);
         /* Sbox done, now L */
         TMP[0] = _mm_xor_si128(_mm_xor_si128(TMP[0], TMP[4]), L128(TMP[4]));

         /* initial xors */
         TMP[4] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[1]), 0);
         TMP[4] = _mm_xor_si128(TMP[4], TMP[2] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[3] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[0] );
         /* Sbox */
         TMP[4] = sBox128(TMP[4]);
         /* Sbox done, now L */
         TMP[1] = _mm_xor_si128(_mm_xor_si128(TMP[1], TMP[4]), L128(TMP[4]));

         /* initial xors */
         TMP[4] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[2]), 0);
         TMP[4] = _mm_xor_si128(TMP[4], TMP[3] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[0] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[1] );
         /* Sbox */
         TMP[4] = sBox128(TMP[4]);
         /* Sbox done, now L */
         TMP[2] = _mm_xor_si128(_mm_xor_si128(TMP[2], TMP[4]), L128(TMP[4]));

         /* initial xors */
         TMP[4] =  _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)pRKey[3]), 0);
         TMP[4] = _mm_xor_si128(TMP[4], TMP[0] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[1] );
         TMP[4] = _mm_xor_si128(TMP[4], TMP[2] );
         /* Sbox */
         TMP[4] = sBox128(TMP[4]);
         /* Sbox done, now L */
         TMP[3]  = _mm_xor_si128(_mm_xor_si128(TMP[3],  TMP[4]), L128(TMP[4]));
      }

      pRKey -= 32;

      TRANSPOSE_OUT_128(TMP[0], TMP[1], TMP[2], TMP[3], TMP[4]); /* TMP[4] - buffer */
      /* Order of blocks is inverted */

      TMP[3] = _mm_shuffle_epi8(TMP[3], M128(swapBytes));
      TMP[2] = _mm_shuffle_epi8(TMP[2], M128(swapBytes));
      TMP[1] = _mm_shuffle_epi8(TMP[1], M128(swapBytes));
      TMP[0] = _mm_shuffle_epi8(TMP[0], M128(swapBytes));

      ciphBytes0 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 0 * cfbBlkSize);
      ciphBytes1 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 1 * cfbBlkSize);
      ciphBytes2 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 2 * cfbBlkSize);
      ciphBytes3 = _mm_maskz_expandloadu_epi8(kMsbMask16, pSrc + 3 * cfbBlkSize);

      TMP[3] = _mm_xor_si128(ciphBytes0, TMP[3]);
      TMP[2] = _mm_xor_si128(ciphBytes1, TMP[2]);
      TMP[1] = _mm_xor_si128(ciphBytes2, TMP[1]);
      TMP[0] = _mm_xor_si128(ciphBytes3, TMP[0]);

      _mm_mask_compressstoreu_epi8(pDst + 0 * cfbBlkSize, kMsbMask16, TMP[3]);
      _mm_mask_compressstoreu_epi8(pDst + 1 * cfbBlkSize, kMsbMask16, TMP[2]);
      _mm_mask_compressstoreu_epi8(pDst + 2 * cfbBlkSize, kMsbMask16, TMP[1]);
      _mm_mask_compressstoreu_epi8(pDst + 3 * cfbBlkSize, kMsbMask16, TMP[0]);
   
      pSrc += 4 * cfbBlkSize;
      pDst += 4 * cfbBlkSize;
   }

   /* clear secret data */
   for(Ipp32u i = 0; i < sizeof(TMP)/sizeof(TMP[0]); i++){
      TMP[i] = _mm_setzero_si128(); //_mm_xor_si128(TMP[i],TMP[i]);
   }

   len -= processedLen;

   if (len){
      _mm_storeu_si128((__m128i*)(pIV), currentState);
   }
}

#endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */
#endif /* _IPP32E>=_IPP32E_K1 */
