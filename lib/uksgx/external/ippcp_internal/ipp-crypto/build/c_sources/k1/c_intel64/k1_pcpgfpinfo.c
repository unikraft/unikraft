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
//     Operations over GF(p) ectension.
// 
//     Context:
//        ippsGFpInfo()
//
*/

#include "owncp.h"
#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"


/*F*
// Name: ippsGFpInit
//
// Purpose: finite field info
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pInfo
//
//    ippStsContextMatchErr      invalid pGFp->idCtx
//
//    ippStsNoErr                no error
//
// Parameters:
//    pInfo    pointer to finite field infon
//    pGFp     Pointer to the context of the finite field.
*F*/
IPPFUN(IppStatus, ippsGFpGetInfo,(IppsGFpInfo* pInfo, const IppsGFpState* pGFp))
{
   IPP_BAD_PTR2_RET(pGFp, pInfo);

   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );

   {
      gsModEngine* pGFpx = GFP_PMA(pGFp);     /* current */
      gsModEngine* pGFpBasic = cpGFpBasic(pGFpx); /* basic */
      pInfo->parentGFdegree = MOD_EXTDEG(pGFpx);               /* parent extension */
      pInfo->basicGFdegree = cpGFpBasicDegreeExtension(pGFpx); /* total basic extention */
      pInfo->basicElmBitSize = GFP_FEBITLEN(pGFpBasic);             /* basic bitsise */

      return ippStsNoErr;
   }
}
