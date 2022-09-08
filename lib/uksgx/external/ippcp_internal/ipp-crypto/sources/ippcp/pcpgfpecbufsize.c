/*******************************************************************************
* Copyright 2010-2021 Intel Corporation
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
//     EC over GF(p^m) definitinons
//
//     Context:
//        ippsGFpECScratchBufferSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"

/*F*
// Name: ippsGFpScratchBufferSize
//
// Purpose: Gets the size of the scratch buffer.
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pEC == NULL
//                              pBufferSize == NULL
//    ippStsContextMatchErr     invalid pEC->idCtx
//    ippStsBadArgErr           0>=nScalars
//                              nScalars>6
//    ippStsNoErr               no error
//
// Parameters:
//    nScalars        Number of scalar values.
//    pEC             Pointer to the context of the elliptic curve
//    pBufferSize     Pointer to the calculated buffer size in bytes.
//
*F*/

IPPFUN(IppStatus, ippsGFpECScratchBufferSize,(int nScalars, const IppsGFpECState* pEC, int* pBufferSize))
{
   IPP_BAD_PTR2_RET(pEC, pBufferSize);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );

   IPP_BADARG_RET( (0>=nScalars)||(nScalars>IPP_MAX_EXPONENT_NUM), ippStsBadArgErr);

   {
      /* select constant size of window */
      const int w = 5;
      /* number of table entries */
      const int nPrecomputed = 1<<(w-1);  /* because of signed digit representation of scalar is uses */

      int pointDataSize = ECP_POINTLEN(pEC)*(Ipp32s)sizeof(BNU_CHUNK_T);

      *pBufferSize = nScalars * pointDataSize*nPrecomputed + CACHE_LINE_SIZE;

      return ippStsNoErr;
   }
}
