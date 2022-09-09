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
//        gfec_NegPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"


IPP_OWN_DEFN (IppsGFpECPoint*, gfec_NegPoint, (IppsGFpECPoint* pR, const IppsGFpECPoint* pP, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elmLen = GFP_FELEN(pGFE);
   if(pR!=pP)
      gfec_CopyPoint(pR, pP, elmLen);
   GFP_METHOD(pGFE)->neg(ECP_POINT_Y(pR), ECP_POINT_Y(pP), pGFE);
   return pR;
}
