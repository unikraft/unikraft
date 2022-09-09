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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     EC over GF(p^m) definitinons
// 
//     Context:
//        ippsGFpECSet()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"

/*F*
// Name: ippsGFpECSet
//
// Purpose: Sets up the parameters of an elliptic curve over a finite field
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pEC
//                                  NULL == pA
//                                  NULL == pB
//
//    ippStsContextMatchErr         invalid pEC->idCtx
//                                  invalid pA->idCtx
//                                  invalid pB->idCtx
//
//    ippStsOutOfRangeErr           GFPE_ROOM(pA)!=GFP_FELEN(pGFE)
//                                  GFPE_ROOM(pB)!=GFP_FELEN(pGFE)
//
//    ippStsNoErr                   no error
//
// Parameters:
//    pA        Pointer to the coefficient A of the equation defining the elliptic curve
//    pB        Pointer to the coefficient B of the equation defining the elliptic curve
//    pEC       Pointer to the context of the elliptic curve
//
*F*/

IPPFUN(IppStatus, ippsGFpECSet,(const IppsGFpElement* pA,
                                const IppsGFpElement* pB,
                                IppsGFpECState* pEC))
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );

   IPP_BAD_PTR2_RET(pA, pB);
   IPP_BADARG_RET( !GFPE_VALID_ID(pA), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pB), ippStsContextMatchErr );

   {
      gsModEngine* pGFE = GFP_PMA(ECP_GFP(pEC));
      int elemLen = GFP_FELEN(pGFE);

      IPP_BADARG_RET( GFPE_ROOM(pA)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
      IPP_BADARG_RET( GFPE_ROOM(pB)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);

      /* copy A */
      cpGFpElementPad(ECP_A(pEC), elemLen, 0);
      cpGFpElementCopy(ECP_A(pEC), GFPE_DATA(pA), elemLen);
      /* and set up A-specific (a==0 or a==-3) if is */
      if(GFP_IS_ZERO(ECP_A(pEC), elemLen))
         ECP_SPECIFIC(pEC) = ECP_EPID2;

      cpGFpElementSetChunk(ECP_B(pEC), elemLen, 3);
      GFP_METHOD(pGFE)->encode(ECP_B(pEC), ECP_B(pEC), pGFE);
      GFP_METHOD(pGFE)->add(ECP_B(pEC), ECP_A(pEC), ECP_B(pEC), pGFE);
      if(GFP_IS_ZERO(ECP_B(pEC), elemLen))
         ECP_SPECIFIC(pEC) = ECP_STD;

      /* copy B */
      cpGFpElementPad(ECP_B(pEC), elemLen, 0);
      cpGFpElementCopy(ECP_B(pEC), GFPE_DATA(pB), elemLen);
      /* and set type of affine infinity representation:
      // (0,1) if B==0
      // (0,0) if B!=0 */
      ECP_INFINITY(pEC) = GFP_IS_ZERO(ECP_B(pEC), elemLen);

      return ippStsNoErr;
   }
}
