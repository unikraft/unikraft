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
//        ippsPrimeSet()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"

/*F*
// Name: ippsPrimeSet
//
// Purpose: Sets a trial BNU for further testing
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pPrime
//    ippStsContextMatchErr      illegal pCtx->idCtx
//    ippStsLengthErr            1 > nBits
//    ippStsOutOfRangeErr        nBits > PRIME_MAXBITSIZE(pCtx)
//    ippStsNoErr                no error
//
// Parameters:
//    pPrime   pointer to the number to be set
//    nBits    bitlength of input number bitlength
//    pCtx     pointer to the context
//
*F*/
IPPFUN(IppStatus, ippsPrimeSet, (const Ipp32u* pPrime, int nBits, IppsPrimeState* pCtx))
{
   IPP_BAD_PTR2_RET(pCtx, pPrime);
   IPP_BADARG_RET(nBits<1, ippStsLengthErr);

   IPP_BADARG_RET(!PRIME_VALID_ID(pCtx), ippStsContextMatchErr);

   IPP_BADARG_RET(nBits > PRIME_MAXBITSIZE(pCtx), ippStsOutOfRangeErr);

   /* clear prime container */
   ZEXPAND_BNU(PRIME_NUMBER(pCtx), 0, BITS_BNU_CHUNK(PRIME_MAXBITSIZE(pCtx)));

   {
      Ipp32u* pValue = (Ipp32u*)PRIME_NUMBER(pCtx);

      cpSize len32 = BITS2WORD32_SIZE(nBits);
      Ipp32u mask = MAKEMASK32(nBits);
      FIX_BNU32(pPrime, len32);

      ZEXPAND_COPY_BNU(pValue, BITS2WORD32_SIZE(PRIME_MAXBITSIZE(pCtx)), pPrime, len32);
      pValue[len32-1] &= mask;

      return ippStsNoErr;
   }
}
