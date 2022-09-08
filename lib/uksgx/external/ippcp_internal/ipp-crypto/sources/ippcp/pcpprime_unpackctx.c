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
//        cpUnpackPrimeCtx()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"
#include "pcptool.h"

/*F*
//    Name: cpUnpackPrimeCtx
//
// Purpose: Deserialize prime context
//
// Parameters:
//    pCtx    context
//    pBuffer buffer
*F*/

IPP_OWN_DEFN (void, cpUnpackPrimeCtx, (const Ipp8u* pBuffer, IppsPrimeState* pCtx))
{
   IppsPrimeState* pB = (IppsPrimeState*)(pBuffer);

   /* max length of prime */
   cpSize nsPrime = BITS_BNU_CHUNK(PRIME_MAXBITSIZE(pB));

   CopyBlock(pB, pCtx, sizeof(IppsPrimeState));

   Ipp8u* ptr = (Ipp8u*)pCtx;
   ptr += sizeof(IppsPrimeState);
   ptr = IPP_ALIGNED_PTR(ptr, PRIME_ALIGNMENT);
   PRIME_NUMBER(pCtx)=   (BNU_CHUNK_T*)(ptr);
   ptr += nsPrime*(Ipp32s)sizeof(BNU_CHUNK_T);
   PRIME_TEMP1(pCtx) =   (BNU_CHUNK_T*)(ptr);
   ptr += nsPrime*(Ipp32s)sizeof(BNU_CHUNK_T);
   PRIME_TEMP2(pCtx) =   (BNU_CHUNK_T*)(ptr);
   ptr += nsPrime*(Ipp32s)sizeof(BNU_CHUNK_T);
   PRIME_TEMP3(pCtx) =   (BNU_CHUNK_T*)(ptr);
   ptr += nsPrime*(Ipp32s)sizeof(BNU_CHUNK_T);
   PRIME_MONT(pCtx)  =   (gsModEngine*)(ptr);

   cpSize gsMontOffset = (cpSize)(IPP_INT_PTR(PRIME_MONT(pCtx)) - IPP_INT_PTR(pCtx));

   CopyBlock((Ipp8u*)pB + sizeof(IppsPrimeState), PRIME_NUMBER(pCtx), nsPrime*(Ipp32s)sizeof(BNU_CHUNK_T));
   gsUnpackModEngineCtx((Ipp8u*)pB + gsMontOffset, PRIME_MONT(pCtx));
}
