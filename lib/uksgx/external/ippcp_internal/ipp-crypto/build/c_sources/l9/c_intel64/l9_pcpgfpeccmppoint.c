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
//        ippsGFpECCmpPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECCmpPoint
//
// Purpose: Compares two points
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pP == NULL
//                                   pQ == NULL
//                                   pEC == NULL
//                                   pResult == NULL
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   invalid pP->idCtx
//                                   invalid pQ->idCtx
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pP)!=GFP_FELEN()
//                                   ECP_POINT_FELEN(pQ)!=GFP_FELEN()
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pP              Pointer to the context of the first elliptic curve point
//    pQ              Pointer to the context of the second elliptic curve point
//    pEC             Pointer to the context of the elliptic curve
//    pResult         Pointer to the result of the comparison
//
*F*/

IPPFUN(IppStatus, ippsGFpECCmpPoint,(const IppsGFpECPoint* pP, const IppsGFpECPoint* pQ,
                                           IppECResult* pResult,
                                           IppsGFpECState* pEC))
{
   IPP_BAD_PTR4_RET(pP, pQ, pResult, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pP), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pQ), ippStsContextMatchErr );

   IPP_BADARG_RET( ECP_POINT_FELEN(pP)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( ECP_POINT_FELEN(pQ)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   *pResult = gfec_ComparePoint(pP, pQ, pEC)? ippECPointIsEqual : ippECPointIsNotEqual;
   return ippStsNoErr;
}
