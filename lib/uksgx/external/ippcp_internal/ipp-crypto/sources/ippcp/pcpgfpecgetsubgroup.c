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
//        ippsGFpECGetSubgroup()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"

/*F*
// Name: ippsGFpECGet
//
// Purpose: Extracts the parameters (base point and its order) of an elliptic curve
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pEC
//
//    ippStsContextMatchErr         invalid pEC->idCtx
//                                  NULL == pEC->subgroup
//                                  invalid pX->idCtx
//                                  invalid pY->idCtx
//
//    ippStsOutOfRangeErr           GFPE_ROOM(pX)!=GFP_FELEN(pGFE)
//                                  GFPE_ROOM(pY)!=GFP_FELEN(pGFE)
//
//    ippStsLengthErr               BN_ROOM(pOrder) < orderLen
//                                  BN_ROOM(pCofactor) < cofactorLen
//
//    ippStsNoErr                   no error
//
// Parameters:
//    ppGFp          Pointer to the pointer to the context of underlying finite field
//    pX, pY         Pointers to the X and Y coordinates of the base point of the elliptic curve
//    pOrder         Pointer to the big number context storing the order of the base point.
//    pCofactor      Pointer to the big number context storing the cofactor.
//    pEC            Pointer to the context of the elliptic curve.
//
*F*/

IPPFUN(IppStatus, ippsGFpECGetSubgroup,(IppsGFpState** const ppGFp,
                                     IppsGFpElement* pX, IppsGFpElement* pY,
                                     IppsBigNumState* pOrder,
                                     IppsBigNumState* pCofactor,
                                     const IppsGFpECState* pEC))
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   {
      const IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);
      Ipp32u elementSize = (Ipp32u)GFP_FELEN(pGFE);

      if(ppGFp) {
         *ppGFp = (IppsGFpState*)pGF;
      }

      if(pX) {
         IPP_BADARG_RET( !GFPE_VALID_ID(pX), ippStsContextMatchErr );
         IPP_BADARG_RET( GFPE_ROOM(pX)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
         cpGFpElementCopy(GFPE_DATA(pX), ECP_G(pEC), (cpSize)elementSize);
      }
      if(pY) {
         IPP_BADARG_RET( !GFPE_VALID_ID(pY), ippStsContextMatchErr );
         IPP_BADARG_RET( GFPE_ROOM(pY)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
         cpGFpElementCopy(GFPE_DATA(pY), ECP_G(pEC)+elementSize, (cpSize)elementSize);
      }

      if(pOrder) {
         BNU_CHUNK_T* pOrderData = MOD_MODULUS(ECP_MONT_R(pEC));
         int orderBitSize = ECP_ORDBITSIZE(pEC);
         int orderLen = BITS_BNU_CHUNK(orderBitSize);
         FIX_BNU(pOrderData, orderLen);

         IPP_BADARG_RET(!BN_VALID_ID(pOrder), ippStsContextMatchErr);
         IPP_BADARG_RET(BN_ROOM(pOrder) < orderLen, ippStsLengthErr);

         ZEXPAND_COPY_BNU(BN_NUMBER(pOrder), BN_ROOM(pOrder), pOrderData, orderLen);
         BN_SIZE(pOrder) = orderLen;
         BN_SIGN(pOrder) = ippBigNumPOS;
      }

      if(pCofactor) {
         BNU_CHUNK_T* pCofactorData = ECP_COFACTOR(pEC);
         int cofactorLen = (cpSize)elementSize;
         FIX_BNU(pCofactorData, cofactorLen);

         IPP_BADARG_RET(!BN_VALID_ID(pCofactor), ippStsContextMatchErr);
         IPP_BADARG_RET(BN_ROOM(pCofactor) < cofactorLen, ippStsLengthErr);

         ZEXPAND_COPY_BNU(BN_NUMBER(pCofactor), BN_ROOM(pCofactor), pCofactorData, cofactorLen);
         BN_SIZE(pCofactor) = cofactorLen;
         BN_SIGN(pCofactor) = ippBigNumPOS;
      }

      return ippStsNoErr;
   }
}
