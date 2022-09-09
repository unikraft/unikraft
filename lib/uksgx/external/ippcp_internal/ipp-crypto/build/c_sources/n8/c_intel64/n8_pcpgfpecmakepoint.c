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
//        ippsGFpECMakePoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"


/*
   computes for given x- computes y-coodinates of P(x,y) point belonging EC if existing
*/

/*F*
// Name: ippsGFpECMakePoint
//
// Purpose: Constructs the coordinates of a point on an elliptic curve based on the X-coordinate
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pPoint == NULL
//                                   pEC == NULL
//                                   pX == NULL
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   invalid pPoint->idCtx
//                                   invalid pX->idCtx
//
//    ippStsBadArgErr                !GFP_IS_BASIC(GFP_PMA(ECP_GFP(pEC)))
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pPoint)!=GFP_FELEN()
//                                   GFPE_ROOM(pX)!=GFP_FELEN()
//
//    ippStsQuadraticNonResidueErr   square of the Y-coordinate of
//                                   the pPoint is a quadratic non-residue modulo
//
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pPoint      Pointer to the IppsGFpECPoint context
//    pEC         Pointer to the context of the elliptic curve
//    pX          Pointer to the X-coordinate of the point on the elliptic curve
//
// Note:
//    Is not a fact that computed point belongs to BP-related subgroup BP
//
*F*/

IPPFUN(IppStatus, ippsGFpECMakePoint,(const IppsGFpElement* pX, IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
{
   IPP_BAD_PTR3_RET(pX, pPoint, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFP_IS_BASIC(GFP_PMA(ECP_GFP(pEC))), ippStsBadArgErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pX), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPoint), ippStsContextMatchErr );

   IPP_BADARG_RET( GFPE_ROOM(pX)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( ECP_POINT_FELEN(pPoint)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   return gfec_MakePoint(pPoint, GFPE_DATA(pX), pEC)? ippStsNoErr : ippStsQuadraticNonResidueErr;
}
