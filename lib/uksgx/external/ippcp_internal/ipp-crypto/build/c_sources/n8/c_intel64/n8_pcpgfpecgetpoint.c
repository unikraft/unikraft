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
//        ippsGFpECGetPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECGetPoint
//
// Purpose: Retrieves coordinates of a point on an elliptic curve
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pPoint == NULL
//                                   pEC == NULL
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   invalid pPoint->idCtx
//                                   pX != NULL && invalid pX->idCtx
//                                   pY != NULL && invalid pY->idCtx
//                                   pX != NULL && GFPE_ROOM(pX)!=GFP_FELEN()
//                                   pY != NULL && GFPE_ROOM(pY)!=GFP_FELEN()
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pPoint)!=GFP_FELEN()
//                                   pX != NULL && GFPE_ROOM(pX)!=GFP_FELEN()
//                                   pY != NULL && GFPE_ROOM(pY)!=GFP_FELEN()
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pPoint      Pointer to the IppsGFpECPoint context
//    pEC         Pointer to the context of the elliptic curve
//    pX, pY      Pointers to the X and Y coordinates of a point on the elliptic curve
//
// Note:
//    Is not a fact that computed point belongs to BP-related subgroup BP
//
*F*/

IPPFUN(IppStatus, ippsGFpECGetPoint,(const IppsGFpECPoint* pPoint,
                                           IppsGFpElement* pX, IppsGFpElement* pY,
                                           IppsGFpECState* pEC))
{
   IPP_BAD_PTR2_RET(pPoint, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPoint), ippStsContextMatchErr );

   IPP_BADARG_RET( pX && !GFPE_VALID_ID(pX), ippStsContextMatchErr );
   IPP_BADARG_RET( pY && !GFPE_VALID_ID(pY), ippStsContextMatchErr );

   IPP_BADARG_RET( pX && GFPE_ROOM(pX)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( pY && GFPE_ROOM(pY)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( ECP_POINT_FELEN(pPoint)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   /* returns (X,Y) == (0,0) if Point is at infinity */
   gfec_GetPoint((pX)? GFPE_DATA(pX):NULL, (pY)? GFPE_DATA(pY):NULL, pPoint, pEC);
   return ippStsNoErr;
}
