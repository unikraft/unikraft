/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
// Montgomery engine preparation (GetSize/init/Set)
*/

/*
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
// 
//     Context:
//        gsMethod_RSA_gpr_private()
//
*/

#include "owncp.h"
#include "pcpngmontexpstuff.h"
#include "gsscramble.h"
#include "pcpngrsamethod.h"
#include "pcpngrsa.h"

#if (_IPP32E>=_IPP32E_K1)
#include "pcpngmontexpstuff_avx512.h"

IPP_OWN_DEFN (gsMethod_RSA*, gsMethod_RSA_avx512_private, (void))
{
   static gsMethod_RSA m = {
      RSA_AVX512_MIN_BITSIZE, RSA_AVX512_MAX_BITSIZE, /* RSA range */

      /* private key exponentiation: private, window, avx512 */
      #if !defined(_USE_WINDOW_EXP_)
      gsMontExpBinBuffer_avx512,
      gsMontExpBin_BNU_sscm_avx512
      #else
      gsMontExpWinBuffer_avx512,
      gsMontExpWin_BNU_sscm_avx512
      #endif
      , NULL
   };
   return &m;
}

#define RSA_DUAL_EXP_AVX512_MIN_BITSIZE 2048
#define RSA_DUAL_EXP_AVX512_MAX_BITSIZE 4096

IPP_OWN_DEFN (gsMethod_RSA*, gsMethod_RSA_avx512_crt_private, (int privExpBitSize)) {
   static gsMethod_RSA m = {
      RSA_DUAL_EXP_AVX512_MIN_BITSIZE, RSA_DUAL_EXP_AVX512_MAX_BITSIZE, /* RSA range */
      gsMontDualExpWinBuffer_avx512,
      NULL,
      NULL
   };

   if (IsFeatureEnabled(ippCPUID_AVX512IFMA)) {
      ngMontDualExp dexpFunc = NULL;
      switch (privExpBitSize) {
         /* RSA 2k,3k,4k only supported */
         case 1024:
         case 1536:
         case 2048:
            dexpFunc = gsMontDualExpWin_BNU_sscm_avx512;
            break;
         default:
            dexpFunc = NULL;
      }
      m.dualExpFun = dexpFunc;
   }

   return &m;
}
#endif /* _IPP32E_K1 */
