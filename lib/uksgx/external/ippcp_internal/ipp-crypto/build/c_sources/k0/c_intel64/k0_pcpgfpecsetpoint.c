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
//        ippsGFpECSetPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECSetPoint
//
// Purpose: Sets a point on an elliptic curve
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pPoint == NULL
//                              pEC == NULL
//                              pX == NULL
//                              pY == NULL
//
//    ippStsContextMatchErr     invalid pEC->idCtx
//                              invalid pPoint->idCtx
//                              invalid pX->idCtx
//                              invalid pY->idCtx
//
//    ippStsOutOfRangeErr       ECP_POINT_FELEN(pX)!=GFP_FELEN()
//                              ECP_POINT_FELEN(pY)!=GFP_FELEN()
//                              ECP_POINT_FELEN(pPoint)!=GFP_FELEN()
//
//    ippStsNoErr               no error
//
// Parameters:
//    pX, pY    Pointers to the X and Y coordinates of a point on the elliptic curve
//    pPoint    Pointer to the IppsGFpECPoint context
//    pEC       Pointer to the context of the elliptic curve
//
*F*/

IPPFUN(IppStatus, ippsGFpECSetPoint,(const IppsGFpElement* pX, const IppsGFpElement* pY,
                                           IppsGFpECPoint* pPoint,
                                           IppsGFpECState* pEC))
{
   IPP_BAD_PTR2_RET(pPoint, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPoint), ippStsContextMatchErr );

   IPP_BAD_PTR2_RET(pX, pY);
   IPP_BADARG_RET( !GFPE_VALID_ID(pX), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pY), ippStsContextMatchErr );

   IPP_BADARG_RET( GFPE_ROOM(pX)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( GFPE_ROOM(pY)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( ECP_POINT_FELEN(pPoint)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   if(gfec_SetPoint(ECP_POINT_DATA(pPoint), GFPE_DATA(pX), GFPE_DATA(pY), pEC))
      ECP_POINT_FLAGS(pPoint) = ECP_AFFINE_POINT | ECP_FINITE_POINT;
   else
      ECP_POINT_FLAGS(pPoint) = 0;
   return ippStsNoErr;
}
