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
//        ippsGFpECCpyPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECCpyPoint
//
// Purpose: Copies one point to another
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pA == NULL
//                                   pR == NULL
//                                   pEC == NULL
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   invalid pA->idCtx
//                                   invalid pR->idCtx
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pA)!=GFP_FELEN()
//                                   ECP_POINT_FELEN(pR)!=GFP_FELEN()
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pA              Pointer to the context of the elliptic curve point being copied
//    pR              Pointer to the context of the elliptic curve point being changed
//    pEC             Pointer to the context of the elliptic curve
//
*F*/

IPPFUN(IppStatus, ippsGFpECCpyPoint,(const IppsGFpECPoint* pA,
                                           IppsGFpECPoint* pR,
                                           IppsGFpECState* pEC))
{
   IPP_BAD_PTR3_RET(pA, pR, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pA), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pR), ippStsContextMatchErr );

   IPP_BADARG_RET( ECP_POINT_FELEN(pA)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( ECP_POINT_FELEN(pR)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   gfec_CopyPoint(pR, pA, GFP_FELEN(GFP_PMA(ECP_GFP(pEC))));
   return ippStsNoErr;
}
