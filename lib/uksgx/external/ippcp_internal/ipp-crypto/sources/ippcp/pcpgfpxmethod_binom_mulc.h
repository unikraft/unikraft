/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     GF(p^d) methods, if binomial generator
//
*/
#if !defined(_CP_GFP_METHOD_BINOM_H)
#define _CP_GFP_METHOD_BINOM_H

#include "owncp.h"
#include "pcpgfpxstuff.h"


static BNU_CHUNK_T* cpGFpxMul_G0(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx)
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;

   BNU_CHUNK_T* pGFpolynomial = GFP_MODULUS(pGFEx); /* g(x) = t^d + g0 */

   #if defined GS_DBG
   BNU_CHUNK_T* arg0 = cpGFpGetPool(1, pGroundGFE);
   BNU_CHUNK_T* arg1 = cpGFpGetPool(1, pGroundGFE);
   int groundElemLen = GFP_FELEN(pGroundGFE);
   #endif

   #if defined GS_DBG
   cpGFpxGet(arg0, groundElemLen, pA, pGroundGFE);
   cpGFpxGet(arg1, groundElemLen, pGFpolynomial, pGroundGFE);
   #endif

   mulF(pR, pA, pGFpolynomial, pGroundGFE);

   #if defined GS_DBG
   cpGFpReleasePool(2, pGroundGFE);
   #endif

   return pR;
}

#endif /* _CP_GFP_METHOD_BINOM_H */
