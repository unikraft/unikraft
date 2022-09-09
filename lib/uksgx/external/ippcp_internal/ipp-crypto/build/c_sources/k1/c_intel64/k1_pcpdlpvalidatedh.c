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
//     DL over Prime Finite Field (validate domain parameters)
// 
//  Contents:
//        ippsDLPValidateDH()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"


/*
//    Name: DLPValidate
//
// Purpose: Validate DL Domain Parameters.
//
// Parameters:
//    nTrials     number of trials of primality test
//    pResult     pointer to the validation result
//    pDL         pointer to the DL context
//    rndFunc     external random generator
//    pRndParam   pointer to the external random generator params
*/
static
IppDLResult DLPValidate(int nTrials, IppsDLPState* pDL,
                 IppBitSupplier rndFunc, void* pRndParam)
{
   /*
   // validate DL parameters:
   //    check that P is odd and P > 2
   //    check that R is odd and R > 2
   //    check that R | (P-1)
   //    check that 1 < G < P
   //    check that 1 == G^R (mod P)
   */

   BNU_CHUNK_T* pP = DLP_P(pDL);
   BNU_CHUNK_T* pR = DLP_R(pDL);
   cpSize lenP = BITS_BNU_CHUNK(DLP_BITSIZEP(pDL));
   cpSize lenR = BITS_BNU_CHUNK(DLP_BITSIZER(pDL));

   IppsPrimeState* pPrimeCtx = DLP_PRIMEGEN(pDL);

   /* allocate BN resources */
   BigNumNode* pList = DLP_BNCTX(pDL);
   IppsBigNumState* pTmp = cpBigNumListGet(&pList);
   BNU_CHUNK_T* pT = BN_NUMBER(pTmp);

   /* P is odd and prime */
   if(0 == (pP[0] & 1))
      return ippDLBaseIsEven;
   if(0==cpPrimeTest(pP, lenP, nTrials, pPrimeCtx, rndFunc,pRndParam))
      return ippDLCompositeBase;

   /* R is odd and prime */
   if(0 == (pR[0] & 1))
      return ippDLOrderIsEven;
   if(0==cpPrimeTest(pR, lenR, nTrials, pPrimeCtx, rndFunc,pRndParam))
      return ippDLCompositeOrder;

   /* R|(P-1) */
   cpDec_BNU(pT, pP, lenP, 1);
   cpMod_BNU(pT, lenP, pR, lenR);
   if(!cpEqu_BNU_CHUNK(pT, lenP, 0))
      return ippDLInvalidCofactor;

   /* 1 < G < P */
   cpMontDec_BN(pTmp, DLP_GENC(pDL), DLP_MONTP0(pDL));
   if( 0>=cpBN_cmp(pTmp, cpBN_OneRef()) || cpCmp_BNU(pT, BN_SIZE(pTmp), DLP_P(pDL), lenP)>=0 )
      return ippDLInvalidGenerator;

   /* G^R = 1 (mod P) */
   cpMontExpBin_BNU(pT, BN_NUMBER(DLP_GENC(pDL)),lenP, pR, lenR,  DLP_MONTP0(pDL) );
   if( cpCmp_BNU(pT,lenP, MOD_MNT_R(DLP_MONTP0(pDL)), lenP) )
      return ippDLInvalidGenerator;

   return ippDLValid;
}

/*F*
//    Name: ippsDLPValidateDH
//
// Purpose: Validate DL (DH) Domain Parameters.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pDL
//                            NULL == pResult
//                            NULL == rndFunc
//
//    ippStsContextMatchErr   illegal pDL->idCtx
//
//    ippStsIncompleteContextErr
//                            incomplete context
//
//    ippStsBadArgErr         nTrials <=0
//
//    ippStsNoErr             no errors
//
// Parameters:
//    nTrials     number of trials of primality test
//    pResult     pointer to the validation result
//    pDL         pointer to the DL context
//    rndFunc     external random generator
//    pRndParam   pointer to the external random generator params
*F*/
IPPFUN(IppStatus, ippsDLPValidateDH,(int nTrials, IppDLResult* pResult, IppsDLPState* pDL,
                                     IppBitSupplier rndFunc, void* pRndParam))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test operation flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   /* test number of trials for primality check */
   IPP_BADARG_RET(nTrials<=0, ippStsBadArgErr);

   /* test another pointers */
   IPP_BAD_PTR3_RET(pResult, rndFunc, pRndParam);

   /* execute genetal DL validation */
   *pResult  = DLPValidate(nTrials, pDL, rndFunc, pRndParam);

   /*
   // DH specific validation
   */
   if(ippDLValid == *pResult) {
      /* allocate BN resources */
      BigNumNode* pList = DLP_BNCTX(pDL);
      IppsBigNumState* pT = cpBigNumListGet(&pList);

      BNU_CHUNK_T* pP = DLP_P(pDL);
      BNU_CHUNK_T* pR = DLP_R(pDL);
      cpSize feBitSize = DLP_BITSIZEP(pDL);
      cpSize ordBitSize= DLP_BITSIZER(pDL);
      cpSize lenP = BITS_BNU_CHUNK(feBitSize);
      cpSize lenR = BITS_BNU_CHUNK(ordBitSize);

      /* DLP_BITSIZEP() >= 512, 256|DLP_BITSIZEP() */
      if( (MIN_DLPDH_BITSIZE > feBitSize) ||
         (feBitSize % 256) ) {
         *pResult = ippDLInvalidBaseRange;
         return ippStsNoErr;
      }

      /* 2^(DLP_BITSIZEP()-1) < P < 2^DLP_BITSIZEP() */
      cpBN_power2(pT, feBitSize-1);
      if( 0<=cpCmp_BNU(BN_NUMBER(pT),BN_SIZE(pT), pP,lenP) ) {
         *pResult = ippDLInvalidBaseRange;
         return ippStsNoErr;
      }
      cpBN_power2(pT, feBitSize);
      if( 0>=cpCmp_BNU(BN_NUMBER(pT),BN_SIZE(pT), pP,lenP) ) {
         *pResult = ippDLInvalidBaseRange;
         return ippStsNoErr;
      }

      /* DLP_BITSIZER() >= 160 */
      if( (MIN_DLPDH_BITSIZER > ordBitSize) ) {
         *pResult = ippDLInvalidOrderRange;
         return ippStsNoErr;
      }
      /* 2^(DLP_BITSIZER()-1) < R < 2^DLP_BITSIZER() */
      cpBN_power2(pT, ordBitSize-1);
      if( 0<=cpCmp_BNU(BN_NUMBER(pT),BN_SIZE(pT), pR,lenR) ) {
         *pResult = ippDLInvalidOrderRange;
         return ippStsNoErr;
      }
      cpBN_power2(pT, ordBitSize);
      if( 0>=cpCmp_BNU(BN_NUMBER(pT),BN_SIZE(pT), pR,lenR) ) {
         *pResult = ippDLInvalidOrderRange;
         return ippStsNoErr;
      }

      /* 1 < G < (P-1) */
      cpMontDec_BN(pT, DLP_GENC(pDL), DLP_MONTP0(pDL));
      if( !(0 < cpBN_cmp(pT, cpBN_OneRef()) &&
            0 > cpCmp_BNU(BN_NUMBER(pT),BN_SIZE(pT), pP,lenP)) ) {
         *pResult = ippDLInvalidGenerator;
         return ippStsNoErr;
      }
   }

   return ippStsNoErr;
}
