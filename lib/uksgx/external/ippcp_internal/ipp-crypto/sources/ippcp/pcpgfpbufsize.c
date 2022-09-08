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
//        ippsGFpScratchBufferSize
//
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpScratchBufferSize
//
// Purpose: Gets the size of the scratch buffer.
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pGFp == NULL
//                              pBufferSize == NULL
//    ippStsContextMatchErr     incorrect pGFp's context id
//    ippStsBadArgErr           0>=nExponents
//                              nExponents>6
//    ippStsNoErr               no error
//
// Parameters:
//    nExponents      Number of exponents.
//    ExpBitSize      Maximum bit size of the exponents.
//    pGFp            Pointer to the context of the finite field.
//    pBufferSize     Pointer to the calculated buffer size in bytes.
//
*F*/

IPPFUN(IppStatus, ippsGFpScratchBufferSize,(int nExponents, int ExpBitSize, const IppsGFpState* pGFp, int* pBufferSize))
{
   IPP_BAD_PTR2_RET(pGFp, pBufferSize);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );

   IPP_BADARG_RET( 0>=nExponents ||nExponents>IPP_MAX_EXPONENT_NUM, ippStsBadArgErr);
   IPP_BADARG_RET( 0>=ExpBitSize, ippStsBadArgErr);

   /* gres 06/10/2019: ExpBirSize=BNU_CHUNK_BITS*n -- meet CTE implementation */
   ExpBitSize = ((ExpBitSize + BNU_CHUNK_BITS-1)/BNU_CHUNK_BITS) * BNU_CHUNK_BITS;
   {
      int elmDataSize = GFP_FELEN(GFP_PMA(pGFp))*(Ipp32s)sizeof(BNU_CHUNK_T);

      /* get window_size */
      int w = (nExponents==1)? cpGFpGetOptimalWinSize(ExpBitSize) : /* use optimal window size, if single-scalar operation */
                               nExponents;                          /* or pseudo-oprimal if multi-scalar operation */

      /* number of table entries */
      int nPrecomputed = 1<<w;

      *pBufferSize = elmDataSize*nPrecomputed + (CACHE_LINE_SIZE-1);

      return ippStsNoErr;
   }
}
