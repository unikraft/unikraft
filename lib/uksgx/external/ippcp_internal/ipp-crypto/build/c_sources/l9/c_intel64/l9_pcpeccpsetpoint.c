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
//        ippsECCPSetPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPSetPoint
//
// Purpose: Converts regular affine coordinates EC point (pX,pY)
//          into internal presentation - montgomery projective.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pPoint
//                            NULL == pX
//                            NULL == pY
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//                            illegal pX->idCtx
//                            illegal pY->idCtx
//                            illegal pPoint->idCtx
//
//    ippStsOutOfECErr        point out-of EC
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pX          pointer to the regular affine coordinate X
//    pY          pointer to the regular affine coordinate Y
//    pPoint      pointer to the EC Point context
//    pEC        pointer to the ECCP context
//
// Note:
//    if B==0 and (x,y)=(0,y) then point at Infinity will be set up
//    if B!=0 and (x,y)=(0,0) then point at Infinity will be set up
//    else point with requested coordinates (x,y) wil be set up
//    There are no check validation inside!
//
*F*/
IPPFUN(IppStatus, ippsECCPSetPoint,(const IppsBigNumState* pX,
                                        const IppsBigNumState* pY,
                                        IppsECCPPointState* pPoint,
                                        IppsECCPState* pEC))
{
   /* test pEC */
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   /* test pX and pY */
   IPP_BAD_PTR2_RET(pX,pY);
   IPP_BADARG_RET(!BN_VALID_ID(pX), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pY), ippStsContextMatchErr);

   {
      IppStatus sts;

      IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);
      int elemLen = GFP_FELEN(pGFE);
      IppsGFpElement elmX, elmY;

      cpGFpElementConstruct(&elmX, cpGFpGetPool(1, pGFE), elemLen);
      cpGFpElementConstruct(&elmY, cpGFpGetPool(1, pGFE), elemLen);
      do {
         BNU_CHUNK_T* pData = BN_NUMBER(pX);
         int ns = BN_SIZE(pX);
         sts = ippsGFpSetElement((Ipp32u*)pData, BITS2WORD32_SIZE(BITSIZE_BNU(pData, ns)), &elmX, pGF);
         if(ippStsNoErr!=sts) break;
         pData = BN_NUMBER(pY);
         ns = BN_SIZE(pY);
         sts = ippsGFpSetElement((Ipp32u*)pData, BITS2WORD32_SIZE(BITSIZE_BNU(pData, ns)), &elmY, pGF);
         if(ippStsNoErr!=sts) break;
         sts = ippsGFpECSetPoint(&elmX, &elmY, pPoint, pEC);
      } while(0);

      cpGFpReleasePool(2, pGFE);
      return sts;
   }
}
