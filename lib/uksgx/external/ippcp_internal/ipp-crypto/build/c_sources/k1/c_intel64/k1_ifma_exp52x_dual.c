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

#include "owncp.h"

#if(_IPP32E>=_IPP32E_K1)

#include "pcptool.h"

#include "pcpngmontexpstuff_avx512.h"
#include "ifma_rsa_arith.h"

IPP_OWN_DEFN (cpSize, gsMontDualExpWinBuffer_avx512, (int modulusBits))
{
   cpSize redNum = numofVariable_avx512(modulusBits);
   /* Reg (ymm) capacity = 4 qwords */
   cpSize redBufferNum = numofVariableBuff_avx512(redNum, 4);
   return redBufferNum * 2 * 8;
}

IPP_OWN_FUNPTR (void, AMM52, (Ipp64u *out, const Ipp64u *a, const Ipp64u *b, const Ipp64u *m, Ipp64u k0))
IPP_OWN_FUNPTR (void, DEXP52, (Ipp64u* out, const Ipp64u* base, const Ipp64u* exp[2], const Ipp64u* modulus, const Ipp64u* pMont, const Ipp64u k0[2]))

IPP_OWN_DEFN (cpSize, gsMontDualExpWin_BNU_sscm_avx512, (BNU_CHUNK_T* dataY[2],
                                                   const BNU_CHUNK_T* dataX[2],
                                                         cpSize nsX[2],
                                                   const BNU_CHUNK_T* dataE[2],
                                                         gsModEngine* pMont[2],
                                                         BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* modulus[2] = {0};
   const BNU_CHUNK_T* rr[2] = {0};
   cpSize modulusSize[2] = {0};
   BNU_CHUNK_T k0[2] = {0};

   modulus[0] = MOD_MODULUS(pMont[0]);
   modulus[1] = MOD_MODULUS(pMont[1]);
   modulusSize[0] = MOD_LEN(pMont[0]);
   modulusSize[1] = MOD_LEN(pMont[1]);
   rr[0] = MOD_MNT_R2(pMont[0]);
   rr[1] = MOD_MNT_R2(pMont[1]);
   k0[0] = MOD_MNT_FACTOR(pMont[0]);
   k0[1] = MOD_MNT_FACTOR(pMont[1]);

   /*
    * Dual expo implemenentation assumes that bit sizes of P and Q are
    * the same, so query size from the first.
    */
   int modulusBitSize = BITSIZE_BNU(modulus[0], modulusSize[0]);
   int redLen = NUMBER_OF_DIGITS(modulusBitSize + 2, EXP_DIGIT_SIZE_AVX512);
   /* For ymm-based implementation reg capacity = 4 qwords */
   int redBufferLen = numofVariableBuff_avx512(redLen, 4);

   /* Allocate buffers */
   BNU_CHUNK_T* redX      = pBuffer;
   BNU_CHUNK_T* redM      = redX     + 2*redBufferLen;
   BNU_CHUNK_T* redRR     = redM     + 2*redBufferLen;
   BNU_CHUNK_T* redCoeff  = redRR    + 2*redBufferLen;
   BNU_CHUNK_T* redBuffer = redCoeff +   redBufferLen;

   AMM52 ammFunc = NULL;
   DEXP52 dexpFunc = NULL;
   switch (modulusBitSize) {
      case 1024:
         ammFunc = ifma256_amm52x20;
         dexpFunc = (DEXP52)ifma256_exp52x20_dual;
         break;
      case 1536:
         ammFunc = ifma256_amm52x30;
         dexpFunc = (DEXP52)ifma256_exp52x30_dual;
         break;
      case 2048:
         ammFunc = ifma256_amm52x40;
         dexpFunc = (DEXP52)ifma256_exp52x40_dual;
         break;
      default:
         /* Other modulus sizes not supported. This function shall not be called for them. */
         return 0;
   }

   ZEXPAND_BNU(redCoeff, 0, redBufferLen);
   int conv_coeff = 4 * (EXP_DIGIT_SIZE_AVX512 * redLen - modulusBitSize);
   /* Set corresponding bit in reduced domain */
   SET_BIT(redCoeff, 64 * (int)(conv_coeff / 52) + conv_coeff % 52);

   for (int i = 0; i < 2; i++) {
      /* Convert base into redundant domain */
      ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataX[i], nsX[i]);
      regular_dig52(redX + i*redBufferLen, redBufferLen, redBuffer, modulusBitSize);

      /* Convert modulus into redundant domain */
      ZEXPAND_COPY_BNU(redBuffer, redBufferLen, modulus[i], modulusSize[i]);
      regular_dig52(redM + i*redBufferLen, redBufferLen, redBuffer, modulusBitSize);

      /*
       * Compute target domain Montgomery converter RR' based on original domain RR.
       *
       * Example: modlen = 1024: RR = 2^2048 mod m, RR' = 2^2080 mod m
       *     conv_coeff = 2^64
       *     (1st amm): 2^2048*2^2048/2^1040= 2^3056 mod m
       *     (2nd amm): 2^3056*2^64/2^1040 = 2^2080 mod m
       *
       */
      ZEXPAND_COPY_BNU(redBuffer, redBufferLen, rr[i], modulusSize[i]);
      regular_dig52(redRR + i*redBufferLen, redBufferLen, redBuffer, modulusBitSize);
      ammFunc(redRR + i*redBufferLen, redRR + i*redBufferLen, redRR + i*redBufferLen, redM + i*redBufferLen, k0[i]);
      ammFunc(redRR + i*redBufferLen, redRR + i*redBufferLen, redCoeff, redM + i*redBufferLen, k0[i]);
   }

   dexpFunc(redRR, redX, dataE, redM, redRR, k0);

   /* Convert result back to regular domain */
   for (int i = 0; i < 2; i++)
      dig52_regular(dataY[i], redRR + i*redBufferLen, modulusBitSize);

   /* Clear redundant exponents buffer */
   PurgeBlock(redRR, 2*redBufferLen*(int)sizeof(BNU_CHUNK_T));

   return (modulusSize[0] + modulusSize[1]);
}

#endif
