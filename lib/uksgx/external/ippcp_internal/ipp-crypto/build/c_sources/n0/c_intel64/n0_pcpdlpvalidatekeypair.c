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
//        ippsDLPValidateKeyPair()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
// Name: ippsDLPValidateKeyPair
//
// Purpose: Validate DL Key Pair
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
//    ippStsNoErr                no error
//
// Parameters:
//    pPrvKey  pointer to the private key
//    pPubKey  pointer to the public  key
//    pResult  pointer to the result: ippDLValid/
//                                    ippDLInvalidPrivateKey/ippDLInvalidPublicKey/
//                                    ippDLInvalidKeyPair
//    pDL      pointer to the DL context
*F*/
IPPFUN(IppStatus, ippsDLPValidateKeyPair,(const IppsBigNumState* pPrvKey,
                                          const IppsBigNumState* pPubKey,
                                          IppDLResult* pResult,
                                          IppsDLPState* pDL))
{
   /* test DL context */
   IPP_BAD_PTR2_RET(pResult, pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   {
      /* allocate BN resources */
      BigNumNode* pList = DLP_BNCTX(pDL);
      IppsBigNumState* pTmp = cpBigNumListGet(&pList);
      BNU_CHUNK_T* pT = BN_NUMBER(pTmp);

      /* assume keys are OK */
      *pResult = ippDLValid;

      /* private key validation request */
      if(pPrvKey) {
         cpSize lenR = BITS_BNU_CHUNK(DLP_BITSIZER(pDL));
         IPP_BADARG_RET(!BN_VALID_ID(pPrvKey), ippStsContextMatchErr);

         /* test private key: 1 < pPrvKey < (R-1)  */
         cpDec_BNU(pT, DLP_R(pDL),lenR, 1);
         if( 0>=cpBN_cmp(pPrvKey, cpBN_OneRef()) ||
            cpCmp_BNU(BN_NUMBER(pPrvKey),BN_SIZE(pPrvKey), pT,lenR)>=0 ) {
            *pResult = ippDLInvalidPrivateKey;
            return ippStsNoErr;
         }
      }

      /* public key validation request */
      if(pPubKey) {
         cpSize lenP = BITS_BNU_CHUNK(DLP_BITSIZEP(pDL));
         IPP_BADARG_RET(!BN_VALID_ID(pPubKey), ippStsContextMatchErr);

         /* test public key: 1 < pPubKey < (P-1) */
         cpDec_BNU(pT, DLP_P(pDL),lenP, 1);
         if( 0>=cpBN_cmp(pPubKey, cpBN_OneRef()) ||
            cpCmp_BNU(BN_NUMBER(pPubKey),BN_SIZE(pPubKey), pT,lenP)>=0 ) {
            *pResult = ippDLInvalidPublicKey;
            return ippStsNoErr;
         }

         /* addition test: pPubKey = G^pPrvKey (mod P) */
         if(pPrvKey) {
            int ordLen = MOD_LEN( DLP_MONTR(pDL) );
            IppsBigNumState* pTmpPrivate = cpBigNumListGet(&pList);
            ZEXPAND_COPY_BNU(BN_NUMBER(pTmpPrivate), ordLen, BN_NUMBER(pPrvKey), BN_SIZE(pPrvKey));
            BN_SIZE(pTmpPrivate) = ordLen;

            /* recompute public key */
            cpMontExpBin_BN_sscm(pTmp, DLP_GENC(pDL), pTmpPrivate, DLP_MONTP0(pDL));
            cpMontDec_BN(pTmp, pTmp, DLP_MONTP0(pDL));

            /* and compare */
            if( cpBN_cmp(pTmp, pPubKey) ) {
               *pResult = ippDLInvalidKeyPair;
               return ippStsNoErr;
            }
         }
      }
   }

   return ippStsNoErr;
}
