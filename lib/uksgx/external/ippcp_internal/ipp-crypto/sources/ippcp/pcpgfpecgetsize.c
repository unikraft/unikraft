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
//        ippsGFpECGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"

/*F*
// Name: ippsGFpECGetSize
//
// Purpose: Gets the size of an elliptic curve over the finite field
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pGFp
//                                  NULL == pSize
//
//    ippStsContextMatchErr         invalid pGFp->idCtx
//
//    ippStsNoErr                   no error
//
// Parameters:
//    pGFp      Pointer to the IppsGFpState context of the underlying finite field
//    pSize     Buffer size in bytes needed for the IppsGFpECState context
//
*F*/

IPPFUN(IppStatus, ippsGFpECGetSize,(const IppsGFpState* pGFp, int* pSize))
{
   IPP_BAD_PTR2_RET(pGFp, pSize);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );

   {
      gsModEngine* pGFE = GFP_PMA(pGFp);
      *pSize = cpGFpECGetSize(cpGFpBasicDegreeExtension(pGFE), GFP_FEBITLEN(cpGFpBasic(pGFE)));
      return ippStsNoErr;
   }
}
