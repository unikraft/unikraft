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
//        ippsGFpECGetPointRegular()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECGetPointRegular
//
// Purpose: Retrieves coordinates of a point on an elliptic curve in the regular domain
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pPoint == NULL
//                                   pEC == NULL
//
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   invalid pPoint->idCtx
//                                   invalid pX->idCtx
//                                   invalid pY->idCtx
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pPoint)!=GFP_FELEN()
//                                   BN_ROOM(pX)*BNU_CHUNK_BITS<GFP_FEBITLEN(pGFE))
//                                   BN_ROOM(pY)*BNU_CHUNK_BITS<GFP_FEBITLEN(pGFE))
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pPoint      Pointer to the IppsGFpECPoint context
//    pEC         Pointer to the context of the elliptic curve
//    pX, pY      Pointers to the X and Y coordinates of a point on the elliptic curve
//
*F*/

IPPFUN(IppStatus, ippsGFpECGetPointRegular,(const IppsGFpECPoint* pPoint,
                                                  IppsBigNumState* pX, IppsBigNumState* pY,
                                                  IppsGFpECState* pEC))
{
   IppsGFpState*  pGF;
   gsModEngine* pGFE;

   IPP_BAD_PTR2_RET(pPoint, pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPoint), ippStsContextMatchErr );

   pGF = ECP_GFP(pEC);
   pGFE = GFP_PMA(pGF);

   if(pX) {
      IPP_BADARG_RET(!BN_VALID_ID(pX), ippStsContextMatchErr );
      IPP_BADARG_RET(BN_ROOM(pX)*BNU_CHUNK_BITS<GFP_FEBITLEN(pGFE), ippStsOutOfRangeErr);
   }
   if(pY) {
      IPP_BADARG_RET(!BN_VALID_ID(pY), ippStsContextMatchErr );
      IPP_BADARG_RET(BN_ROOM(pY)*BNU_CHUNK_BITS<GFP_FEBITLEN(pGFE), ippStsOutOfRangeErr);
   }

   {
      int elmLen = GFP_FELEN(pGFE);
      BNU_CHUNK_T* x = cpGFpGetPool(2, pGFE);
      BNU_CHUNK_T* y = x + elmLen;

      /* returns (X,Y) == (0,0) if Point is at infinity */
      gfec_GetPoint((pX)? x:NULL, (pY)? y:NULL, pPoint, pEC);

      /* convert into refular domain */
      if(pX) {
         GFP_METHOD(pGFE)->decode(x, x, pGFE);
         ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)x, pX);
      }
      if(pY) {
         GFP_METHOD(pGFE)->decode(y, y, pGFE);
         ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)y, pY);
      }

      cpGFpReleasePool(2, pGFE);
      return ippStsNoErr;
   }
}
