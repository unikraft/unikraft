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
//        ippsPrimeTest()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"
#include "pcpprng.h"
#include "pcptool.h"

/*F*
// Name: ippsPrimeTest
//
// Purpose: Test a number for being a probable prime.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == rndFunc
//                               NULL == pResult
//    ippStsContextMatchErr      !PRIME_VALID_ID()
//    ippStsBadArgErr            1 > nTrials
//    ippStsNoErr                no error
//
// Parameters:
//    pResult     result of test
//    nTrials     parameter for the Miller-Rabin probable primality test
//    pCtx        pointer to the context
//    rndFunc     external PRNG
//    pRndParam   pointer to the external PRNG parameters
*F*/

IPPFUN(IppStatus, ippsPrimeTest, (int nTrials,
                                  Ipp32u* pResult, IppsPrimeState* pCtx,
                                  IppBitSupplier rndFunc, void* pRndParam))
{
   IPP_BAD_PTR3_RET(pResult, pCtx, rndFunc);
   IPP_BADARG_RET(nTrials<1, ippStsBadArgErr);

   IPP_BADARG_RET(!PRIME_VALID_ID(pCtx), ippStsContextMatchErr);

   {
      BNU_CHUNK_T* pPrime = PRIME_NUMBER(pCtx);
      cpSize ns = BITS_BNU_CHUNK(PRIME_MAXBITSIZE(pCtx));
      FIX_BNU(pPrime, ns);

      {
         int ret = cpPrimeTest(pPrime, ns, nTrials, pCtx, rndFunc, pRndParam);
         if(-1 == ret)
            return ippStsErr;
         else {
            *pResult = ret? IPP_IS_PRIME : IPP_IS_COMPOSITE;
            return ippStsNoErr;
         }
      }
   }
}
