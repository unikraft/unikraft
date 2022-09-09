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
//        ippsGFpECSetSubgroup()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"

/*F*
// Name: ippsGFpECSetSubgroup
//
// Purpose: Sets up the parameters defining an elliptic curve points subgroup.
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pEC
//                                  NULL == pX
//                                  NULL == pY
//                                  NULL == pOrder
//                                  NULL == pCofactor
//
//    ippStsContextMatchErr         invalid pEC->idCtx
//                                  invalid pX->idCtx
//                                  invalid pY->idCtx
//                                  invalid pOrder->idCtx
//                                  invalid pCofactor->idCtx
//
//    ippStsBadArgErr               pOrder <= 0
//                                  pCofactor <= 0
//
//    ippStsOutOfRangeErr           GFPE_ROOM(pX)!=GFP_FELEN(pGFE)
//                                  GFPE_ROOM(pY)!=GFP_FELEN(pGFE)
//
//    ippStsRangeErr                orderBitSize>maxOrderBits
//                                  cofactorBitSize>elemLen*BITSIZE(BNU_CHUNK_T)
//
//    ippStsNoErr                   no error
//
// Parameters:
//    pX, pY         Pointers to the X and Y coordinates of the base point of the elliptic curve
//    pOrder         Pointer to the big number context storing the order of the base point.
//    pCofactor      Pointer to the big number context storing the cofactor.
//    pEC            Pointer to the context of the elliptic curve.
//
*F*/

IPPFUN(IppStatus, ippsGFpECSetSubgroup,(const IppsGFpElement* pX, const IppsGFpElement* pY,
                                        const IppsBigNumState* pOrder,
                                        const IppsBigNumState* pCofactor,
                                        IppsGFpECState* pEC))
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );

   IPP_BAD_PTR2_RET(pX, pY);
   IPP_BADARG_RET( !GFPE_VALID_ID(pX), ippStsContextMatchErr );
   IPP_BADARG_RET( !GFPE_VALID_ID(pY), ippStsContextMatchErr );

   IPP_BAD_PTR2_RET(pOrder, pCofactor);
   IPP_BADARG_RET(!BN_VALID_ID(pOrder), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_SIGN(pOrder)!= IppsBigNumPOS, ippStsBadArgErr);

   IPP_BADARG_RET(!BN_VALID_ID(pCofactor), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_SIGN(pCofactor)!= IppsBigNumPOS, ippStsBadArgErr);

   {
      gsModEngine* pGFE = GFP_PMA(ECP_GFP(pEC));
      int elemLen = GFP_FELEN(pGFE);

      IPP_BADARG_RET( GFPE_ROOM(pX)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);
      IPP_BADARG_RET( GFPE_ROOM(pY)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);

      gfec_SetPoint(ECP_G(pEC), GFPE_DATA(pX), GFPE_DATA(pY), pEC);

      {
         int maxOrderBits = 1+ cpGFpBasicDegreeExtension(pGFE) * GFP_FEBITLEN(cpGFpBasic(pGFE));
         BNU_CHUNK_T* pOrderData = BN_NUMBER(pOrder);
         int orderLen= BN_SIZE(pOrder);
         int orderBitSize = BITSIZE_BNU(pOrderData, orderLen);
         IPP_BADARG_RET(orderBitSize>maxOrderBits, ippStsRangeErr)

         /* set actual size of order and re-init engine */
         ECP_ORDBITSIZE(pEC) = orderBitSize;
         gsModEngineInit(ECP_MONT_R(pEC),(Ipp32u*)pOrderData, orderBitSize, MONT_DEFAULT_POOL_LENGTH, gsModArithMont());
      }

      {
         BNU_CHUNK_T* pCofactorData = BN_NUMBER(pCofactor);
         int cofactorLen= BN_SIZE(pCofactor);
         int cofactorBitSize = BITSIZE_BNU(pCofactorData, cofactorLen);
         IPP_BADARG_RET(cofactorBitSize>elemLen*BITSIZE(BNU_CHUNK_T), ippStsRangeErr)
         COPY_BNU(ECP_COFACTOR(pEC), pCofactorData, cofactorLen);
      }

      ECP_SUBGROUP(pEC) = 1;
      return ippStsNoErr;
   }
}
