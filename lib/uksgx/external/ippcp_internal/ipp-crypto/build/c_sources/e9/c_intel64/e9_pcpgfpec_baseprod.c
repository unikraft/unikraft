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
//        gfec_BasePointProduct()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"


IPP_OWN_DEFN (IppsGFpECPoint*, gfec_BasePointProduct, (IppsGFpECPoint* pR, const BNU_CHUNK_T* pScalarG, int scalarGlen, const IppsGFpECPoint* pP, const BNU_CHUNK_T* pScalarP, int scalarPlen, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
{
   FIX_BNU(pScalarG, scalarGlen);
   FIX_BNU(pScalarP, scalarPlen);

   {
      gsModEngine* pGForder = ECP_MONT_R(pEC);
      int orderBits = MOD_BITSIZE(pGForder);
      int orderLen  = MOD_LEN(pGForder);
      BNU_CHUNK_T* tmpScalarG = cpGFpGetPool(2, pGForder);
      BNU_CHUNK_T* tmpScalarP = tmpScalarG+orderLen+1;

      cpGFpElementCopyPad(tmpScalarG, orderLen+1, pScalarG,scalarGlen);
      cpGFpElementCopyPad(tmpScalarP, orderLen+1, pScalarP,scalarPlen);

      if(ECP_PREMULBP(pEC)) {
         BNU_CHUNK_T* productG = cpEcGFpGetPool(2, pEC);
         BNU_CHUNK_T* productP = productG+ECP_POINTLEN(pEC);

         gfec_base_point_mul(productG, (Ipp8u*)tmpScalarG, orderBits, pEC);
         gfec_point_mul(productP, ECP_POINT_X(pP), (Ipp8u*)tmpScalarP, orderBits, pEC, pScratchBuffer);
         gfec_point_add(ECP_POINT_X(pR), productG, productP, pEC);

         cpEcGFpReleasePool(2, pEC);
      }

      else {
         gfec_point_prod(ECP_POINT_X(pR),
                         ECP_G(pEC), (Ipp8u*)tmpScalarG,
                         ECP_POINT_X(pP), (Ipp8u*)tmpScalarP,
                         orderBits,
                         pEC, pScratchBuffer);
      }

      cpGFpReleasePool(2, pGForder);
   }

   ECP_POINT_FLAGS(pR) = gfec_IsPointAtInfinity(pR)? 0 : ECP_FINITE_POINT;
   return pR;
}
