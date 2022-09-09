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
//        ippsPrimeGetSize()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"
#include "pcptool.h"

/*F*
// Name: ippsPrimeGetSize
//
// Purpose: Returns size of Prime Number Generator context (bytes).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSize
//    ippStsLengthErr            1 > nMaxBits
//    ippStsNoErr                no error
//
// Parameters:
//    nMaxBits max length of a prime number
//    pSize    pointer to the size of internal context
*F*/
IPPFUN(IppStatus, ippsPrimeGetSize, (int nMaxBits, int* pSize))
{
   IPP_BAD_PTR1_RET(pSize);
   IPP_BADARG_RET(nMaxBits<1, ippStsLengthErr);

   {
      cpSize len = BITS_BNU_CHUNK(nMaxBits);
      cpSize montSize;

      gsModEngineGetSize(nMaxBits, MONT_DEFAULT_POOL_LENGTH, &montSize);

      *pSize = (Ipp32s)sizeof(IppsPrimeState)
              +len*(Ipp32s)sizeof(BNU_CHUNK_T)
              +len*(Ipp32s)sizeof(BNU_CHUNK_T)
              +len*(Ipp32s)sizeof(BNU_CHUNK_T)
              +len*(Ipp32s)sizeof(BNU_CHUNK_T)
              +montSize
              +PRIME_ALIGNMENT-1;

      return ippStsNoErr;
   }
}
