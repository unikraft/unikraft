/*******************************************************************************
* Copyright 2004-2021 Intel Corporation
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
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptographic Primitives (ippcp)
//     Prime Number Primitives.
// 
//  Contents:
//        cpPackPrimeCtx()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"
#include "pcptool.h"

/*F*
//    Name: cpPackPrimeCtx
//
// Purpose: Serialize prime context
//
// Parameters:
//    pCtx    context
//    pBuffer buffer
*F*/

IPP_OWN_DEFN (void, cpPackPrimeCtx, (const IppsPrimeState* pCtx, Ipp8u* pBuffer))
{
   IppsPrimeState* pB = (IppsPrimeState*)(pBuffer);

   /* max length of prime */
   cpSize nsPrime = BITS_BNU_CHUNK(PRIME_MAXBITSIZE(pCtx));

   CopyBlock(pCtx, pB, sizeof(IppsPrimeState));

   cpSize dataAlignment = (cpSize)(IPP_INT_PTR(PRIME_NUMBER(pCtx)) - IPP_INT_PTR(pCtx) - (IPP_INT64)sizeof(IppsPrimeState));
   cpSize gsMontOffset = (cpSize)(IPP_INT_PTR(PRIME_MONT(pCtx)) - IPP_INT_PTR(pCtx) - dataAlignment);

   CopyBlock(PRIME_NUMBER(pCtx), (Ipp8u*)pB + sizeof(IppsPrimeState), nsPrime*(Ipp32s)sizeof(BNU_CHUNK_T));
   gsPackModEngineCtx(PRIME_MONT(pCtx), (Ipp8u*)pB + gsMontOffset);
}
