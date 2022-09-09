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
//        ippsGFpECNegPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECNegPoint
//
// Purpose: Computes the inverse of a point
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pP == NULL
//                                   pR == NULL
//                                   pEC == NULL
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   invalid pP->idCtx
//                                   invalid pR->idCtx
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pP)!=GFP_FELEN()
//                                   ECP_POINT_FELEN(pR)!=GFP_FELEN()
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pP              Pointer to the context of the first elliptic curve point
//    pR              Pointer to the context of the resulting elliptic curve point
//    pEC             Pointer to the context of the elliptic curve
//
*F*/


IPPFUN(IppStatus, ippsGFpECNegPoint,(const IppsGFpECPoint* pP,
                                           IppsGFpECPoint* pR,
                                           IppsGFpECState* pEC))
{
   IPP_BAD_PTR3_RET(pP, pR, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pP), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pR), ippStsContextMatchErr );

   IPP_BADARG_RET( ECP_POINT_FELEN(pP)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( ECP_POINT_FELEN(pR)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   gfec_NegPoint(pR, pP, pEC);
   return ippStsNoErr;
}
