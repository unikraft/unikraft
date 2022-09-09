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
//     AES-XTS VAES512 Functions (IEEE P1619)
//
//  Contents:
//        cpAESEncryptXTS_VAES()
//        cpAESDecryptXTS_VAES()
//
*/

#include "owncp.h"
#include "pcpaesmxts.h"
#include "pcptool.h"
#include "pcpaesmxtsstuff.h"

#include "pcpaes_encrypt_vaes512.h"
#include "pcpaes_decrypt_vaes512.h"

#if (_IPP32E>=_IPP32E_K1)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4310) // cast truncates constant value in MSVC
#endif

#define M512(mem)    (*((__m512i*)(mem)))

/* Generate next 4 tweaks with 2^8 multiplier */
__INLINE __m512i nextTweaks_x8(__m512i tweak128x4)
{
   const __m512i poly = _mm512_set_epi64(0, 0x87, 0, 0x87, 0, 0x87, 0, 0x87);

   __m512i highBytes = _mm512_bsrli_epi128(tweak128x4, 15);
   __m512i tmp = _mm512_clmulepi64_epi128(highBytes, poly, 0);
   tweak128x4 = _mm512_bslli_epi128(tweak128x4, 1);
   tweak128x4 = _mm512_xor_si512(tweak128x4, tmp);

   return tweak128x4;
}

/* Generate next 4 tweaks with 2^32 multiplier */
__INLINE __m512i nextTweaks_x32(__m512i tweak128x4)
{
   const __m512i poly = _mm512_set_epi64(0, 0x87, 0, 0x87, 0, 0x87, 0, 0x87);

   /* Shift 128-bit lanes right by 12 bytes */
   __m512i highBytes = _mm512_bsrli_epi128(tweak128x4, 12);

   __m512i tmp = _mm512_clmulepi64_epi128(highBytes, poly, 0);

   /* Shift 128-bit lanes left by 4 bytes */
   tweak128x4 = _mm512_bslli_epi128(tweak128x4, 4);

   /* Xor low 4 bytes in each 128-bit lane with 0x87-modified ones */
   tweak128x4 = _mm512_xor_si512(tweak128x4, tmp);

   return tweak128x4;
}

IPP_OWN_DEFN (void, cpAESEncryptXTS_VAES, (Ipp8u* outBlk, const Ipp8u* inpBlk, int nBlks, const Ipp8u* pRKey, int nr, Ipp8u* pTweak))
{
   if (0 == nBlks) {
      return; // do not modify tweak value
   }

   int cipherRounds = nr - 1;

   __m128i* pRkey = (__m128i*)pRKey;
   __m512i* pInp512 = (__m512i*)inpBlk;
   __m512i* pOut512 = (__m512i*)outBlk;

   /* Produce initial 32 tweaks */
   __ALIGN64 Ipp8u tempTweakBuffer[AES_BLK_SIZE * 4 * 8]; // 32 tweaks
   cpXTSwhitening(tempTweakBuffer, 8, pTweak); // generate 8 tweaks

   const __m512i* pInitialTweaks = (const __m512i*)tempTweakBuffer;

   int tailTweaksConsumedCount = 0;

   __m512i tweakBlk0 = M512(pInitialTweaks);
   __m512i tweakBlk1 = M512(pInitialTweaks + 1);
   __m512i tweakBlk2 = M512(pInitialTweaks + 2);
   __m512i tweakBlk3 = M512(pInitialTweaks + 3);
   __m512i tweakBlk4 = M512(pInitialTweaks + 4);
   __m512i tweakBlk5 = M512(pInitialTweaks + 5);
   __m512i tweakBlk6 = M512(pInitialTweaks + 6);
   __m512i tweakBlk7 = M512(pInitialTweaks + 7);

   // generate other 24 tweaks
   tweakBlk2 = nextTweaks_x8(tweakBlk0);
   tweakBlk3 = nextTweaks_x8(tweakBlk1);
   tweakBlk4 = nextTweaks_x8(tweakBlk2);
   tweakBlk5 = nextTweaks_x8(tweakBlk3);
   tweakBlk6 = nextTweaks_x8(tweakBlk4);
   tweakBlk7 = nextTweaks_x8(tweakBlk5);

   int blocks;
   for (blocks = nBlks; blocks >= (4 * 8); blocks -= (4 * 8)) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pInp512 + 3);
      __m512i blk4 = _mm512_loadu_si512(pInp512 + 4);
      __m512i blk5 = _mm512_loadu_si512(pInp512 + 5);
      __m512i blk6 = _mm512_loadu_si512(pInp512 + 6);
      __m512i blk7 = _mm512_loadu_si512(pInp512 + 7);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);
      blk4 = _mm512_xor_epi64(tweakBlk4, blk4);
      blk5 = _mm512_xor_epi64(tweakBlk5, blk5);
      blk6 = _mm512_xor_epi64(tweakBlk6, blk6);
      blk7 = _mm512_xor_epi64(tweakBlk7, blk7);

      cpAESEncrypt4_VAES_NI(&blk0, &blk1, &blk2, &blk3, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);

      tweakBlk0 = nextTweaks_x32(tweakBlk0);
      tweakBlk1 = nextTweaks_x32(tweakBlk1);
      tweakBlk2 = nextTweaks_x32(tweakBlk2);
      tweakBlk3 = nextTweaks_x32(tweakBlk3);

      cpAESEncrypt4_VAES_NI(&blk4, &blk5, &blk6, &blk7, pRkey, cipherRounds);

      blk4 = _mm512_xor_epi64(tweakBlk4, blk4);
      blk5 = _mm512_xor_epi64(tweakBlk5, blk5);
      blk6 = _mm512_xor_epi64(tweakBlk6, blk6);
      blk7 = _mm512_xor_epi64(tweakBlk7, blk7);

      tweakBlk4 = nextTweaks_x32(tweakBlk4);
      tweakBlk5 = nextTweaks_x32(tweakBlk5);
      tweakBlk6 = nextTweaks_x32(tweakBlk6);
      tweakBlk7 = nextTweaks_x32(tweakBlk7);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);
      _mm512_storeu_si512(pOut512 + 3, blk3);
      _mm512_storeu_si512(pOut512 + 4, blk4);
      _mm512_storeu_si512(pOut512 + 5, blk5);
      _mm512_storeu_si512(pOut512 + 6, blk6);
      _mm512_storeu_si512(pOut512 + 7, blk7);

      pInp512 += 8;
      pOut512 += 8;
   }

   if ((4 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pInp512 + 3);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);

      cpAESEncrypt4_VAES_NI(&blk0, &blk1, &blk2, &blk3, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);
      _mm512_storeu_si512(pOut512 + 3, blk3);

      tailTweaksConsumedCount += 4;
      pInp512 += 4;
      pOut512 += 4;
      blocks -= (4 * 4);
   }

   if ((3 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);
      tweakBlk1 = M512(pInitialTweaks + tailTweaksConsumedCount + 1);
      tweakBlk2 = M512(pInitialTweaks + tailTweaksConsumedCount + 2);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);

      cpAESEncrypt3_VAES_NI(&blk0, &blk1, &blk2, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);

      tailTweaksConsumedCount += 3;
      pInp512 += 3;
      pOut512 += 3;
      blocks -= (3 * 4);
   }
   else if ((2 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);
      tweakBlk1 = M512(pInitialTweaks + tailTweaksConsumedCount + 1);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);

      cpAESEncrypt2_VAES_NI(&blk0, &blk1, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);

      tailTweaksConsumedCount += 2;
      pInp512 += 2;
      pOut512 += 2;
      blocks -= (2 * 4);
   }
   else if ((1 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      cpAESEncrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      _mm512_storeu_si512(pOut512, blk0);

      tailTweaksConsumedCount += 1;
      pInp512 += 1;
      pOut512 += 1;
      blocks -= (1 * 4);
   }

   if (blocks) {
      __mmask8 k = (__mmask8)((1 << (blocks + blocks)) - 1);
      __m512i blk0 = _mm512_maskz_loadu_epi64(k, pInp512);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      cpAESEncrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      _mm512_mask_storeu_epi64(pOut512, k, blk0);
   }

   {
      __mmask8 maskTweakToReturn = (__mmask8)(((Ipp8u)0x03u << (blocks << 1)));
      _mm512_mask_compressstoreu_epi64(pTweak, maskTweakToReturn /* the first unused tweak */, tweakBlk0);
   }

}

IPP_OWN_DEFN (void, cpAESDecryptXTS_VAES, (Ipp8u* outBlk, const Ipp8u* inpBlk, int nBlks, const Ipp8u* pRKey, int nr, Ipp8u* pTweak))
{
   if (0 == nBlks) {
      return; // do not modify tweak value
   }

   int cipherRounds = nr - 1;

   __m128i* pRkey = (__m128i*)pRKey + cipherRounds + 1;
   __m512i* pInp512 = (__m512i*)inpBlk;
   __m512i* pOut512 = (__m512i*)outBlk;

   /* Produce initial 32 tweaks */
   __ALIGN64 Ipp8u tempTweakBuffer[AES_BLK_SIZE * 4 * 8]; // 32 tweaks
   cpXTSwhitening(tempTweakBuffer, 8, pTweak); // generate 8 tweaks

   const __m512i* pInitialTweaks = (const __m512i*)tempTweakBuffer;

   int tailTweaksConsumedCount = 0;

   __m512i tweakBlk0 = M512(pInitialTweaks);
   __m512i tweakBlk1 = M512(pInitialTweaks + 1);
   __m512i tweakBlk2 = M512(pInitialTweaks + 2);
   __m512i tweakBlk3 = M512(pInitialTweaks + 3);
   __m512i tweakBlk4 = M512(pInitialTweaks + 4);
   __m512i tweakBlk5 = M512(pInitialTweaks + 5);
   __m512i tweakBlk6 = M512(pInitialTweaks + 6);
   __m512i tweakBlk7 = M512(pInitialTweaks + 7);

   // generate other 24 tweaks
   tweakBlk2 = nextTweaks_x8(tweakBlk0);
   tweakBlk3 = nextTweaks_x8(tweakBlk1);
   tweakBlk4 = nextTweaks_x8(tweakBlk2);
   tweakBlk5 = nextTweaks_x8(tweakBlk3);
   tweakBlk6 = nextTweaks_x8(tweakBlk4);
   tweakBlk7 = nextTweaks_x8(tweakBlk5);

   int blocks;
   for (blocks = nBlks; blocks >= (4 * 8); blocks -= (4 * 8)) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pInp512 + 3);
      __m512i blk4 = _mm512_loadu_si512(pInp512 + 4);
      __m512i blk5 = _mm512_loadu_si512(pInp512 + 5);
      __m512i blk6 = _mm512_loadu_si512(pInp512 + 6);
      __m512i blk7 = _mm512_loadu_si512(pInp512 + 7);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);
      blk4 = _mm512_xor_epi64(tweakBlk4, blk4);
      blk5 = _mm512_xor_epi64(tweakBlk5, blk5);
      blk6 = _mm512_xor_epi64(tweakBlk6, blk6);
      blk7 = _mm512_xor_epi64(tweakBlk7, blk7);

      cpAESDecrypt4_VAES_NI(&blk0, &blk1, &blk2, &blk3, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);

      tweakBlk0 = nextTweaks_x32(tweakBlk0);
      tweakBlk1 = nextTweaks_x32(tweakBlk1);
      tweakBlk2 = nextTweaks_x32(tweakBlk2);
      tweakBlk3 = nextTweaks_x32(tweakBlk3);

      cpAESDecrypt4_VAES_NI(&blk4, &blk5, &blk6, &blk7, pRkey, cipherRounds);

      blk4 = _mm512_xor_epi64(tweakBlk4, blk4);
      blk5 = _mm512_xor_epi64(tweakBlk5, blk5);
      blk6 = _mm512_xor_epi64(tweakBlk6, blk6);
      blk7 = _mm512_xor_epi64(tweakBlk7, blk7);

      tweakBlk4 = nextTweaks_x32(tweakBlk4);
      tweakBlk5 = nextTweaks_x32(tweakBlk5);
      tweakBlk6 = nextTweaks_x32(tweakBlk6);
      tweakBlk7 = nextTweaks_x32(tweakBlk7);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);
      _mm512_storeu_si512(pOut512 + 3, blk3);
      _mm512_storeu_si512(pOut512 + 4, blk4);
      _mm512_storeu_si512(pOut512 + 5, blk5);
      _mm512_storeu_si512(pOut512 + 6, blk6);
      _mm512_storeu_si512(pOut512 + 7, blk7);

      pInp512 += 8;
      pOut512 += 8;
   }

   if ((4 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);
      __m512i blk3 = _mm512_loadu_si512(pInp512 + 3);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);

      cpAESDecrypt4_VAES_NI(&blk0, &blk1, &blk2, &blk3, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);
      blk3 = _mm512_xor_epi64(tweakBlk3, blk3);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);
      _mm512_storeu_si512(pOut512 + 3, blk3);

      tailTweaksConsumedCount += 4;
      pInp512 += 4;
      pOut512 += 4;
      blocks -= (4 * 4);
   }

   if ((3 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);
      __m512i blk2 = _mm512_loadu_si512(pInp512 + 2);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);
      tweakBlk1 = M512(pInitialTweaks + tailTweaksConsumedCount + 1);
      tweakBlk2 = M512(pInitialTweaks + tailTweaksConsumedCount + 2);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);

      cpAESDecrypt3_VAES_NI(&blk0, &blk1, &blk2, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);
      blk2 = _mm512_xor_epi64(tweakBlk2, blk2);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);
      _mm512_storeu_si512(pOut512 + 2, blk2);

      tailTweaksConsumedCount += 3;
      pInp512 += 3;
      pOut512 += 3;
      blocks -= (3 * 4);
   }
   else if ((2 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);
      __m512i blk1 = _mm512_loadu_si512(pInp512 + 1);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);
      tweakBlk1 = M512(pInitialTweaks + tailTweaksConsumedCount + 1);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);

      cpAESDecrypt2_VAES_NI(&blk0, &blk1, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);
      blk1 = _mm512_xor_epi64(tweakBlk1, blk1);

      _mm512_storeu_si512(pOut512, blk0);
      _mm512_storeu_si512(pOut512 + 1, blk1);

      tailTweaksConsumedCount += 2;
      pInp512 += 2;
      pOut512 += 2;
      blocks -= (2 * 4);
   }
   else if ((1 * 4) <= blocks) {
      __m512i blk0 = _mm512_loadu_si512(pInp512);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      cpAESDecrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      _mm512_storeu_si512(pOut512, blk0);

      tailTweaksConsumedCount += 1;
      pInp512 += 1;
      pOut512 += 1;
      blocks -= (1 * 4);
   }

   if (blocks) {
      __mmask8 k = (__mmask8)((1 << (blocks + blocks)) - 1);
      __m512i blk0 = _mm512_maskz_loadu_epi64(k, pInp512);

      tweakBlk0 = M512(pInitialTweaks + tailTweaksConsumedCount);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      cpAESDecrypt1_VAES_NI(&blk0, pRkey, cipherRounds);

      blk0 = _mm512_xor_epi64(tweakBlk0, blk0);

      _mm512_mask_storeu_epi64(pOut512, k, blk0);
   }

   {
      __mmask8 maskTweakToReturn = (__mmask8)(((Ipp8u)0x03u << (blocks << 1)));
      _mm512_mask_compressstoreu_epi64(pTweak, maskTweakToReturn /* the first unused tweak */, tweakBlk0);
   }
}

#endif /* (_IPP32E>=_IPP32E_K1) */
