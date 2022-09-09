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
//        gfec_ComparePoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"


#if ( ECP_PROJECTIVE_COORD == JACOBIAN )
IPP_OWN_DEFN (int, gfec_ComparePoint, (const IppsGFpECPoint* pP, const IppsGFpECPoint* pQ, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elemLen = GFP_FELEN(pGFE);

   /* P or/and Q at Infinity */
   if( !IS_ECP_FINITE_POINT(pP) )
      return !IS_ECP_FINITE_POINT(pQ)? 1:0;
   if( !IS_ECP_FINITE_POINT(pQ) )
      return !IS_ECP_FINITE_POINT(pP)? 1:0;

   /* Px==Qx && Py==Qy && Pz==Qz */
   if(  GFP_EQ(ECP_POINT_Z(pP), ECP_POINT_Z(pQ), elemLen)
      &&GFP_EQ(ECP_POINT_X(pP), ECP_POINT_X(pQ), elemLen)
      &&GFP_EQ(ECP_POINT_Y(pP), ECP_POINT_Y(pQ), elemLen))
      return 1;

   else {
      mod_mul mulF = GFP_METHOD(pGFE)->mul;
      mod_sqr sqrF = GFP_METHOD(pGFE)->sqr;

      int isEqu = 1;

      BNU_CHUNK_T* pPtmp = cpGFpGetPool(1, pGFE);
      BNU_CHUNK_T* pQtmp = cpGFpGetPool(1, pGFE);
      BNU_CHUNK_T* pPz   = cpGFpGetPool(1, pGFE);
      BNU_CHUNK_T* pQz   = cpGFpGetPool(1, pGFE);

      if(isEqu) {
         /* Px*Qz^2 ~ Qx*Pz^2 */
         if( IS_ECP_AFFINE_POINT(pQ) ) /* Ptmp = Px * Qz^2 */
            cpGFpElementCopy(pPtmp, ECP_POINT_X(pP), elemLen);
         else {
            sqrF(pQz, ECP_POINT_Z(pQ), pGFE);
            mulF(pPtmp, ECP_POINT_X(pP), pQz, pGFE);
         }
         if( IS_ECP_AFFINE_POINT(pP) ) /* Qtmp = Qx * Pz^2 */
            cpGFpElementCopy(pQtmp, ECP_POINT_X(pQ), elemLen);
         else {
            sqrF(pPz, ECP_POINT_Z(pP), pGFE);
            mulF(pQtmp, ECP_POINT_X(pQ), pPz, pGFE);
         }
         isEqu = GFP_EQ(pPtmp, pQtmp, elemLen);
      }

      if(isEqu) {
         /* Py*Qz^3 ~ Qy*Pz^3 */
         if( IS_ECP_AFFINE_POINT(pQ) ) /* Ptmp = Py * Qz^3 */
            cpGFpElementCopy(pPtmp, ECP_POINT_Y(pP), elemLen);
         else {
            mulF(pQz, ECP_POINT_Z(pQ), pQz, pGFE);
            mulF(pPtmp, pQz, ECP_POINT_Y(pP), pGFE);
         }
         if( IS_ECP_AFFINE_POINT(pP) ) /* Qtmp = Qy * Pz^3 */
            cpGFpElementCopy(pQtmp, ECP_POINT_Y(pQ), elemLen);
         else {
            mulF(pPz, ECP_POINT_Z(pP), pPz, pGFE);
            mulF(pQtmp, pPz, ECP_POINT_Y(pQ), pGFE);
         }
         isEqu = GFP_EQ(pPtmp, pQtmp, elemLen);
      }

      cpGFpReleasePool(4, pGFE);
      return isEqu;
   }
}
#endif
