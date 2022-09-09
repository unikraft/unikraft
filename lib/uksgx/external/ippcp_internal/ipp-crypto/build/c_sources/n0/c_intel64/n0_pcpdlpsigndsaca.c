/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     DL over Prime Finite Field (Sign, DSA version)
// 
//  Contents:
//     ippsDLPSignDSA()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsDLSignDSA
//
// Purpose: Signing of message representative
//          (DSA version).
//
// Returns:                      Reason:
//    ippStsNullPtrErr              NULL == pDL
//                                  NULL == pPrvKey
//                                  NULL == pMsgDigest
//                                  NULL == pSignR
//                                  NULL == pSignS
//
//    ippStsContextMatchErr         illegal pDL->idCtx
//                                  illegal pPrvKey->idCtx
//                                  illegal pMsgDigest->idCtx
//                                  illegal pSignR->idCtx
//                                  illegal pSignS->idCtx
//
//    ippStsIncompleteContextErr
//                                  incomplete context: P and/or R and/or G is not set
//
//    ippStsMessageErr              MsgDigest >= R
//                                  MsgDigest <  0
//
//    ippStsIvalidPrivateKey        PrvKey >= R
//                                  PrvKey < 0
//
//    ippStsRangeErr                not enough room for:
//                                  signR
//                                  signS
//
//    ippStsNoErr                   no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to be signed
//    pPevKey        pointer to the (signatory's) regular private key
//    pSignR,pSignS  pointer to the signature
//    pDL            pointer to the DL context
//
// Primitive sequence call:
//    1) set up domain parameters
//    2) generate (signatory's) ephemeral key pair
//    3) set up (signatory's) ephemeral key pair
//    4) use primitive with (signatory's) private key
*F*/
IPPFUN(IppStatus, ippsDLPSignDSA,(const IppsBigNumState* pMsgDigest,
                                  const IppsBigNumState* pPrvKey,
                                        IppsBigNumState* pSignR,
                                        IppsBigNumState* pSignS,
                                  IppsDLPState *pDL))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   /* test message representative */
   IPP_BAD_PTR1_RET(pMsgDigest);
   IPP_BADARG_RET(!BN_VALID_ID(pMsgDigest), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pMsgDigest), ippStsMessageErr);

   /* test regular private key */
   IPP_BAD_PTR1_RET(pPrvKey);
   IPP_BADARG_RET(!BN_VALID_ID(pPrvKey), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pPrvKey), ippStsIvalidPrivateKey);

   /* test signature */
   IPP_BAD_PTR2_RET(pSignR,pSignS);
   IPP_BADARG_RET(!BN_VALID_ID(pSignR), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pSignS), ippStsContextMatchErr);
   IPP_BADARG_RET(BITSIZE(BNU_CHUNK_T)*BN_ROOM(pSignR)<DLP_BITSIZER(pDL), ippStsRangeErr);
   IPP_BADARG_RET(BITSIZE(BNU_CHUNK_T)*BN_ROOM(pSignS)<DLP_BITSIZER(pDL), ippStsRangeErr);

   {
      gsModEngine* pMontR = DLP_MONTR(pDL);
      BNU_CHUNK_T* pOrder = MOD_MODULUS(pMontR);
      int ordLen = MOD_LEN(pMontR);

      BNU_CHUNK_T* pPriData = BN_NUMBER(pPrvKey);
      int priLen = BN_SIZE(pPrvKey);

      BNU_CHUNK_T* pMsgData = BN_NUMBER(pMsgDigest);
      int msgLen = BN_SIZE(pMsgDigest);

      /* make sure regular 0 < private < order */
      IPP_BADARG_RET(cpEqu_BNU_CHUNK(pPriData, priLen, 0) ||
                  0<=cpCmp_BNU(pPriData, priLen, pOrder, ordLen), ippStsIvalidPrivateKey);
      /* make sure msg <order */
      IPP_BADARG_RET(0<=cpCmp_BNU(pMsgData, msgLen, pOrder, ordLen), ippStsMessageErr);

      {
         gsModEngine* pMontP = DLP_MONTP0(pDL);
         int elmLen = MOD_LEN(pMontP);

         BNU_CHUNK_T* dataR = BN_NUMBER(pSignR);
         BNU_CHUNK_T* dataS = BN_NUMBER(pSignS);
       //BNU_CHUNK_T* buffR = BN_BUFFER(pSignR);
         BNU_CHUNK_T* buffS = BN_BUFFER(pSignS);
         int ns;

         /*
         // signR = eY (mod R), eY = ephemeral public key (already set up)
         */
         BNU_CHUNK_T* buffer = gsModPoolAlloc(pMontP, 1);
         ZEXPAND_COPY_BNU(buffer, elmLen, BN_NUMBER(DLP_YENC(pDL)), BN_SIZE(DLP_YENC(pDL)));
         MOD_METHOD(pMontP)->decode(buffer, buffer, pMontP);
         ns = cpMod_BNU(buffer, elmLen, pOrder, ordLen);
         ZEXPAND_COPY_BNU(dataR, ordLen, buffer, ns);
         gsModPoolFree(pMontP, 1);

         if(!cpEqu_BNU_CHUNK(dataR, ordLen, 0)) {
            /*
            // signS = ((1/eX)*(MsgDigest + X*signR)) (mod R)
            */

            /* private representation in Montgomery domain */
            ZEXPAND_COPY_BNU(dataS, ordLen, pPriData, priLen);
            MOD_METHOD(pMontR)->encode(dataS, dataS, pMontR);

            /* (X*signR) in regular domain */
            MOD_METHOD(pMontR)->mul(dataS, dataS, dataR, pMontR);

            /* pMsgDigest + (X*signR) */
            ZEXPAND_COPY_BNU(buffS, ordLen, pMsgData, msgLen);
            cpModAdd_BNU(dataS, dataS, buffS, pOrder, ordLen, buffS);

            if(!cpEqu_BNU_CHUNK(dataS, ordLen, 0)) {

               ZEXPAND_COPY_BNU(buffS, ordLen, BN_NUMBER(DLP_X(pDL)), BN_SIZE(DLP_X(pDL)));
               /* (1/eX) in Montgomery domain  */
               gs_mont_inv(buffS, buffS, pMontR, alm_mont_inv_ct);

               /* signS = (1/eX)*(MsgDigest + X*signR) */
               MOD_METHOD(pMontR)->mul(dataS, dataS, buffS, pMontR);

               /* signR */
               ns = ordLen;
               FIX_BNU(dataR, ns);
               BN_SIGN(pSignR) = ippBigNumPOS;
               BN_SIZE(pSignR) = ns;
               /* signS */
               ns = ordLen;
               FIX_BNU(dataS, ns);
               BN_SIGN(pSignS) = ippBigNumPOS;
               BN_SIZE(pSignS) = ns;

               return ippStsNoErr;
            }
         }

         return ippStsEphemeralKeyErr;
      }
   }
}
