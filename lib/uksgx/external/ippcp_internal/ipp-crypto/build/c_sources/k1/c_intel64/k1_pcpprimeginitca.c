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
//        ippsPrimeInit()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"
#include "pcptool.h"

/*F*
// Name: ippsPrimeInit
//
// Purpose: Initializes Prime Number Generator context
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//    ippStsLengthErr            1 > nMaxBits
//    ippStsNoErr                no error
//
// Parameters:
//    nMaxBits  max length of a prime number
//    pCtx     pointer to the context to be initialized
*F*/
IPPFUN(IppStatus, ippsPrimeInit, (int nMaxBits, IppsPrimeState* pCtx))
{
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(nMaxBits<1, ippStsLengthErr);

   {
      Ipp8u* ptr = (Ipp8u*)pCtx;

      cpSize len = BITS_BNU_CHUNK(nMaxBits);

      PRIME_SET_ID(pCtx);
      PRIME_MAXBITSIZE(pCtx) = nMaxBits;

      ptr += sizeof(IppsPrimeState);
      ptr = (Ipp8u*)(IPP_ALIGNED_PTR(ptr, PRIME_ALIGNMENT));
      PRIME_NUMBER(pCtx) = (BNU_CHUNK_T*)ptr;

      ptr += len*(Ipp32s)sizeof(BNU_CHUNK_T);
      PRIME_TEMP1(pCtx) = (BNU_CHUNK_T*)ptr;

      ptr += len*(Ipp32s)(Ipp32s)sizeof(BNU_CHUNK_T);
      PRIME_TEMP2(pCtx) = (BNU_CHUNK_T*)ptr;

      ptr += len*(Ipp32s)sizeof(BNU_CHUNK_T);
      PRIME_TEMP3(pCtx) = (BNU_CHUNK_T*)ptr;

      ptr += len*(Ipp32s)sizeof(BNU_CHUNK_T);
      PRIME_MONT(pCtx) = (gsModEngine*)(ptr);
      gsModEngineInit(PRIME_MONT(pCtx), NULL, nMaxBits, MONT_DEFAULT_POOL_LENGTH, gsModArithMont());

      return ippStsNoErr;
   }
}
