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
//     DL over Prime Finite Field (setup/retrieve domain parameters)
// 
//  Contents:
//        ippsDLPSet()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
//    Name: ippsDLPSet
//
// Purpose: Set DL Domain Parameters.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pP
//                            NULL == pR
//                            NULL == pG
//                            NULL == pDL
//
//    ippStsContextMatchErr   illegal pP->idCtx
//                            illegal pR->idCtx
//                            illegal pG->idCtx
//                            illegal pDL->idCtx
//
//    ippStsRangeErr          not enough room for:
//                            pP,
//                            pR,
//                            pG
//
//    errors produced by      ippsMontSet()
//                            ippsMontForm()
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pP       pointer to the DL domain parameter (P)
//    pR       pointer to the DL domain parameter (R)
//    pG       pointer to the DL domain parameter (G)
//    pDSA     pointer to the DL context
//
*F*/
IPPFUN(IppStatus, ippsDLPSet,(const IppsBigNumState* pP,
                              const IppsBigNumState* pR,
                              const IppsBigNumState* pG,
                              IppsDLPState* pDL))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test DL domain parameters */
   IPP_BAD_PTR3_RET(pP, pR, pG);
   IPP_BADARG_RET(!BN_VALID_ID(pP), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pR), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pG), ippStsContextMatchErr);

   /* test size of DL domain parameters */
   IPP_BADARG_RET(BN_SIZE(pP)>BITS_BNU_CHUNK(DLP_BITSIZEP(pDL)), ippStsRangeErr);
   IPP_BADARG_RET(BN_SIZE(pR)>BITS_BNU_CHUNK(DLP_BITSIZER(pDL)), ippStsRangeErr);
   IPP_BADARG_RET(BN_SIZE(pG)>BITS_BNU_CHUNK(DLP_BITSIZEP(pDL)), ippStsRangeErr);

   /*
   // set up DL domain parameters
   */
   {
      IppStatus sts;

      DLP_FLAG(pDL) = 0;

      cpBN_zero(DLP_X(pDL));
      cpBN_zero(DLP_YENC(pDL));

      sts = gsModEngineInit(DLP_MONTP0(pDL), (Ipp32u*)BN_NUMBER(pP), cpBN_bitsize(pP), DLP_MONT_POOL_LENGTH, gsModArithDLP());

      if(ippStsNoErr==sts) {
         sts = gsModEngineInit(DLP_MONTR(pDL), (Ipp32u*)BN_NUMBER(pR), cpBN_bitsize(pR), DLP_MONT_POOL_LENGTH, gsModArithDLP());
         if(ippStsNoErr==sts) {
            cpMontEnc_BN(DLP_GENC(pDL), pG, DLP_MONTP0(pDL));
            DLP_FLAG(pDL) = ippDLPkeyP|ippDLPkeyR|ippDLPkeyG;
            return ippStsNoErr;
         }
      }
      return sts;
   }
}
