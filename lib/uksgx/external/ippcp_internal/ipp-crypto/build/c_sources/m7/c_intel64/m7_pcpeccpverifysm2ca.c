/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     EC over Prime Finite Field (Verify Signature, SM2 version)
// 
//  Contents:
//     ippsECCPVerifySM2()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsECCPVerifySM2
//
// Purpose: Verify Signature (SM2 version).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pMsgDigest
//                               NULL == pRegPublic
//                               NULL == pSignR
//                               NULL == pSignS
//                               NULL == pResult
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               illegal pMsgDigest->idCtx
//                               illegal pRegPublic->idCtx
//                               illegal pSignR->idCtx
//                               illegal pSignS->idCtx
//
//    ippStsMessageErr           0> MsgDigest
//                               order<= MsgDigest
//
//    ippStsRangeErr             SignR < 0 or SignS < 0
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to being signed
//    pRegPublic     pointer to the regular public key
//    pSignR,pSignS  pointer to the signature
//    pResult        pointer to the result: ippECValid/ippECInvalidSignature
//    pEC            pointer to the ECCP context
//
*F*/
IPPFUN(IppStatus, ippsECCPVerifySM2,(const IppsBigNumState* pMsgDigest,
                                     const IppsECCPPointState* pRegPublic,
                                     const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                                     IppECResult* pResult,
                                     IppsECCPState* pEC))
{
   IppsGFpState* pGF;
   gsModEngine* pGFE;

   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   pGF = ECP_GFP(pEC);
   pGFE = GFP_PMA(pGF);

   /* test message representative: pMsgDigest>=0 */
   IPP_BAD_PTR1_RET(pMsgDigest);
   IPP_BADARG_RET(!BN_VALID_ID(pMsgDigest), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pMsgDigest), ippStsMessageErr);
   /* make sure bisize(pMsgDigest) <= bitsize(order) */
   IPP_BADARG_RET(ECP_ORDBITSIZE(pEC) < cpBN_bitsize(pMsgDigest), ippStsMessageErr);

   /* test regular public key */
   IPP_BAD_PTR1_RET(pRegPublic);
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pRegPublic), ippStsContextMatchErr );
   IPP_BADARG_RET( ECP_POINT_FELEN(pRegPublic)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);

   /* test result */
   IPP_BAD_PTR1_RET(pResult);

   /* test signature */
   IPP_BAD_PTR2_RET(pSignR, pSignS);
   IPP_BADARG_RET(!BN_VALID_ID(pSignR), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pSignS), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pSignR), ippStsRangeErr);
   IPP_BADARG_RET(BN_NEGATIVE(pSignS), ippStsRangeErr);

   {
      IppECResult vResult = ippECInvalidSignature;

      gsModEngine* pMontR = ECP_MONT_R(pEC);
      BNU_CHUNK_T* pOrder = MOD_MODULUS(pMontR);
      int orderLen = MOD_LEN(pMontR);

      BNU_CHUNK_T* pMsgData = BN_NUMBER(pMsgDigest);
      int msgLen = BN_SIZE(pMsgDigest);

      /* test signature value */
      if(!cpEqu_BNU_CHUNK(BN_NUMBER(pSignR), BN_SIZE(pSignR), 0) &&
         !cpEqu_BNU_CHUNK(BN_NUMBER(pSignS), BN_SIZE(pSignS), 0) &&
         0>cpCmp_BNU(BN_NUMBER(pSignR), BN_SIZE(pSignR), pOrder, orderLen) &&
         0>cpCmp_BNU(BN_NUMBER(pSignS), BN_SIZE(pSignS), pOrder, orderLen)) {

         int elmLen = GFP_FELEN(pGFE);
         int ns;

         BNU_CHUNK_T* r = cpGFpGetPool(4, pGFE);
         BNU_CHUNK_T* s = r+orderLen;
         BNU_CHUNK_T* t = s+orderLen;
         BNU_CHUNK_T* f = t+orderLen;

         /* expand signatire's components */
         cpGFpElementCopyPad(r, orderLen, BN_NUMBER(pSignR), BN_SIZE(pSignR));
         cpGFpElementCopyPad(s, orderLen, BN_NUMBER(pSignS), BN_SIZE(pSignS));

         /* t = (r+s) mod order */
         cpModAdd_BNU(t, r, s, pOrder, orderLen, f);

         /* check if t!=0 */
         if( !cpIsGFpElemEquChunk_ct(t, orderLen, 0) ) {

            /* P = [s]G +[t]regPublic, t = P.x */
            IppsGFpECPoint P, G;
            cpEcGFpInitPoint(&P, cpEcGFpGetPool(1, pEC),0, pEC);
            cpEcGFpInitPoint(&G, ECP_G(pEC), ECP_AFFINE_POINT|ECP_FINITE_POINT, pEC);

            gfec_BasePointProduct(&P,
                                  s, orderLen, pRegPublic, t, orderLen,
                                  pEC, (Ipp8u*)ECP_SBUFFER(pEC));

            gfec_GetPoint(t, NULL, &P, pEC);
            GFP_METHOD(pGFE)->decode(t, t, pGFE);
            ns = cpMod_BNU(t, elmLen, pOrder, orderLen);

            cpEcGFpReleasePool(1, pEC);

            /* t = (msg+t) mod order */
            cpGFpElementCopyPad(f, orderLen, pMsgData, msgLen);
            cpModSub_BNU(f, f, pOrder, pOrder, orderLen, s);
            cpModAdd_BNU(t, t, f, pOrder, orderLen, s);

            if(GFP_EQ(t, r, orderLen))
               vResult = ippECValid;
         }

         cpGFpReleasePool(4, pGFE);

      }

      *pResult = vResult;
      return ippStsNoErr;
   }
}
