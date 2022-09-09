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
//        ippsGFpCmpElement()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpCmpElement
//
// Purpose: Compares GF Elements
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pA
//                               NULL == pB
//                               NULL == pResult
//
//    ippStsContextMatchErr      invalid pGFp->idCtx
//                               invalid pA->idCtx
//                               invalid pB->idCtx
//
//    ippStsOutOfRangeErr        GFPE_ROOM() != GFP_FELEN()
//
//    ippStsNoErr                no error
//
// Parameters:
//    pA         Pointer to the context of the first finite field element.
//    pB         Pointer to the context of the second finite field element.
//    pResult    Pointer to the result of the comparison.
//    pGFp       Pointer to the context of the finite field.
//
*F*/

IPPFUN(IppStatus, ippsGFpCmpElement,(const IppsGFpElement* pA, const IppsGFpElement* pB,
                                     int* pResult,
                                     const IppsGFpState* pGFp))
{
   IPP_BAD_PTR4_RET(pA, pB, pResult, pGFp);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pA), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pB), ippStsContextMatchErr );
   {
      gsModEngine* pGFE = GFP_PMA(pGFp);
      IPP_BADARG_RET( (GFPE_ROOM(pA)!=GFP_FELEN(pGFE)) || (GFPE_ROOM(pB)!=GFP_FELEN(pGFE)), ippStsOutOfRangeErr);
      {
         int flag = cpGFpElementCmp(GFPE_DATA(pA), GFPE_DATA(pB), GFP_FELEN(pGFE));
         if( GFP_IS_BASIC(pGFE) )
            *pResult = (0==flag)? IPP_IS_EQ : (0<flag)? IPP_IS_GT : IPP_IS_LT;
         else
            *pResult = (0==flag)? IPP_IS_EQ : IPP_IS_NE;
         return ippStsNoErr;
      }
   }
}
