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
//     EC over Prime Finite Field (Sign, NR version)
// 
//  Contents:
//     ippsECCPSignNR()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsECCPSignNR
//
// Purpose: Signing of message representative.
//          (NR version).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pMsgDigest
//                               NULL == pPrivate
//                               NULL == pSignX
//                               NULL == pSignY
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               illegal pMsgDigest->idCtx
//                               illegal pPrivate->idCtx
//                               illegal pSignX->idCtx
//                               illegal pSignY->idCtx
//
//    ippStsIvalidPrivateKey     0 >= Private
//                               Private >= order
//
//    ippStsMessageErr           MsgDigest >= order
//                               MsgDigest <  0
//
//    ippStsRangeErr             not enough room for:
//                               signX
//                               signY
//
//    ippStsEphemeralKeyErr      (0==signX) || (0==signY)
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to be signed
//    pPrivate       pointer to the regular private key
//    pSignX,pSignY  pointer to the signature
//    pEC            pointer to the ECCP context
//
// Note:
//    - ephemeral key pair extracted from pEC and
//      must be generated and before ippsECCPNRSign() usage
//    - ephemeral key pair destroy before exit
//
*F*/
IPPFUN(IppStatus, ippsECCPSignNR,(const IppsBigNumState* pMsgDigest,
                                  const IppsBigNumState* pPrivate,
                                  IppsBigNumState* pSignX, IppsBigNumState* pSignY,
                                  IppsECCPState* pEC))
{
   /* use aligned EC context */
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   /* test private key*/
   IPP_BAD_PTR1_RET(pPrivate);
   IPP_BADARG_RET(!BN_VALID_ID(pPrivate), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pPrivate), ippStsIvalidPrivateKey);

   /* test message representative: msg>=0 */
   IPP_BAD_PTR1_RET(pMsgDigest);
   IPP_BADARG_RET(!BN_VALID_ID(pMsgDigest), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pMsgDigest), ippStsMessageErr);

   /* test signature */
   IPP_BAD_PTR2_RET(pSignX,pSignY);
   IPP_BADARG_RET(!BN_VALID_ID(pSignX), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pSignY), ippStsContextMatchErr);
   IPP_BADARG_RET((BN_ROOM(pSignX)*BITSIZE(BNU_CHUNK_T)<ECP_ORDBITSIZE(pEC)), ippStsRangeErr);
   IPP_BADARG_RET((BN_ROOM(pSignY)*BITSIZE(BNU_CHUNK_T)<ECP_ORDBITSIZE(pEC)), ippStsRangeErr);

   {
      gsModEngine* pMontR = ECP_MONT_R(pEC);
      BNU_CHUNK_T* pOrder = MOD_MODULUS(pMontR);
      int ordLen = MOD_LEN(pMontR);

      BNU_CHUNK_T* pPriData = BN_NUMBER(pPrivate);
      int priLen = BN_SIZE(pPrivate);

      BNU_CHUNK_T* pMsgData = BN_NUMBER(pMsgDigest);
      int msgLen = BN_SIZE(pMsgDigest);

      /* make sure regular 0 < private < order */
      IPP_BADARG_RET(cpEqu_BNU_CHUNK(pPriData, priLen, 0) ||
                  0<=cpCmp_BNU(pPriData, priLen, pOrder, ordLen), ippStsIvalidPrivateKey);
      /* make sure 0<= msg < order */
      IPP_BADARG_RET(0<=cpCmp_BNU(pMsgData, msgLen, pOrder, ordLen), ippStsMessageErr);

      {
         IppStatus sts = ippStsEphemeralKeyErr;

         IppsGFpState* pGF = ECP_GFP(pEC);
         gsModEngine* pMontP = GFP_PMA(pGF);
         int elmLen = GFP_FELEN(pMontP);

         BNU_CHUNK_T* dataC = BN_NUMBER(pSignX);
         BNU_CHUNK_T* dataD = BN_NUMBER(pSignY);
         BNU_CHUNK_T* buffF = BN_BUFFER(pSignX);
         int ns;

         /* ephemeral public */
         IppsGFpECPoint  ephPublic;
         cpEcGFpInitPoint(&ephPublic, ECP_PUBLIC_E(pEC), ECP_FINITE_POINT|ECP_AFFINE_POINT, pEC);

         /* ephPublic.x (mod order) */
         {
            BNU_CHUNK_T* buffer = gsModPoolAlloc(pMontP, 1);
            gfec_GetPoint(buffer, NULL, &ephPublic, pEC);
            GFP_METHOD(pMontP)->decode(buffer, buffer, pMontP);
            ns = cpMod_BNU(buffer, elmLen, pOrder, ordLen);
            cpGFpElementCopyPad(dataC, ordLen, buffer, ns);
            gsModPoolFree(pMontP, 1);
         }

         /* signX = (pMsgDigest + C) (mod order) */
         ZEXPAND_COPY_BNU(buffF, ordLen, pMsgData, msgLen);
         cpModAdd_BNU(dataC, dataC, buffF, pOrder, ordLen, dataD);

         if(!GFP_IS_ZERO(dataC, ordLen)) {

            /* signY = (eph_private - private*signX) (mod order) */
            ZEXPAND_COPY_BNU(dataD, ordLen, pPriData, priLen);
            GFP_METHOD(pMontR)->encode(dataD, dataD, pMontR);
            GFP_METHOD(pMontR)->mul(dataD, dataD, dataC, pMontR);
            cpModSub_BNU(dataD, ECP_PRIVAT_E(pEC), dataD, pOrder, ordLen, buffF);

            /* signX */
            ns = ordLen;
            FIX_BNU(dataC, ns);
            BN_SIGN(pSignX) = ippBigNumPOS;
            BN_SIZE(pSignX) = ns;

            /* signY */
            ns = ordLen;
            FIX_BNU(dataD, ns);
            BN_SIGN(pSignY) = ippBigNumPOS;
            BN_SIZE(pSignY) = ns;

            sts= ippStsNoErr;
         }

         /* clear ephemeral keys pair */
         {
            BNU_CHUNK_T* pEphPrivate = ECP_PRIVAT_E(pEC);
            BNU_CHUNK_T* pEphPublic  = ECP_PUBLIC_E(pEC);
            cpGFpElementSetChunk(pEphPrivate, BITS_BNU_CHUNK(ECP_ORDBITSIZE(pEC)), 0);
            cpGFpElementSetChunk(pEphPublic, ECP_POINTLEN(pEC), 0);
         }

         return sts;
      }
   }
}
