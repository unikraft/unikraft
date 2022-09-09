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
//        ippsGFpSetElementRandom()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpSetElementRandom
//
// Purpose: Set GF Element Random
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pR
//                               NULL == rndFunc
//
//    ippStsContextMatchErr      invalid pGFp->idCtx
//                               invalid pR->idCtx
//
//    ippStsOutOfRangeErr        GFPE_ROOM() != GFP_FELEN()
//
//    ippStsErr                  internal error caused by call of rndFunc()
//
//    ippStsNoErr                no error
//
// Parameters:
//    pR           Pointer to the context of the finite field element.
//    pGFp         Pointer to the context of the finite field.
//    rndFunc      Pseudorandom number generator.
//    pRndParam    Pointer to the context of the pseudorandom number generator.
*F*/
IPPFUN(IppStatus, ippsGFpSetElementRandom,(IppsGFpElement* pR, IppsGFpState* pGFp,
                                           IppBitSupplier rndFunc, void* pRndParam))
{
   IPP_BAD_PTR3_RET(pR, pGFp, rndFunc);
   IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pR), ippStsContextMatchErr );

   {
      gsModEngine* pGFE = GFP_PMA(pGFp);
      IPP_BADARG_RET( GFPE_ROOM(pR)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
      return cpGFpxRand(GFPE_DATA(pR), pGFE, rndFunc, pRndParam)? ippStsNoErr : ippStsErr;
   }
}
