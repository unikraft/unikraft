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
//        gfec_MakePoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"

IPP_OWN_DEFN (int, gfec_MakePoint, (IppsGFpECPoint* pPoint, const BNU_CHUNK_T* pElm, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elemLen = GFP_FELEN(pGFE);

   mod_mul mulF = GFP_METHOD(pGFE)->mul;
   mod_sqr sqrF = GFP_METHOD(pGFE)->sqr;
   mod_add addF = GFP_METHOD(pGFE)->add;

   BNU_CHUNK_T* pX = ECP_POINT_X(pPoint);
   BNU_CHUNK_T* pY = ECP_POINT_Y(pPoint);
   BNU_CHUNK_T* pZ = ECP_POINT_Z(pPoint);

   /* set x-coordinate */
   cpGFpElementCopy(pX, pElm, elemLen);

   /* T = X^3 + A*X + B */
   sqrF(pY, pX, pGFE);
   mulF(pY, pY, pX, pGFE);
   if(ECP_SPECIFIC(pEC)!=ECP_EPID2) {
      mulF(pZ, ECP_A(pEC), pX, pGFE);
      addF(pY, pY, pZ, pGFE);
   }
   addF(pY, pY, ECP_B(pEC), pGFE);

   /* set z-coordinate =1 */
   cpGFpElementCopyPad(pZ, elemLen, GFP_MNT_R(pGFE), elemLen);

   /* Y = sqrt(Y) */
   if( cpGFpSqrt(pY, pY, pGFE) ) {
      ECP_POINT_FLAGS(pPoint) = ECP_AFFINE_POINT | ECP_FINITE_POINT;
      return 1;
   }
   else {
      gfec_SetPointAtInfinity(pPoint);
      return 0;
   }
}
