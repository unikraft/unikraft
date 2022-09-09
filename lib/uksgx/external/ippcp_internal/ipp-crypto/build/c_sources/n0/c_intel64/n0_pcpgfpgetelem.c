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
//        ippsGFpGetElement()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"

//tbcd: temporary excluded: #include <assert.h>

/*F*
// Name: ippsGFpGetElement
//
// Purpose: Get GF Element
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pA
//                               NULL == pDataA
//
//    ippStsContextMatchErr      invalid pGFp->idCtx
//                               invalid pA->idCtx
//
//    ippStsOutOfRangeErr        GFPE_ROOM() != GFP_FELEN()
//
//    ippStsSizeErr              !(0<lenA && lenA>=GFP_FELEN32(pGFE))
//
//    ippStsNoErr                no error
//
// Parameters:
//    pA       Pointer to the context of the finite field element.
//    pDataA   Pointer to the data array to copy the finite field element from.
//    lenA     Length of the data array.
//    pGFp     Pointer to the context of the finite field.
*F*/

IPPFUN(IppStatus, ippsGFpGetElement, (const IppsGFpElement* pA, Ipp32u* pDataA, int lenA, IppsGFpState* pGFp))
{
   IPP_BAD_PTR3_RET(pA, pDataA, pGFp);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pA), ippStsContextMatchErr );
   {
      gsModEngine* pGFE = GFP_PMA(pGFp);
      IPP_BADARG_RET( GFPE_ROOM(pA)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
      IPP_BADARG_RET( !(0<lenA && lenA>=GFP_FELEN32(pGFE)), ippStsSizeErr );

      {
         int elemLen = GFP_FELEN(pGFE);
         BNU_CHUNK_T* pTmp = cpGFpGetPool(1, pGFE);
         //tbcd: temporary excluded: assert(NULL!=pTmp);

         cpGFpxGet(pTmp, elemLen, GFPE_DATA(pA), pGFE);
         cpGFpxCopyFromChunk(pDataA, pTmp, pGFE);

         cpGFpReleasePool(1, pGFE);
         return ippStsNoErr;
      }
   }
}
