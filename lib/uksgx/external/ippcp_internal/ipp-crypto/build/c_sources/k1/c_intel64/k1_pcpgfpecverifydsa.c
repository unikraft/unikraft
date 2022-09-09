/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
// 
//     Context:
//        ippsGFpECVerifyDSA()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsGFpECVerifyDSA
//
// Purpose: DSA Signature Verification.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pMsgDigest
//                               NULL == pRegPublic
//                               NULL == pSignR
//                               NULL == pSignS
//                               NULL == pResult
//                               NULL == pScratchBuffer
//
//    ippStsContextMatchErr      illegal pECC->idCtx
//                               pEC->subgroup == NULL
//                               illegal pMsgDigestDigest->idCtx
//                               illegal pRegPublic->idCtx
//                               illegal pSignR->idCtx
//                               illegal pSignS->idCtx
//
//    ippStsMessageErr           0> MsgDigest
//                               order<= MsgDigest
//
//    ippStsRangeErr             SignR < 0 or SignS < 0
//
//    ippStsOutOfRangeErr        bitsize(pRegPublic) != bitsize(prime)
//
//    ippStsNotSupportedModeErr  1<GFP_EXTDEGREE(pGFE)
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to being signed
//    pRegPublic     pointer to the regular public key
//    pSignR,pSignS  pointer to the signature
//    pResult        pointer to the result: ippECValid/ippECInvalidSignature
//    pEC            pointer to the ECCP context
//    pScratchBuffer pointer to buffer (2 mul_point operation)
//
*F*/
IPPFUN(IppStatus, ippsGFpECVerifyDSA,(const IppsBigNumState* pMsgDigest,
                                      const IppsGFpECPoint* pRegPublic,
                                      const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                                      IppECResult* pResult,
                                      IppsGFpECState* pEC,
                                      Ipp8u* pScratchBuffer))
{
   IppsGFpState*  pGF;
   gsModEngine* pGFE;

   /* EC context and buffer */
   IPP_BAD_PTR2_RET(pEC, pScratchBuffer);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   pGF = ECP_GFP(pEC);
   pGFE = GFP_PMA(pGF);
   IPP_BADARG_RET(1<GFP_EXTDEGREE(pGFE), ippStsNotSupportedModeErr);

   /* test message representative */
   IPP_BAD_PTR1_RET(pMsgDigest);
   IPP_BADARG_RET(!BN_VALID_ID(pMsgDigest), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pMsgDigest), ippStsMessageErr);
   /* make sure bisize(pMsgDigest) <= bitsiz(order) */
   IPP_BADARG_RET(ECP_ORDBITSIZE(pEC) < cpBN_bitsize(pMsgDigest), ippStsMessageErr);

   /* test regular public key */
   IPP_BAD_PTR1_RET(pRegPublic);
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pRegPublic), ippStsContextMatchErr );
   IPP_BADARG_RET( ECP_POINT_FELEN(pRegPublic)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);

   /* test signature */
   IPP_BAD_PTR2_RET(pSignR, pSignS);
   IPP_BADARG_RET(!BN_VALID_ID(pSignR), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pSignS), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pSignR), ippStsRangeErr);
   IPP_BADARG_RET(BN_NEGATIVE(pSignS), ippStsRangeErr);

   /* test result */
   IPP_BAD_PTR1_RET(pResult);

   {
      IppECResult vResult = ippECInvalidSignature;

      gsModEngine* pMontR = ECP_MONT_R(pEC);
      BNU_CHUNK_T* pOrder = MOD_MODULUS(pMontR);
      int orderLen = MOD_LEN(pMontR);

      /* test mesage: msg<order */
      //IPP_BADARG_RET((0<=cpCmp_BNU(BN_NUMBER(pMsgDigest), BN_SIZE(pMsgDigest), pOrder, orderLen)), ippStsMessageErr);

      /* test signature value */
      if(!cpEqu_BNU_CHUNK(BN_NUMBER(pSignR), BN_SIZE(pSignR), 0) &&
         !cpEqu_BNU_CHUNK(BN_NUMBER(pSignS), BN_SIZE(pSignS), 0) &&
         0>cpCmp_BNU(BN_NUMBER(pSignR), BN_SIZE(pSignR), pOrder, orderLen) &&
         0>cpCmp_BNU(BN_NUMBER(pSignS), BN_SIZE(pSignS), pOrder, orderLen)) {

         int elmLen = GFP_FELEN(pGFE);
         int pelmLen = GFP_PELEN(pGFE);
         BNU_CHUNK_T* h1 = cpGFpGetPool(3, pGFE);
         BNU_CHUNK_T* h2 = h1+pelmLen;
         BNU_CHUNK_T* h  = h2+pelmLen;

         IppsGFpECPoint P;
         cpEcGFpInitPoint(&P, cpEcGFpGetPool(1, pEC),0, pEC);

         /* copy message and reduce */
         ZEXPAND_COPY_BNU(h1, orderLen, BN_NUMBER(pMsgDigest), BN_SIZE(pMsgDigest));
         cpModSub_BNU(h1, h1, pOrder, pOrder, orderLen, h2);

         /* h = d^-1, h1 = msg*h, h2 = c*h */
         ZEXPAND_COPY_BNU(h, orderLen, BN_NUMBER(pSignS),BN_SIZE(pSignS));
         gs_mont_inv(h, h, pMontR, alm_mont_inv);

         cpMontMul_BNU(h1, h, h1, pMontR);
         ZEXPAND_COPY_BNU(h2, orderLen, BN_NUMBER(pSignR),BN_SIZE(pSignR));
         cpMontMul_BNU(h2, h, h2, pMontR);

         /* P = [h1]BasePoint + [h2]publicKey */
         gfec_BasePointProduct(&P,
                               h1, orderLen, pRegPublic, h2, orderLen,
                               pEC, pScratchBuffer);

         /* check that P!=O */
         if( !gfec_IsPointAtInfinity(&P)) {
            /* get P.X */
            gfec_GetPoint(h1, NULL, &P, pEC);
            /* c' = int(P.x) mod order */
            GFP_METHOD(pGFE)->decode(h1, h1, pGFE);
            elmLen = cpMod_BNU(h1, elmLen, pOrder, orderLen);
            cpGFpElementPad(h1+elmLen, orderLen-elmLen, 0);

            /* and make sure c' = signC */
            cpGFpElementCopyPad(h2, orderLen, BN_NUMBER(pSignR), BN_SIZE(pSignR));
            if(GFP_EQ(h1, h2, orderLen))
               vResult = ippECValid;
         }

         cpEcGFpReleasePool(1, pEC);
         cpGFpReleasePool(3, pGFE);
      }

      *pResult = vResult;
      return ippStsNoErr;
   }
}
