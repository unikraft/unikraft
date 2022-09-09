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
//        ippsGFpIsUnityElement()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpIsUnityElement
//
// Purpose: Compare GF Element with unity element
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pA
//                               NULL == pResult
//
//    ippStsContextMatchErr      invalid pGFp->idCtx
//                               invalid pA->idCtx
//
//    ippStsOutOfRangeErr        GFPE_ROOM() != GFP_FELEN()
//
//    ippStsNoErr                no error
//
// Parameters:
//    pA         Pointer to the context of the finite field element.
//    pResult    Pointer to the result of the comparison
//    pGFp       Pointer to the context of the finite field.
//
*F*/

IPPFUN(IppStatus, ippsGFpIsUnityElement,(const IppsGFpElement* pA,
                                     int* pResult,
                                     const IppsGFpState* pGFp))
{
   IPP_BAD_PTR3_RET(pA, pResult, pGFp);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pA), ippStsContextMatchErr );
   {
      gsModEngine* pGFE = GFP_PMA(pGFp);
      IPP_BADARG_RET( GFPE_ROOM(pA)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
      {
         gsModEngine* pBasicGFE = cpGFpBasic(pGFE);
         int basicElmLen = GFP_FELEN(pBasicGFE);
         BNU_CHUNK_T* pUnity = GFP_MNT_R(pBasicGFE);

         int elmLen = GFP_FELEN(pGFE);
         int flag;

         FIX_BNU(pUnity, basicElmLen);
         FIX_BNU(GFPE_DATA(pA), elmLen);

         flag = (basicElmLen==elmLen) && (0 == cpGFpElementCmp(GFPE_DATA(pA), pUnity, elmLen));
         *pResult = (1==flag)? IPP_IS_EQ : IPP_IS_NE;
         return ippStsNoErr;
      }
   }
}
