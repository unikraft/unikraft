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
//        gfec_GetPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"


#if ( ECP_PROJECTIVE_COORD == JACOBIAN )
/* returns 1/0 if point is finite/infinite */
IPP_OWN_DEFN (int, gfec_GetPoint, (BNU_CHUNK_T* pX, BNU_CHUNK_T* pY, const IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elemLen = GFP_FELEN(pGFE);

   if( !IS_ECP_FINITE_POINT(pPoint) ) {
      if(pX) cpGFpElementPad(pX, elemLen, 0);
      if(pY) cpGFpElementPad(pY, elemLen, 0);
      return 0;
   }

   /* affine point (1==Z) */
   if( IS_ECP_AFFINE_POINT(pPoint) ) {
      if(pX)
         cpGFpElementCopy(pX, ECP_POINT_X(pPoint), elemLen);
      if(pY)
         cpGFpElementCopy(pY, ECP_POINT_Y(pPoint), elemLen);
      return 1;
   }

   /* projective point (1!=Z) */
   {
      mod_mul mulF = GFP_METHOD(pGFE)->mul;
      mod_sqr sqrF = GFP_METHOD(pGFE)->sqr;

      /* T = (1/Z)*(1/Z) */
      BNU_CHUNK_T* pT    = cpGFpGetPool(1, pGFE);
      BNU_CHUNK_T* pZinv = cpGFpGetPool(1, pGFE);
      BNU_CHUNK_T* pU = cpGFpGetPool(1, pGFE);
      cpGFpxInv(pZinv, ECP_POINT_Z(pPoint), pGFE);
      sqrF(pT, pZinv, pGFE);

      if(pX) {
         mulF(pU, ECP_POINT_X(pPoint), pT, pGFE);
         cpGFpElementCopy(pX, pU, elemLen);
      }
      if(pY) {
         mulF(pT, pZinv, pT, pGFE);
         mulF(pU, ECP_POINT_Y(pPoint), pT, pGFE);
         cpGFpElementCopy(pY, pU, elemLen);
      }

      cpGFpReleasePool(3, pGFE);
      return 1;
   }
}
#endif
