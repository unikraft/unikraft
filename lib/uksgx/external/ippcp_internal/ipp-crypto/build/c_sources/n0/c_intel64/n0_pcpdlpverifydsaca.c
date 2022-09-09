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
//     DL over Prime Finite Field (Verify, DSA version)
// 
//  Contents:
//     ippsDLPVerifyDSA()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"


/*F*
//    Name: ippsDLPVerifyDSA
//
// Purpose: Verify Signature (DSA version)
//
// Returns:                      Reason:
//    ippStsNullPtrErr              NULL == pDL
//                                  NULL == pMsgDigest
//                                  NULL == pSignR
//                                  NULL == pSignS
//                                  NULL == pResult
//
//    ippStsContextMatchErr         illegal pDL->idCtx
//                                  illegal pMsgDigest->idCtx
//                                  illegal pSignR->idCtx
//                                  illegal pSignS->idCtx
//
//    ippStsIncompleteContextErr
//                                  incomplete context
//
//    ippStsMessageErr              MsgDigest >= R
//                                  MsgDigest <  0
//
//    ippStsNoErr                   no errors
//
// Parameters:
//    pMsgDigest     pointer to the message representative to be signed
//    pSignR,pSignS  pointer to the signature
//    pResult        pointer to the result: IppSignIsValid/IppSignIsInvalid
//    pDSA           pointer to the DL context
//
// Primitive sequence call:
//    1) set up domain parameters
//    2) set up (signatory's) public key
*F*/

IPPFUN(IppStatus, ippsDLPVerifyDSA,(const IppsBigNumState* pMsgDigest,
                                    const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                                    IppDLResult* pResult,
                                    IppsDLPState* pDL))
{
   /* test context*/
   IPP_BAD_PTR2_RET(pDL,pResult);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test operation flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   /* test message representative */
   IPP_BAD_PTR1_RET(pMsgDigest);
   IPP_BADARG_RET(!BN_VALID_ID(pMsgDigest), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pMsgDigest), ippStsMessageErr);
   /* make sure msg <order */
   IPP_BADARG_RET(0<=cpCmp_BNU(BN_NUMBER(pMsgDigest), BN_SIZE(pMsgDigest),
                               DLP_R(pDL), BITS_BNU_CHUNK(DLP_BITSIZER(pDL))), ippStsMessageErr);

   /* test signature */
   IPP_BAD_PTR2_RET(pSignR,pSignS);
   IPP_BADARG_RET(!BN_VALID_ID(pSignR), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pSignS), ippStsContextMatchErr);

   /* test signature range */
   if(0<cpBN_cmp(cpBN_OneRef(), pSignR)||
      0<=cpCmp_BNU(BN_NUMBER(pSignR),BN_SIZE(pSignR), DLP_R(pDL), BITS_BNU_CHUNK(DLP_BITSIZER(pDL)))) {
      *pResult = ippDLInvalidSignature;
      return ippStsNoErr;
   }
   if(0<cpBN_cmp(cpBN_OneRef(), pSignS)||
      0<=cpCmp_BNU(BN_NUMBER(pSignS),BN_SIZE(pSignS), DLP_R(pDL), BITS_BNU_CHUNK(DLP_BITSIZER(pDL)))) {
      *pResult = ippDLInvalidSignature;
      return ippStsNoErr;
   }

   {
      /* allocate BN resources */
      BigNumNode* pList = DLP_BNCTX(pDL);
      IppsBigNumState* pW    = cpBigNumListGet(&pList);
      IppsBigNumState* pU1   = cpBigNumListGet(&pList);
      IppsBigNumState* pU2   = cpBigNumListGet(&pList);

      IppsBigNumState* pOrder = cpBigNumListGet(&pList);
      ippsSet_BN(ippBigNumPOS, BITS2WORD32_SIZE(DLP_BITSIZER(pDL)), (Ipp32u*)DLP_R(pDL), pOrder);

      /* W = 1/SignS (mod R) */
      ippsModInv_BN((IppsBigNumState*)pSignS, pOrder, pW);
      cpMontEnc_BN(pW, pW, DLP_MONTR(pDL));

      /* reduct pMsgDigest if necessary */
      if(0 < cpBN_cmp(pMsgDigest, pOrder))
         ippsMod_BN((IppsBigNumState*)pMsgDigest, pOrder, pU1);
      else
         cpBN_copy(pU1, pMsgDigest);

      /* U1 = (MsgDigest*W) (mod R) */
      cpMontMul_BN(pU1, pW, pU1, DLP_MONTR(pDL));

      /* U2 = (SignR*W) (mod R) */
      cpMontMul_BN(pU2, pSignR, pW, DLP_MONTR(pDL));

      /*
      // V = ((G^U1)*(Y^U2) (mod P)) (mod R)
      */

      /* precompute multi-exp table {1, G, Y, G*Y} */
      {
         cpSize pSize = BITS_BNU_CHUNK( DLP_BITSIZEP(pDL) );
         BNU_CHUNK_T* pX1 = BN_NUMBER(DLP_GENC(pDL));
         BNU_CHUNK_T* pX2 = BN_NUMBER(DLP_YENC(pDL));
         const BNU_CHUNK_T* ppX[2];
         ppX[0] = pX1;
         ppX[1] = pX2;

         ZEXPAND_BNU(pX1, BN_SIZE(DLP_GENC(pDL)), pSize);
         ZEXPAND_BNU(pX2, BN_SIZE(DLP_YENC(pDL)), pSize);

         cpMontMultiExpInitArray(DLP_METBL(pDL),
                                 ppX, pSize*BITSIZE(BNU_CHUNK_T),
                                 2,
                                 DLP_MONTP0(pDL));
      }
      /* W = ((G^U1)*(Y^U2) (mod P) */
      {
         cpSize sizeE1  = BN_SIZE(pU1);
         cpSize sizeE2  = BN_SIZE(pU2);
         cpSize sizeE = IPP_MAX(sizeE1, sizeE2);
         BNU_CHUNK_T* pE1 = BN_NUMBER(pU1);
         BNU_CHUNK_T* pE2 = BN_NUMBER(pU2);
         const Ipp8u* ppE[2];
         ppE[0] = (Ipp8u*)pE1;
         ppE[1] = (Ipp8u*)pE2;

         ZEXPAND_BNU(pE1, sizeE1, sizeE);
         ZEXPAND_BNU(pE2, sizeE2, sizeE);

         cpFastMontMultiExp(BN_NUMBER(pW),
                            DLP_METBL(pDL),
                            ppE, sizeE*BITSIZE(BNU_CHUNK_T),
                            2,
                            DLP_MONTP0(pDL));
         BN_SIZE(pW) = BITS_BNU_CHUNK( DLP_BITSIZEP(pDL) );
         BN_SIGN(pW) = ippBigNumPOS;
      }

      cpMontDec_BN(pW, pW, DLP_MONTP0(pDL));

      BN_SIZE(pW) = cpMod_BNU(BN_NUMBER(pW),     BN_SIZE(pW),
                              BN_NUMBER(pOrder), BN_SIZE(pOrder));

      /* result = W~R */
      *pResult = 0==cpBN_cmp(pW, pSignR)? ippDLValid : ippDLInvalidSignature;

      return ippStsNoErr;
   }
}
