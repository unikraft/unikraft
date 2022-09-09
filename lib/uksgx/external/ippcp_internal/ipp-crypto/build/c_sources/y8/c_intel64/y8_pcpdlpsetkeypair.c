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
//        ippsDLPSetKeyPair()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
// Name: ippsDSASetKeyPair
//
// Purpose: Set up Key Pair into the DL context
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
//                               incomplete context: P and/or R and/or G is not set
//
//    ippStsIvalidPrivateKey     PrvKey >= R
//                               PrvKey <  0
//
//    ippStsRangeErr             PubKey >= P
//                               PubKey < 0
//
//    ippStsNoErr                no error
//
// Parameters:
//    pPrvKey  pointer to the private key
//    pPubKey  pointer to the public key
//    pDL      pointer to the DL context
*F*/
IPPFUN(IppStatus, ippsDLPSetKeyPair,(const IppsBigNumState* pPrvKey,
                                     const IppsBigNumState* pPubKey,
                                     IppsDLPState* pDL))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   /* set up private key */
   if(pPrvKey) {
      IPP_BADARG_RET(!BN_VALID_ID(pPrvKey), ippStsContextMatchErr);
      IPP_BADARG_RET(BN_NEGATIVE(pPrvKey), ippStsIvalidPrivateKey);
      {
         gsModEngine* pMontR = DLP_MONTR(pDL);
         BNU_CHUNK_T* pOrder = MOD_MODULUS(pMontR);
         int ordLen = MOD_LEN(pMontR);

         BNU_CHUNK_T* pPriData = BN_NUMBER(pPrvKey);
         int priLen = BN_SIZE(pPrvKey);

         /* make sure regular 0 < private < order */
         IPP_BADARG_RET(cpEqu_BNU_CHUNK(pPriData, priLen, 0) ||
                     0<=cpCmp_BNU(pPriData, priLen, pOrder, ordLen), ippStsIvalidPrivateKey);

         cpBN_copy(DLP_X(pDL), pPrvKey);
         BN_SIZE(DLP_X(pDL)) = ordLen;
      }
   }

   /* set up public key */
   if(pPubKey) {
      IPP_BADARG_RET(!BN_VALID_ID(pPubKey), ippStsContextMatchErr);
      IPP_BADARG_RET(BN_NEGATIVE(pPubKey), ippStsRangeErr);
      {
         gsModEngine* pMontP = DLP_MONTP0(pDL);
         BNU_CHUNK_T* pPrime = MOD_MODULUS(pMontP);
         int primeLen = MOD_LEN(pMontP);

         BNU_CHUNK_T* pPubData = BN_NUMBER(pPubKey);
         int pubLen = BN_SIZE(pPubKey);

         /* make sure regular 0 < public < prime */
         IPP_BADARG_RET(cpEqu_BNU_CHUNK(pPubData, pubLen, 0) ||
                     0<=cpCmp_BNU(pPubData, pubLen, pPrime, primeLen), ippStsRangeErr);

         cpMontEnc_BN(DLP_YENC(pDL), pPubKey, DLP_MONTP0(pDL));
      }
   }

   return ippStsNoErr;
}
