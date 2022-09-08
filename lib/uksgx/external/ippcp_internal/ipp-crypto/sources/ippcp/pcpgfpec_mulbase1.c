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
//        gfec_MulBasePoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"

IPP_OWN_DEFN (IppsGFpECPoint*, gfec_MulBasePoint, (IppsGFpECPoint* pR, const BNU_CHUNK_T* pScalar, int scalarLen, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
{
   FIX_BNU(pScalar, scalarLen);
   {
      gsModEngine* pGForder = ECP_MONT_R(pEC);

      BNU_CHUNK_T* pTmpScalar = cpGFpGetPool(1, pGForder); /* length of scalar does not exceed length of order */
      int orderBits = MOD_BITSIZE(pGForder);
      int orderLen  = MOD_LEN(pGForder);
      cpGFpElementCopyPad(pTmpScalar,orderLen+1, pScalar,scalarLen);

      if(ECP_PREMULBP(pEC))
         gfec_base_point_mul(ECP_POINT_X(pR),
                             (Ipp8u*)pTmpScalar, orderBits,
                             pEC);
      else
         gfec_point_mul(ECP_POINT_X(pR), ECP_G(pEC),
                        (Ipp8u*)pTmpScalar, orderBits,
                        pEC, pScratchBuffer);
      cpGFpReleasePool(1, pGForder);

      ECP_POINT_FLAGS(pR) = gfec_IsPointAtInfinity(pR)? 0 : ECP_FINITE_POINT;
      return pR;
   }
}
