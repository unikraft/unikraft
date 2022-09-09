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
//        ippsGFpECSignNR()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsGFpECSignNR
//
// Purpose: NR Signature Generation.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pMsgDigest
//                               NULL == pRegPrivate
//                               NULL == pEphPrivate
//                               NULL == pSignR
//                               NULL == pSignS
//                               NULL == pScratchBuffer
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               pEC->subgroup == NULL
//                               illegal pMsgDigest->idCtx
//                               illegal pRegPrivate->idCtx
//                               illegal pEphPrivate->idCtx
//                               illegal pSignR->idCtx
//                               illegal pSignS->idCtx
//
//    ippStsIvalidPrivateKey     0 >= RegPrivate
//                               RegPrivate >= order
//
//                               0 >= EphPrivate
//                               EphPrivate >= order
//
//    ippStsMessageErr           pMsgDigest < 0
//                               pMsgDigest >= order
//
//    ippStsRangeErr             not enough room for:
//                               signC
//                               signD
//
//    ippStsEphemeralKeyErr      (0==signR) || (0==signS)
//
//    ippStsNotSupportedModeErr  1<GFP_EXTDEGREE(pGFE)
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to be signed
//    pRegPrivate    pointer to the regular private key
//    pEphPrivate    pointer to the ephemeral private key
//    pSignR,pSignS  pointer to the signature
//    pEC            pointer to the EC context
//    pScratchBuffer pointer to buffer (1 mul_point operation)
//
*F*/
IPPFUN(IppStatus, ippsGFpECSignNR,(const IppsBigNumState* pMsgDigest,
                                   const IppsBigNumState* pRegPrivate,
                                   IppsBigNumState* pEphPrivate,
                                   IppsBigNumState* pSignR, IppsBigNumState* pSignS,
                                   IppsGFpECState* pEC,
                                   Ipp8u* pScratchBuffer))
{
   IppsGFpState*  pGF;
   gsModEngine* pMontP;

   /* EC context and buffer */
   IPP_BAD_PTR2_RET(pEC, pScratchBuffer);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   pGF = ECP_GFP(pEC);
   pMontP = GFP_PMA(pGF);
   IPP_BADARG_RET(1<GFP_EXTDEGREE(pMontP), ippStsNotSupportedModeErr);

   /* test message representative */
   IPP_BAD_PTR1_RET(pMsgDigest);
   IPP_BADARG_RET(!BN_VALID_ID(pMsgDigest), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pMsgDigest), ippStsMessageErr);

   /* test signature */
   IPP_BAD_PTR2_RET(pSignR, pSignS);
   IPP_BADARG_RET(!BN_VALID_ID(pSignR), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pSignS), ippStsContextMatchErr);
   IPP_BADARG_RET((BN_ROOM(pSignR)*BITSIZE(BNU_CHUNK_T)<ECP_ORDBITSIZE(pEC)), ippStsRangeErr);
   IPP_BADARG_RET((BN_ROOM(pSignS)*BITSIZE(BNU_CHUNK_T)<ECP_ORDBITSIZE(pEC)), ippStsRangeErr);

   /* test private keys */
   IPP_BAD_PTR2_RET(pRegPrivate, pEphPrivate);

   IPP_BADARG_RET(!BN_VALID_ID(pRegPrivate), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pRegPrivate), ippStsIvalidPrivateKey);

   IPP_BADARG_RET(!BN_VALID_ID(pEphPrivate), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pEphPrivate), ippStsEphemeralKeyErr);

   {
      gsModEngine* pMontR = ECP_MONT_R(pEC);
      BNU_CHUNK_T* pOrder = MOD_MODULUS(pMontR);
      int ordLen = MOD_LEN(pMontR);

      BNU_CHUNK_T* dataC = BN_NUMBER(pSignR);
      BNU_CHUNK_T* dataD = BN_NUMBER(pSignS);
      BNU_CHUNK_T* buffF = BN_BUFFER(pSignR);
      BNU_CHUNK_T* buffT = BN_BUFFER(pSignS);

      BNU_CHUNK_T* pPriData = BN_NUMBER(pRegPrivate);
      int priLen = BN_SIZE(pRegPrivate);

      BNU_CHUNK_T* pEphData = BN_NUMBER(pEphPrivate);
      int ephLen = BN_SIZE(pEphPrivate);

      BNU_CHUNK_T* pMsgData = BN_NUMBER(pMsgDigest);
      int msgLen = BN_SIZE(pMsgDigest);

      /* test value of private keys: 0 < regPrivate<order, 0 < ephPrivate < order */
      IPP_BADARG_RET(cpEqu_BNU_CHUNK(pPriData, priLen, 0) ||
                  0<=cpCmp_BNU(pPriData, priLen, pOrder, ordLen), ippStsIvalidPrivateKey);
      IPP_BADARG_RET(cpEqu_BNU_CHUNK(pEphData, ephLen, 0) ||
                  0<=cpCmp_BNU(pEphData, ephLen, pOrder, ordLen), ippStsEphemeralKeyErr);

      /* test mesage: msg < order */
      IPP_BADARG_RET(0<=cpCmp_BNU(pMsgData, msgLen, pOrder, ordLen), ippStsMessageErr);

      {
         IppStatus sts = ippStsEphemeralKeyErr;

         int elmLen = GFP_FELEN(pMontP);
         int ns;

         /* compute ephemeral public key */
         IppsGFpECPoint ephPublic;
         cpEcGFpInitPoint(&ephPublic, cpEcGFpGetPool(1, pEC), 0, pEC);
         gfec_MulBasePoint(&ephPublic,
                           BN_NUMBER(pEphPrivate), BN_SIZE(pEphPrivate),
                           pEC, pScratchBuffer);

         /* x = (ephPublic.x) mod order */
         {
            BNU_CHUNK_T* buffer = gsModPoolAlloc(pMontP, 1);
            gfec_GetPoint(buffer, NULL, &ephPublic, pEC);
            GFP_METHOD(pMontP)->decode(buffer, buffer, pMontP);
            ns = cpMod_BNU(buffer, elmLen, pOrder, ordLen);
            cpGFpElementCopyPad(dataC, ordLen, buffer, ns);
            gsModPoolFree(pMontP, 1);
         }
         cpEcGFpReleasePool(1, pEC);

         /* C = (ephPublic.x + msg) mod order */
         ZEXPAND_COPY_BNU(buffF, ordLen, pMsgData, msgLen);
         cpModAdd_BNU(dataC, dataC, buffF, pOrder, ordLen, dataD);

         if(!GFP_IS_ZERO(dataC, ordLen)) {

            /* signS = (eph_private - private*signR) (mod order) */
            ZEXPAND_COPY_BNU(dataD, ordLen, pPriData, priLen);
            GFP_METHOD(pMontR)->encode(dataD, dataD, pMontR);
            GFP_METHOD(pMontR)->mul(dataD, dataD, dataC, pMontR);
            ZEXPAND_COPY_BNU(buffF, ordLen, pEphData, ephLen);
            cpModSub_BNU(dataD, buffF, dataD, pOrder, ordLen, buffT);

            /* signR */
            ns = ordLen;
            FIX_BNU(dataC, ns);
            BN_SIGN(pSignR) = ippBigNumPOS;
            BN_SIZE(pSignR) = ns;
            /* signS */
            ns = ordLen;
            FIX_BNU(dataD, ns);
            BN_SIGN(pSignS) = ippBigNumPOS;
            BN_SIZE(pSignS) = ns;

            sts = ippStsNoErr;
         }

         /* clear ephemeral private key */
         cpBN_zero(pEphPrivate);

         return sts;
      }
   }
}
