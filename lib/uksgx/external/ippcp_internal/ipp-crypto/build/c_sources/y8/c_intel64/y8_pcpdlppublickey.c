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
//     DL over Prime Finite Field (EC Key Generation, Validation and Set Up)
// 
//  Contents:
//        ippsDLPPublicKey()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
// Name: ippsDLPPublicKey
//
// Purpose: Compute DL public key
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pDL
//                               NULL == pPrvKey
//                               NULL == pPubKey
//
//    ippStsContextMatchErr      invalid pDL->idCtx
//                               invalid pPrvKey->idCtx
//                               invalid pPubKey->idCtx
//
//    ippStsIncompleteContextErr
//                               incomplete context
//
//    ippStsIvalidPrivateKey     !(0 < pPrivate < DLP_R())
//
//    ippStsRangeErr             not enough room for pPubKey
//
//    ippStsNoErr                no error
//
// Parameters:
//    pPrvKey  pointer to the private key
//    pPubKey  pointer to the public  key
//    pDL      pointer to the DL context
*F*/
IPPFUN(IppStatus, ippsDLPPublicKey,(const IppsBigNumState* pPrvKey,
                                    IppsBigNumState* pPubKey,
                                    IppsDLPState* pDL))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   /* test private/public keys */
   IPP_BAD_PTR2_RET(pPrvKey, pPubKey);
   IPP_BADARG_RET(!BN_VALID_ID(pPrvKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pPubKey), ippStsContextMatchErr);

   /* test private key */
   IPP_BADARG_RET((0<=cpBN_cmp(cpBN_OneRef(), pPrvKey))||
                  (0<=cpCmp_BNU(BN_NUMBER(pPrvKey),BN_SIZE(pPrvKey), DLP_R(pDL),BITS_BNU_CHUNK(DLP_BITSIZER(pDL)))), ippStsIvalidPrivateKey);

   /* test public key's room */
   IPP_BADARG_RET(BN_ROOM(pPubKey)<BITS_BNU_CHUNK(DLP_BITSIZEP(pDL)), ippStsRangeErr);

   {
      gsModEngine* pME= DLP_MONTP0(pDL);
    //int nsM = MOD_LEN(pME);

      gsModEngine* pMEorder = DLP_MONTR(pDL);
      int ordLen = MOD_LEN(pMEorder);

      /* expand privKeyA */
      BigNumNode* pList = DLP_BNCTX(pDL);
      IppsBigNumState* pTmpPrivKey = cpBigNumListGet(&pList);
      ZEXPAND_COPY_BNU(BN_NUMBER(pTmpPrivKey), ordLen, BN_NUMBER(pPrvKey), BN_SIZE(pPrvKey));
      BN_SIZE(pTmpPrivKey) = ordLen;

      /* compute public key:  G^prvKey (mod P) */
      cpMontExpBin_BN_sscm(pPubKey, DLP_GENC(pDL), pTmpPrivKey, pME);
      cpMontDec_BN(pPubKey, pPubKey, pME);

      return ippStsNoErr;
   }
}
