/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     Cryptography Primitive.
//     EC over Prime Finite Field (EC Point operations)
// 
//  Contents:
//        ippsECCPGetPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPGetPoint
//
// Purpose: Converts  internal presentation EC point - montgomery projective
//          into regular affine coordinates EC point (pX,pY)
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pPoint
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//                            illegal pPoint->idCtx
//                            NULL != pX, illegal pX->idCtx
//                            NULL != pY, illegal pY->idCtx
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pX          pointer to the regular affine coordinate X
//    pY          pointer to the regular affine coordinate Y
//    pPoint      pointer to the EC Point context
//    pEC        pointer to the ECCP context
//
*F*/
IPPFUN(IppStatus, ippsECCPGetPoint,(IppsBigNumState* pX, IppsBigNumState* pY,
                                  const IppsECCPPointState* pPoint,
                                  IppsECCPState* pEC))
{
   /* test pEC */
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   /* test pX and pY */
   if(pX) {
      IPP_BADARG_RET(!BN_VALID_ID(pX), ippStsContextMatchErr);
   }
   if(pY) {
      IPP_BADARG_RET(!BN_VALID_ID(pY), ippStsContextMatchErr);
   }

   {
      IppStatus sts;

      IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);
      int elemLen = GFP_FELEN(pGFE);

      mod_decode decode = GFP_METHOD(pGFE)->decode;  /* gf decode method  */

      IppsGFpElement elmX, elmY;

      cpGFpElementConstruct(&elmX, cpGFpGetPool(1, pGFE), elemLen);
      cpGFpElementConstruct(&elmY, cpGFpGetPool(1, pGFE), elemLen);
      do {
         sts = ippsGFpECGetPoint(pPoint, pX? &elmX:NULL, pY? &elmY:NULL, pEC);
         if(ippStsNoErr!=sts) break;

         if(pX) {
            decode(elmX.pData, elmX.pData, pGFE);
            sts = ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)elmX.pData, pX);
            if(ippStsNoErr!=sts) break;
         }
         if(pY) {
            decode(elmY.pData, elmY.pData, pGFE);
            sts = ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)elmY.pData, pY);
            if(ippStsNoErr!=sts) break;
         }
      } while(0);

      cpGFpReleasePool(2, pGFE);
      return sts;
   }
}
