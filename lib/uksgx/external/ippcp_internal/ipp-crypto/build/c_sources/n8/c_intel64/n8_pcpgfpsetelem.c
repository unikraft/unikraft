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
//        ippsGFpSetElement()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"

//tbcd: temporary excluded: #include <assert.h>

/*F*
// Name: ippsGFpSetElement
//
// Purpose: Set GF Element
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pR
//                               NULL == pA && lenA>0
//
//    ippStsContextMatchErr      invalid pGFp->idCtx
//                               invalid pR->idCtx
//
//    ippStsSizeErr              pA && !(0<=lenA && lenA<GFP_FELEN32())
//
//    ippStsOutOfRangeErr        GFPE_ROOM() != GFP_FELEN()
//                               BNU representation of pA[i]..pA[i+GFP_FELEN32()-1] >= modulus
//
//    ippStsNoErr                no error
//
// Parameters:
//    pA          pointer to the data representation Finite Field element
//    lenA        length of Finite Field data representation array
//    pR          pointer to Finite Field Element context
//    pGFp        pointer to Finite Field context
*F*/
IPPFUN(IppStatus, ippsGFpSetElement,(const Ipp32u* pA, int lenA, IppsGFpElement* pR, IppsGFpState* pGFp))
{
   IPP_BAD_PTR2_RET(pR, pGFp);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pR), ippStsContextMatchErr );

   IPP_BADARG_RET( !pA && (0<lenA), ippStsNullPtrErr);
   IPP_BADARG_RET( pA && !(0<=lenA && lenA<=GFP_FELEN32(GFP_PMA(pGFp))), ippStsSizeErr );
   IPP_BADARG_RET( GFPE_ROOM(pR)!=GFP_FELEN(GFP_PMA(pGFp)), ippStsOutOfRangeErr );

   {
      IppStatus sts = ippStsNoErr;

      gsModEngine* pGFE = GFP_PMA(pGFp);
      int elemLen = GFP_FELEN(pGFE);
      BNU_CHUNK_T* pTmp = cpGFpGetPool(1, pGFE);
      //tbcd: temporary excluded: assert(NULL!=pTmp);

      ZEXPAND_BNU(pTmp, 0, elemLen);
      if(pA && lenA)
         cpGFpxCopyToChunk(pTmp, pA, lenA, pGFE);

      if(!cpGFpxSet(GFPE_DATA(pR), pTmp, elemLen, pGFE))
         sts = ippStsOutOfRangeErr;

      cpGFpReleasePool(1, pGFE);
      return sts;
   }
}
