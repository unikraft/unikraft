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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Operations over GF(p).
// 
//     Context:
//        cpGFpSetGFp()
// 
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


static void cpGFESet(gsModEngine* pGFE, const BNU_CHUNK_T* pPrime, int primeBitSize, const gsModMethod* method)
{
   int primeLen = BITS_BNU_CHUNK(primeBitSize);

   /* arithmetic methods */
   GFP_METHOD(pGFE) = method;

   /* store modulus */
   COPY_BNU(GFP_MODULUS(pGFE), pPrime, primeLen);

   /* montgomery factor */
   GFP_MNT_FACTOR(pGFE) = gsMontFactor(GFP_MODULUS(pGFE)[0]);

   /* montgomery identity (R) */
   ZEXPAND_BNU(GFP_MNT_R(pGFE), 0, primeLen);
   GFP_MNT_R(pGFE)[primeLen] = 1;
   cpMod_BNU(GFP_MNT_R(pGFE), primeLen+1, GFP_MODULUS(pGFE), primeLen);

   /* montgomery domain converter (RR) */
   ZEXPAND_BNU(GFP_MNT_RR(pGFE), 0, primeLen);
   COPY_BNU(GFP_MNT_RR(pGFE)+primeLen, GFP_MNT_R(pGFE), primeLen);
   cpMod_BNU(GFP_MNT_RR(pGFE), 2*primeLen, GFP_MODULUS(pGFE), primeLen);

   /* half of modulus */
   cpLSR_BNU(GFP_HMODULUS(pGFE), GFP_MODULUS(pGFE), primeLen, 1);

   /* set qnr value */
   cpGFEqnr(pGFE);
}

IPP_OWN_DEFN (IppStatus, cpGFpSetGFp, (const BNU_CHUNK_T* pPrime, int primeBitSize, const IppsGFpMethod* method, IppsGFpState* pGF))
{
   cpGFESet(GFP_PMA(pGF), pPrime, primeBitSize, method->arith);
   return ippStsNoErr;
}
