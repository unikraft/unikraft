/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     Cryptography Primitive.
//     EC over Prime Finite Field (Verify Signature, NR version)
// 
//  Contents:
//     ippsECCPVerifyNR()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsECCPVerifyNR
//
// Purpose: Verify Signature (NR version).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pMsgDigest
//                               NULL == pSignX
//                               NULL == pSignY
//                               NULL == pResult
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               illegal pMsgDigest->idCtx
//                               illegal pSignX->idCtx
//                               illegal pSignY->idCtx
//
//    ippStsMessageErr           0> MsgDigest
//                               order<= MsgDigest
//
//    ippStsRangeErr             SignX < 0 or SignY < 0
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to be signed
//    pSignX,pSignY  pointer to the signature
//    pResult        pointer to the result: ippECValid/ippECInvalidSignature
//    pEC           pointer to the ECCP context
//
// Note:
//    - signer's key must be set up in ECCP context
//      before ippsECCPVerifyNR() usage
//
*F*/
IPPFUN(IppStatus, ippsECCPVerifyNR,(const IppsBigNumState* pMsgDigest,
                                    const IppsBigNumState* pSignX, const IppsBigNumState* pSignY,
                                    IppECResult* pResult,
                                    IppsECCPState* pEC))
{
   gsModEngine* pModEngine;
   BNU_CHUNK_T* pOrder;
   int orderLen;

   BNU_CHUNK_T* pMsgData;
   int msgLen;

   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   pModEngine = ECP_MONT_R(pEC);
   pOrder = MOD_MODULUS(pModEngine);
   orderLen = MOD_LEN(pModEngine);

   /* test message representative */
   IPP_BAD_PTR1_RET(pMsgDigest);
   IPP_BADARG_RET(!BN_VALID_ID(pMsgDigest), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pMsgDigest), ippStsMessageErr);

   pMsgData = BN_NUMBER(pMsgDigest);
   msgLen = BN_SIZE(pMsgDigest);
   IPP_BADARG_RET(0<=cpCmp_BNU(pMsgData, msgLen, pOrder, orderLen), ippStsMessageErr);

   /* test result */
   IPP_BAD_PTR1_RET(pResult);

   /* test signature */
   IPP_BAD_PTR2_RET(pSignX,pSignY);
   IPP_BADARG_RET(!BN_VALID_ID(pSignX), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pSignY), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pSignX), ippStsRangeErr);
   IPP_BADARG_RET(BN_NEGATIVE(pSignY), ippStsRangeErr);

   {
      IppECResult vResult = ippECInvalidSignature;

      IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);

      int elmLen = GFP_FELEN(pGFE);
      int pelmLen = GFP_PELEN(pGFE);

      BNU_CHUNK_T* pH1 = cpGFpGetPool(3, pGFE);
      BNU_CHUNK_T* pH2 = pH1 + pelmLen;
      BNU_CHUNK_T* pR1 = pH2 + pelmLen;
      BNU_CHUNK_T* pF  = pR1 + pelmLen;

      /* test signature value */
      if(0<cpBN_tst(pSignX) && 0<cpBN_tst(pSignY) &&
         0>cpCmp_BNU(BN_NUMBER(pSignX),BN_SIZE(pSignX), pOrder,orderLen) &&
         0>cpCmp_BNU(BN_NUMBER(pSignY),BN_SIZE(pSignY), pOrder,orderLen)) {

         /* validate signature */
         IppsGFpECPoint P, G, Public;
         cpEcGFpInitPoint(&P, cpEcGFpGetPool(1, pEC),0, pEC);
         cpEcGFpInitPoint(&G, ECP_G(pEC), ECP_AFFINE_POINT|ECP_FINITE_POINT, pEC);
         cpEcGFpInitPoint(&Public, ECP_PUBLIC(pEC), ECP_FINITE_POINT, pEC);

         /* expand signature: H1 = signY, H2 = signX */
         cpGFpElementCopyPad(pH1, orderLen, BN_NUMBER(pSignY), BN_SIZE(pSignY));
         cpGFpElementCopyPad(pH2, orderLen, BN_NUMBER(pSignX), BN_SIZE(pSignX));

         /* compute H1*BasePoint + H2*publicKey */
         gfec_BasePointProduct(&P,
                               pH1, orderLen, &Public, pH2, orderLen,
                               pEC, (Ipp8u*)ECP_SBUFFER(pEC));

         /* check that P!=O */
         if( !gfec_IsPointAtInfinity(&P)) {
            /* P.X */
            gfec_GetPoint(pH1, NULL, &P, pEC);
            /* H1 = int(P.X) mod order */
            GFP_METHOD(pGFE)->decode(pH1, pH1, pGFE);
            elmLen = cpMod_BNU(pH1, elmLen, pOrder, orderLen);
            cpGFpElementPad(pH1+elmLen, orderLen-elmLen, 0);

            /* recovered message: (SignX - H1) mod order */
            cpModSub_BNU(pH1, pH2, pH1, pOrder, orderLen, pF);

            /* and compare with input message*/
            cpGFpElementCopyPad(pH2, orderLen, pMsgData, msgLen);
            if(GFP_EQ(pH1, pH2, orderLen))
               vResult = ippECValid;
         }

         cpEcGFpReleasePool(1, pEC);
      }

      cpGFpReleasePool(3, pGFE);

      *pResult = vResult;
      return ippStsNoErr;
   }
}
