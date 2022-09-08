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
//        ippsPrimeSet_BN()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"

/*F*
// Name: ippsPrimeSet_BN
//
// Purpose: Sets a trial BN for further testing
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pPrime
//    ippStsContextMatchErr      illegal pCtx->idCtx
//                               illegal pPrime->idCtx
//    ippStsOutOfRangeErr        BITSIZE_BNU(BN_NUMBER(pPrime), BN_SIZE(pPrime)) 
//                                                  > PRIME_MAXBITSIZE(pCtx)
//    ippStsNoErr                no error
//
// Parameters:
//    pPrime      pointer to the BN to be set
//    pCtx     pointer to the context
//
*F*/
IPPFUN(IppStatus, ippsPrimeSet_BN, (const IppsBigNumState* pPrime, IppsPrimeState* pCtx))
{
   IPP_BAD_PTR2_RET(pCtx, pPrime);

   IPP_BADARG_RET(!BN_VALID_ID(pPrime), ippStsContextMatchErr);
   IPP_BADARG_RET(!PRIME_VALID_ID(pCtx), ippStsContextMatchErr);

   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pPrime), BN_SIZE(pPrime)) > PRIME_MAXBITSIZE(pCtx), ippStsOutOfRangeErr);

   {
      BNU_CHUNK_T* pPrimeU = BN_NUMBER(pPrime);
      cpSize ns = BN_SIZE(pPrime);
      cpSize nBits = BITSIZE_BNU(pPrimeU, ns);

      BNU_CHUNK_T* pPrimeCtx = PRIME_NUMBER(pCtx);
      BNU_CHUNK_T topMask = MASK_BNU_CHUNK(nBits);

      ZEXPAND_COPY_BNU(pPrimeCtx, BITS_BNU_CHUNK(PRIME_MAXBITSIZE(pCtx)), pPrimeU, ns);
      pPrimeCtx[ns-1] &= topMask;

      return ippStsNoErr;
   }
}
