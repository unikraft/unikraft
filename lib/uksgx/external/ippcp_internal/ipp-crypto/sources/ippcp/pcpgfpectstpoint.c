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
//
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     EC over GF(p) Operations
//
//     Context:
//        ippsGFpECTstPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECTstPoint
//
// Purpose: Checks if a point belongs to an elliptic curve
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pP == NULL
//                                   pEC == NULL
//                                   pResult == NULL
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   invalid pP->idCtx
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pP)!=GFP_FELEN()
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pP          Pointer to the IppsGFpECPoint context
//    pEC         Pointer to the context of the elliptic curve
//    pResult     Pointer to the result of the check
//
// Note:
//    Even if test passed is not a fact that the point belongs to BP-related subgroup BP
//
*F*/

IPPFUN(IppStatus, ippsGFpECTstPoint,(const IppsGFpECPoint* pP,
                                     IppECResult* pResult,
                                     IppsGFpECState* pEC))
{
   IPP_BAD_PTR3_RET(pP, pResult, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pP), ippStsContextMatchErr );

   IPP_BADARG_RET( ECP_POINT_FELEN(pP)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   if( gfec_IsPointAtInfinity(pP) )
      *pResult = ippECPointIsAtInfinite;
   else if( !gfec_IsPointOnCurve(pP, pEC) )
      *pResult = ippECPointIsNotValid;
   else
      *pResult = ippECValid;

   return ippStsNoErr;
}
