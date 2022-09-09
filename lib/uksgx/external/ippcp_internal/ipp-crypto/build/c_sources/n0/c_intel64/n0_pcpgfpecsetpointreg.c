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
//        ippsGFpECSetPointRegular()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECSetPointRegular
//
// Purpose: Sets a point with Big Number coordinates
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
//    ippStsOutOfRangeErr       GFP_FELEN() < pX <= 0
//                              GFP_FELEN() < pY <= 0
//                              ECP_POINT_FELEN(pPoint)!=GFP_FELEN()
//
//    ippStsNoErr              no error
//
// Parameters:
//    pX, pY    Pointers to the X and Y coordinates of a point on the elliptic curve
//    pPoint    Pointer to the IppsGFpECPoint context
//    pEC       Pointer to the context of the elliptic curve
//
*F*/

IPPFUN(IppStatus, ippsGFpECSetPointRegular,(const IppsBigNumState* pX, const IppsBigNumState* pY,
                                           IppsGFpECPoint* pPoint,
                                           IppsGFpECState* pEC))
{
   IPP_BAD_PTR2_RET(pPoint, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPoint), ippStsContextMatchErr );

   IPP_BAD_PTR2_RET(pX, pY);
   IPP_BADARG_RET( !BN_VALID_ID(pX), ippStsContextMatchErr );
   IPP_BADARG_RET( !BN_VALID_ID(pY), ippStsContextMatchErr );
   IPP_BADARG_RET( !BN_POSITIVE(pX), ippStsOutOfRangeErr);
   IPP_BADARG_RET( !BN_POSITIVE(pY), ippStsOutOfRangeErr);

   {
      IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);
      int elemLen = GFP_FELEN(pGFE);

      IPP_BADARG_RET( !GFP_IS_BASIC(pGFE), ippStsBadArgErr )

      IPP_BADARG_RET( BN_SIZE(pX) > elemLen, ippStsOutOfRangeErr);
      IPP_BADARG_RET( BN_SIZE(pY) > elemLen, ippStsOutOfRangeErr);
      IPP_BADARG_RET( ECP_POINT_FELEN(pPoint)!=elemLen, ippStsOutOfRangeErr);

      {
         BNU_CHUNK_T* pointX = ECP_POINT_X(pPoint);
         BNU_CHUNK_T* pointY = ECP_POINT_Y(pPoint);
         BNU_CHUNK_T* pointZ = ECP_POINT_Z(pPoint);

         cpGFpElementCopyPad(pointX, elemLen, BN_NUMBER(pX), BN_SIZE(pX));
         cpGFpElementCopyPad(pointY, elemLen, BN_NUMBER(pY), BN_SIZE(pY));
         cpGFpElementCopy(pointZ, MOD_MNT_R(pGFE), elemLen);

         if( cpGFpSet(pointX, pointX, elemLen, pGFE) && cpGFpSet(pointY, pointY, elemLen, pGFE) )
            ECP_POINT_FLAGS(pPoint) = ECP_AFFINE_POINT | ECP_FINITE_POINT;
         else
            gfec_SetPointAtInfinity(pPoint);

         return ippStsNoErr;
      }
   }
}
