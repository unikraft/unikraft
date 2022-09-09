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
//     Internal EC over GF(p^m) basic Definitions & Function Prototypes
// 
//     Context:
//        gfec_SetPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"

static int gfec_IsAffinePointAtInfinity(int ecInfinity,
                           const BNU_CHUNK_T* pX, const BNU_CHUNK_T* pY,
                           const IppsGFpState* pGF)
{
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elmLen = GFP_FELEN(pGFE);

   int atInfinity = GFP_IS_ZERO(pX,elmLen);

   BNU_CHUNK_T* tmpY = cpGFpGetPool(1, pGFE);

   /* set tmpY either:
   // 0,       if ec.b !=0
   // mont(1)  if ec.b ==0
   */
   cpGFpElementPad(tmpY, elmLen, 0);
   if(ecInfinity) {
      gsModEngine* pBasicGFE = cpGFpBasic(pGFE);
      int basicElmLen = GFP_FELEN(pBasicGFE);
      BNU_CHUNK_T* mont1 = GFP_MNT_R(pBasicGFE);
      cpGFpElementCopyPad(tmpY, elmLen, mont1, basicElmLen);
   }

   /* check if (x,y) represents point at infinity */
   atInfinity &= GFP_EQ(pY, tmpY, elmLen);

   cpGFpReleasePool(1, pGFE);
   return atInfinity;
}

/* returns: 1/0 if set up finite/infinite point */
IPP_OWN_DEFN (int, gfec_SetPoint, (BNU_CHUNK_T* pPointData, const BNU_CHUNK_T* pX, const BNU_CHUNK_T* pY, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elmLen = GFP_FELEN(pGFE);

   int finite_point= !gfec_IsAffinePointAtInfinity(ECP_INFINITY(pEC), pX, pY, pGF);
   if(finite_point) {
      gsModEngine* pBasicGFE = cpGFpBasic(pGFE);
      cpGFpElementCopy(pPointData, pX, elmLen);
      cpGFpElementCopy(pPointData+elmLen, pY, elmLen);
      cpGFpElementCopyPad(pPointData+elmLen*2, elmLen, GFP_MNT_R(pBasicGFE), GFP_FELEN(pBasicGFE));
   }
   else
      cpGFpElementPad(pPointData, 3*elmLen, 0);

   return finite_point;
}
