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
//        ippsPrimeGet()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"

/*F*
// Name: ippsPrimeGet
//
// Purpose: Extracts the bitlength and the probable prime BNU.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pPrime
//                               NULL == pBits
//    ippStsContextMatchErr      illegal pCtx->idCtx
//    ippStsNoErr                no error
//
// Parameters:
//    pPrime   pointer to the BNU value
//    pSize    pointer to the BNU wordsize
//    pCtx     pointer to the context
//
*F*/
IPPFUN(IppStatus, ippsPrimeGet, (Ipp32u* pPrime, int* pSize, const IppsPrimeState* pCtx))
{
   IPP_BAD_PTR3_RET(pCtx, pPrime, pSize);

   IPP_BADARG_RET(!PRIME_VALID_ID(pCtx), ippStsContextMatchErr);

   {
      Ipp32u* pValue = (Ipp32u*)PRIME_NUMBER(pCtx);
      cpSize len32 = BITS2WORD32_SIZE(PRIME_MAXBITSIZE(pCtx));
      FIX_BNU32(pValue, len32);

      COPY_BNU(pPrime, pValue, len32);
      *pSize = len32;

      return ippStsNoErr;
   }
}
